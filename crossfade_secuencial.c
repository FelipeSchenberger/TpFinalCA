// gcc -O3 -o crossfade_secuencial crossfade_secuencial.c -lcsfml-graphics -lcsfml-window -lcsfml-system -lm
// ./crossfade_secuencial 2000x2000.jpg 2000x2000.jpg 96

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SFML/Graphics.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s color.png bw.png [num_frames]\n", argv[0]);
        return 1;
    }

    const char *file_color = argv[1];
    const char *file_bw = argv[2];
    int frames = (argc >= 4) ? atoi(argv[3]) : 96;
    if (frames <= 0) frames = 96;

    sfImage *img_color = sfImage_createFromFile(file_color);
    sfImage *img_bw = sfImage_createFromFile(file_bw);
    if (!img_color || !img_bw) {
        fprintf(stderr, "Error cargando las imágenes.\n");
        return 1;
    }

    sfVector2u s1 = sfImage_getSize(img_color);
    sfVector2u s2 = sfImage_getSize(img_bw);
    if (s1.x != s2.x || s1.y != s2.y) {
        fprintf(stderr, "Las imágenes deben tener el mismo tamaño.\n");
        return 1;
    }

    unsigned width = s1.x, height = s1.y;
    size_t npix = (size_t)width * height;

    const sfUint8 *pc = sfImage_getPixelsPtr(img_color);
    const sfUint8 *pb = sfImage_getPixelsPtr(img_bw);

    unsigned char *out = malloc(npix * 4);
    if (!out) {
        fprintf(stderr, "Error: no se pudo asignar memoria de salida.\n");
        return 1;
    }

    for (int f = 0; f < frames; ++f) {
        float P = f / (float)(frames - 1);

        for (size_t i = 0; i < npix; i++) {
            unsigned char cr = pc[4 * i + 0];
            unsigned char cg = pc[4 * i + 1];
            unsigned char cb = pc[4 * i + 2];
            unsigned char br = pb[4 * i + 0];
            unsigned char bg = pb[4 * i + 1];
            unsigned char bb = pb[4 * i + 2];

            out[4 * i + 0] = (unsigned char)(cr * (1.0f - P) + br * P);
            out[4 * i + 1] = (unsigned char)(cg * (1.0f - P) + bg * P);
            out[4 * i + 2] = (unsigned char)(cb * (1.0f - P) + bb * P);
            out[4 * i + 3] = 255;
        }

        sfImage *outImg = sfImage_createFromPixels(width, height, out);
        if (!outImg) {
            fprintf(stderr, "Error creando imagen de salida.\n");
            break;
        }

        char fname[128];
        snprintf(fname, sizeof(fname), "frame_%03d.png", f);
        if (!sfImage_saveToFile(outImg, fname))
            fprintf(stderr, "Error guardando %s\n", fname);
        else
            printf("Guardado: %s\n", fname);

        sfImage_destroy(outImg);
    }

    free(out);
    sfImage_destroy(img_color);
    sfImage_destroy(img_bw);

    return 0;
}
