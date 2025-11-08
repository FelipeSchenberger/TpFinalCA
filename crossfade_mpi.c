// mpicc -O3 -o crossfade_mpi crossfade_mpi.c -lcsfml-graphics -lcsfml-window -lcsfml-system -lm
// mpirun -np 4 ./crossfade_mpi 800x8001.jpg 800x8002.jpg 96

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <SFML/Graphics.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0)
            fprintf(stderr, "Uso: %s color.png bw.png [num_frames]\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    const char *file_color = argv[1];
    const char *file_bw = argv[2];
    int frames = (argc >= 4) ? atoi(argv[3]) : 96;
    if (frames <= 0) frames = 96;

    unsigned width = 0, height = 0;
    unsigned char *colorRGB = NULL;
    unsigned char *bwRGB = NULL;

    sfImage *img_c = sfImage_createFromFile(file_color);
    sfImage *img_b = sfImage_createFromFile(file_bw);
    if (!img_c || !img_b) {
        fprintf(stderr, "Proceso %d: error cargando imágenes\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    sfVector2u sc = sfImage_getSize(img_c), sb = sfImage_getSize(img_b);
    if (sc.x != sb.x || sc.y != sb.y) {
        fprintf(stderr, "Proceso %d: tamaños diferentes\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    width = sc.x;
    height = sc.y;
    const sfUint8 *pc = sfImage_getPixelsPtr(img_c);
    const sfUint8 *pb = sfImage_getPixelsPtr(img_b);
    size_t npix = (size_t)width * height;
    colorRGB = malloc(npix * 3);
    bwRGB = malloc(npix * 3);

    for (size_t i = 0; i < npix; ++i) {
        colorRGB[3 * i + 0] = pc[4 * i + 0];
        colorRGB[3 * i + 1] = pc[4 * i + 1];
        colorRGB[3 * i + 2] = pc[4 * i + 2];
        bwRGB[3 * i + 0] = pb[4 * i + 0];
        bwRGB[3 * i + 1] = pb[4 * i + 1];
        bwRGB[3 * i + 2] = pb[4 * i + 2];
    }

    sfImage_destroy(img_c);
    sfImage_destroy(img_b);

    unsigned char *out = malloc(npix * 4);

    int base = frames / size;
    int resto = frames % size;
    int start = rank * base + (rank < resto ? rank : resto);
    int count = base + (rank < resto ? 1 : 0);
    int end = start + count;

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    for (int f = start; f < end; ++f) {
        float P = f / (float)(frames - 1);
        for (size_t i = 0; i < npix; i++) {
            unsigned char cr = colorRGB[3 * i + 0];
            unsigned char cg = colorRGB[3 * i + 1];
            unsigned char cb = colorRGB[3 * i + 2];
            unsigned char br = bwRGB[3 * i + 0];
            unsigned char bg = bwRGB[3 * i + 1];
            unsigned char bb = bwRGB[3 * i + 2];
            out[4 * i + 0] = (unsigned char)(cr * (1.0f - P) + br * P);
            out[4 * i + 1] = (unsigned char)(cg * (1.0f - P) + bg * P);
            out[4 * i + 2] = (unsigned char)(cb * (1.0f - P) + bb * P);
            out[4 * i + 3] = 255;
        }

        sfImage *outImg = sfImage_createFromPixels(width, height, out);
        if (outImg) {
            char fname[128];
            snprintf(fname, sizeof(fname), "frame_%03d_proc%d.png", f, rank);
            if (!sfImage_saveToFile(outImg, fname))
                fprintf(stderr, "Proceso %d: error guardando %s\n", rank, fname);
            else
                printf("Proceso %d generó %s\n", rank, fname);
            sfImage_destroy(outImg);
        } else {
            fprintf(stderr, "Proceso %d: error creando imagen\n", rank);
        }
        fflush(stdout);
    }

    double t1 = MPI_Wtime();
    double elapsed = t1 - t0;
    double max_elapsed;
    MPI_Reduce(&elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (rank == 0)
        printf("Tiempo total MPI (max proc) = %.6f s\n", max_elapsed);

    free(colorRGB);
    free(bwRGB);
    free(out);

    MPI_Finalize();
    return 0;
}

