#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <time.h> // Include time.h for clock()

#define THRESHOLD_VALUE 128

int main(int argc, char *argv[]) {
    clock_t start_time, end_time;
    double total_time;

    if (argc != 2) {
        printf("Usage: %s <image_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *imagePath = argv[1];

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
        fprintf(stderr, "SDL_image initialization failed: %s\n", IMG_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow("Thresholded Image",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        640, 480, SDL_WINDOW_SHOWN);

    if (window == NULL) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Surface *imageSurface = IMG_Load(imagePath);
    if (imageSurface == NULL) {
        fprintf(stderr, "Failed to load image: %s\n", IMG_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return EXIT_FAILURE;
    }

    // Measure execution time
    start_time = clock();

    // Thresholding (convert image to binary)
    SDL_LockSurface(imageSurface);

    for (int y = 0; y < imageSurface->h; ++y) {
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

    printf("Execution Time: %f seconds\n", total_time);

    SDL_Surface *screenSurface = SDL_GetWindowSurface(window);
    SDL_BlitSurface(imageSurface, NULL, screenSurface, NULL);
    SDL_UpdateWindowSurface(window);

    SDL_Delay(5000); // Display for 5 seconds

    SDL_FreeSurface(imageSurface);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return EXIT_SUCCESS;
}
