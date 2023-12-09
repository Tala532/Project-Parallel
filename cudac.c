#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <time.h>

#define THRESHOLD_VALUE 128
#define BLOCK_SIZE 16

global void thresholdImageKernel(unsigned char *image, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < width && y < height) {
        int index = y * width + x;
        unsigned char pixel = image[index];

        // Thresholding
        image[index] = (pixel < THRESHOLD_VALUE) ? 0 : 255;
    }
}

int main(int argc, char *argv[]) {
    clock_t start_time, end_time;
    double total_time;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *imagePath = argv[1];

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
        fprintf(stderr, "SDL_image initialization failed: %s\n", IMG_GetError());
        return EXIT_FAILURE;
    }

    SDL_Surface *imageSurface = IMG_Load(imagePath);
    if (imageSurface == NULL) {
        fprintf(stderr, "Failed to load image: %s\n", IMG_GetError());
        IMG_Quit();
        return EXIT_FAILURE;
    }

    int width = imageSurface->w;
    int height = imageSurface->h;

    // Host (CPU) memory for the image
    unsigned char *hostImage = (unsigned char *)malloc(width * height * sizeof(unsigned char));

    // Copy image data to host memory
    SDL_LockSurface(imageSurface);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Uint32 *pixel = (Uint32 *)((Uint8 *)imageSurface->pixels + y * imageSurface->pitch + x * sizeof(Uint32));
            Uint8 r, g, b;
            SDL_GetRGB(*pixel, imageSurface->format, &r, &g, &b);
            hostImage[y * width + x] = (Uint8)(0.3 * r + 0.59 * g + 0.11 * b);
        }
    }
    SDL_UnlockSurface(imageSurface);

    // Device (GPU) memory for the image
    unsigned char *deviceImage;
    cudaMalloc((void **)&deviceImage, width * height * sizeof(unsigned char));
    cudaMemcpy(deviceImage, hostImage, width * height * sizeof(unsigned char), cudaMemcpyHostToDevice);

    // Measure execution time
    start_time = clock();

    // Launch CUDA kernel
    dim3 dimBlock(BLOCK_SIZE, BLOCK_SIZE);
    dim3 dimGrid((width + dimBlock.x - 1) / dimBlock.x, (height + dimBlock.y - 1) / dimBlock.y);
    thresholdImageKernel<<<dimGrid, dimBlock>>>(deviceImage, width, height);
    cudaDeviceSynchronize();
    printf("Grid Size: (%d, %d)\n", dimGrid.x, dimGrid.y);

    // Measure execution time
    end_time = clock();
    total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Execution Time: %f seconds\n", total_time);

    // Copy the result back to the host
    cudaMemcpy(hostImage, deviceImage, width * height * sizeof(unsigned char), cudaMemcpyDeviceToHost);

    // Free device and host memory
    cudaFree(deviceImage);
    free(hostImage);

    // Cleanup SDL
    SDL_FreeSurface(imageSurface);
    IMG_Quit();

    return EXIT_SUCCESS;
}
