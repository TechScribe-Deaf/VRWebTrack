#include "camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

void yuyv_to_rgb(unsigned char *yuyv_buffer, unsigned char *rgb_buffer, int width, int height) {
    int yuyv_index = 0;
    int rgb_index = 0;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; j += 2) {
            unsigned char y1 = yuyv_buffer[yuyv_index++];
            unsigned char u = yuyv_buffer[yuyv_index++];
            unsigned char y2 = yuyv_buffer[yuyv_index++];
            unsigned char v = yuyv_buffer[yuyv_index++];

            // Convert first pixel
            int r1 = y1 + 1.402 * (v - 128);
            int g1 = y1 - 0.344136 * (u - 128) - 0.714136 * (v - 128);
            int b1 = y1 + 1.772 * (u - 128);

            // Clamp and assign to RGB buffer
            rgb_buffer[rgb_index++] = (unsigned char)(r1 > 255 ? 255 : r1 < 0 ? 0 : r1);
            rgb_buffer[rgb_index++] = (unsigned char)(g1 > 255 ? 255 : g1 < 0 ? 0 : g1);
            rgb_buffer[rgb_index++] = (unsigned char)(b1 > 255 ? 255 : b1 < 0 ? 0 : b1);

            // Convert second pixel
            int r2 = y2 + 1.402 * (v - 128);
            int g2 = y2 - 0.344136 * (u - 128) - 0.714136 * (v - 128);
            int b2 = y2 + 1.772 * (u - 128);

            // Clamp and assign to RGB buffer
            rgb_buffer[rgb_index++] = (unsigned char)(r2 > 255 ? 255 : r2 < 0 ? 0 : r2);
            rgb_buffer[rgb_index++] = (unsigned char)(g2 > 255 ? 255 : g2 < 0 ? 0 : g2);
            rgb_buffer[rgb_index++] = (unsigned char)(b2 > 255 ? 255 : b2 < 0 ? 0 : b2);
        }
    }
}

int write_rgb_to_bmp(const char *file_path, unsigned char *rgb_buffer, int width, int height) {
    FILE *file = fopen(file_path, "wb");
    if (file == NULL) {
        fprintf(stderr,"Could not open file for writing");
        return -1;
    }

    // Bitmap file header (14 bytes)
    uint16_t bfType = 0x4D42;  // 'BM'
    uint32_t bfSize = 14 + 40 + (width * height * 3);
    uint16_t bfReserved1 = 0;
    uint16_t bfReserved2 = 0;
    uint32_t bfOffBits = 14 + 40;  // File header size + DIB header size

    // Bitmap info header (DIB header, 40 bytes)
    uint32_t biSize = 40;
    int32_t biWidth = width;
    int32_t biHeight = height;
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 24;  // 3 bytes per pixel
    uint32_t biCompression = 0;  // BI_RGB, no compression
    uint32_t biSizeImage = width * height * 3;
    int32_t biXPelsPerMeter = 0;
    int32_t biYPelsPerMeter = 0;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;

    // Write headers
    fwrite(&bfType, 1, 2, file);
    fwrite(&bfSize, 1, 4, file);
    fwrite(&bfReserved1, 1, 2, file);
    fwrite(&bfReserved2, 1, 2, file);
    fwrite(&bfOffBits, 1, 4, file);
    fwrite(&biSize, 1, 4, file);
    fwrite(&biWidth, 1, 4, file);
    fwrite(&biHeight, 1, 4, file);
    fwrite(&biPlanes, 1, 2, file);
    fwrite(&biBitCount, 1, 2, file);
    fwrite(&biCompression, 1, 4, file);
    fwrite(&biSizeImage, 1, 4, file);
    fwrite(&biXPelsPerMeter, 1, 4, file);
    fwrite(&biYPelsPerMeter, 1, 4, file);
    fwrite(&biClrUsed, 1, 4, file);
    fwrite(&biClrImportant, 1, 4, file);

    // Write pixel data
    for (int y = height - 1; y >= 0; y--) {  // BMP stores rows bottom-to-top
        unsigned char *row = rgb_buffer + y * width * 3;
        for (int x = 0; x < width; x++) {
            fwrite(row + x * 3 + 2, 1, 1, file);  // B
            fwrite(row + x * 3 + 1, 1, 1, file);  // G
            fwrite(row + x * 3, 1, 1, file);  // R
        }
    }

    fclose(file);
    return 0;
}
