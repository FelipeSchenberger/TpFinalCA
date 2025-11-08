// gcc -O3 -fopenmp -march=native -o crossfade_omp crossfade_omp.c -lcsfml-graphics -lcsfml-window -lcsfml-system -lm
// export OMP_NUM_THREADS=4
// ./crossfade_omp 2000x2000.jpg 2000x2000.jpg 96

#include <SFML/Graphics.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <imagen_color> <imagen_bw> [frames]\n", argv[0]);
        return 1;
    }

    const char *img_color_path = argv[1];
    const char *img_bw_path = argv[2];
    int frames = (argc >= 4) ? atoi(argv[3]) : 96;
    if (frames <= 0) frames = 96;

    sfImage *img_color = sfImage_createFromFile(img_color_path);
    sfImage *img_bw = sfImage_createFromFile(img_bw_path);
    if (!img_color || !img_bw) {
        fprintf(stderr, "Error cargando imágenes.\n");
        return 1;
    }

    sfVector2u size1 = sfImage_getSize(img_color);
    sfVector2u size2 = sfImage_getSize(img_bw);
    if (size1.x != size2.x || size1.y != size2.y) {
        fprintf(stderr, "Las imágenes deben tener el mismo tamaño.\n");
        return 1;
    }

    unsigned width = size1.x, height = size1.y;
    size_t npix = (size_t)width * height;

    const sfUint8 *pix_color = sfImage_getPixelsPtr(img_color);
    const sfUint8 *pix_bw = sfImage_getPixelsPtr(img_bw);

    double start_time = omp_get_wtime();

    printf("Generando %d frames con OpenMP (%u x %u)...\n", frames, width, height);

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int num_threads = omp_get_num_threads();

        unsigned char *out = malloc(npix * 4);
        if (!out) {
            fprintf(stderr, "Error de memoria en hilo %d\n", thread_id);
            exit(1);
        }

        #pragma omp for schedule(static)
        for (int f = 0; f < frames; f++) {
            float P = f / (float)(frames - 1);

            for (size_t i = 0; i < npix; i++) {
                unsigned char cr = pix_color[4 * i + 0];
                unsigned char cg = pix_color[4 * i + 1];
                unsigned char cb = pix_color[4 * i + 2];

                unsigned char br = pix_bw[4 * i + 0];
                unsigned char bg = pix_bw[4 * i + 1];
                unsigned char bb = pix_bw[4 * i + 2];

                out[4 * i + 0] = (unsigned char)(cr * (1.0f - P) + br * P);
                out[4 * i + 1] = (unsigned char)(cg * (1.0f - P) + bg * P);
                out[4 * i + 2] = (unsigned char)(cb * (1.0f - P) + bb * P);
                out[4 * i + 3] = 255;
            }

            printf("Hilo %d generando frame %d\n", thread_id, f);

            sfImage *outImg = sfImage_createFromPixels(width, height, out);
            char fname[128];
            snprintf(fname, sizeof(fname), "frame_%03d_thread_%d.png", f, thread_id);
            sfImage_saveToFile(outImg, fname);
            sfImage_destroy(outImg);
        }

        free(out);
    }

    double end_time = omp_get_wtime();
    printf("Tiempo total: %.3f segundos\n", end_time - start_time);

    sfImage_destroy(img_color);
    sfImage_destroy(img_bw);

    return 0;
}
