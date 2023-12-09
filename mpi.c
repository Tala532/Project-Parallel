#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <time.h>
#include <mpi.h>

#define THRESHOLD_VALUE 128

int main(int argc, char *argv[]) {
    clock_t start_time, end_time;
    double total_time;

    if (argc != 2) {
        printf("Usage: %s <image_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *imagePath = argv[1];

    MPI_Init(&argc, &argv);

    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
        fprintf(stderr, "SDL_image initialization failed: %s\n", IMG_GetError());
        SDL_Quit();
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow("Thresholded Image",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        640, 480, SDL_WINDOW_SHOWN);

    if (window == NULL) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    SDL_Surface *imageSurface = IMG_Load(imagePath);
    if (imageSurface == NULL) {
        fprintf(stderr, "Failed to load image: %s\n", IMG_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int rows_per_process = imageSurface->h / num_procs;
    int start_row = rank * rows_per_process;
    int end_row = (rank == num_procs - 1) ? imageSurface->h : start_row + rows_per_process;

    // Measure execution time
    start_time = clock();

    // Thresholding (convert image to binary)
    SDL_LockSurface(imageSurface);

    for (int y = start_row; y < end_row; ++y) {
        for (int x = 0; x < imageSurface->w; ++x) {
            Uint32 *pixel = (Uint32 *)((Uint8 *)imageSurface->pixels + y * imageSurface->pitch + x * sizeof(Uint32));
            Uint8 r, g, b;
            SDL_GetRGB(*pixel, imageSurface->format, &r, &g, &b);
            Uint8 gray = (Uint8)(0.3 * r + 0.59 * g + 0.11 * b);

            if (gray < THRESHOLD_VALUE) {
                *pixel = SDL_MapRGB(imageSurface->format, 0, 0, 0); // Black
            } else {
                *pixel = SDL_MapRGB(imageSurface->format, 255, 255, 255); // White
            }
        }
    }

    SDL_UnlockSurface(imageSurface);

    end_time = clock();
    total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("Process %d Execution Time: %f seconds\n", rank, total_time);

    // Gather results from all processes
    MPI_Barrier(MPI_COMM_WORLD);

    // Merge image segments (assuming here that the root process is 0)
    if (rank == 0) {
        // Gather data from other processes
        for (int i = 1; i < num_procs; ++i) {
            int recv_start_row = i * rows_per_process;
            int recv_end_row = (i == num_procs - 1) ? imageSurface->h : recv_start_row + rows_per_process;
            MPI_Recv((Uint8 *)imageSurface->pixels + recv_start_row * imageSurface->pitch,
                     (recv_end_row - recv_start_row) * imageSurface->pitch, MPI_BYTE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        MPI_Send((Uint8 *)imageSurface->pixels + start_row * imageSurface->pitch,
                 (end_row - start_row) * imageSurface->pitch, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
    }

    if (rank == 0) {
        SDL_Surface *screenSurface = SDL_GetWindowSurface(window);
        SDL_BlitSurface(imageSurface, NULL, screenSurface, NULL);
        SDL_UpdateWindowSurface(window);

        SDL_Delay(10000); // Display for 5 seconds

        SDL_FreeSurface(imageSurface);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
