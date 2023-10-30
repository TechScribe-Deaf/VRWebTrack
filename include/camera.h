#ifndef CAMERA_H
#define CAMERA_H
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdatomic.h>
#include <stdbool.h>

/**
 * @brief Convert YUYV pixel format to RGB pixel format.
 *
 * @param yuyv_buffer Pointer to the source buffer containing YUYV data.
 * @param rgb_buffer Pointer to the destination buffer to store RGB data.
 * @param width Width of the image in pixels.
 * @param height Height of the image in pixels.
 */
void yuyv_to_rgb(unsigned char *yuyv_buffer, unsigned char *rgb_buffer, int width, int height);

/**
 * @brief Write the contents of a buffer to a file.
 *
 * @param file_path Path to the file to write.
 * @param buffer Pointer to the buffer containing the data.
 * @param size Size of the data in bytes.
 * @return 0 on success, or error code on failure.
 */
int write_buffer_to_file(const char *file_path, unsigned char *buffer, size_t size);

/**
 * @brief Write RGB image data to a BMP file.
 *
 * @param file_path Path to the BMP file to write.
 * @param rgb_buffer Pointer to the buffer containing RGB data.
 * @param width Width of the image in pixels.
 * @param height Height of the image in pixels.
 * @return 0 on success, or error code on failure.
 */
int write_rgb_to_bmp(const char *file_path, unsigned char *rgb_buffer, int width, int height);

/**
 * @brief Print the capabilities of a webcam.
 *
 * @param fd File descriptor of the webcam.
 * @param width Desired width of the image in pixels.
 * @param height Desired height of the image in pixels.
 * @return 0 on success, or error code on failure.
 */
int PrintCapsOfWebcam(int fd, uint32_t width, uint32_t height);

/**
 * @brief Decode an AV packet and convert it to RGB format.
 *
 * @param codec_context Pointer to the AVCodecContext.
 * @param sws_ctx Pointer to the SwsContext for pixel format conversion.
 * @param packet Pointer to the AVPacket to decode.
 * @param frame Pointer to the AVFrame to store the decoded frame.
 * @param rgb_frame Pointer to the AVFrame to store the RGB frame.
 * @param rgb_buffer Pointer to the buffer to store RGB data.
 * @param width Width of the image in pixels.
 * @param height Height of the image in pixels.
 * @return 0 on success, or error code on failure.
 */
int decode_packet(AVCodecContext *codec_context, struct SwsContext* sws_ctx, AVPacket *packet, AVFrame *frame, AVFrame *rgb_frame, unsigned char *rgb_buffer, int width, int height);

/**
 * @brief Start capturing video using the V4L2 API and FFmpeg libraries.
 * 
 * This function performs the following key operations:
 * - Initializes FFmpeg codecs for video decoding
 * - Sets up the video capture format and frame rate
 * - Manages buffers using mmap
 * - Handles video frame decoding and optional scaling
 *
 * @param pathToCamera Path to the camera device (e.g., "/dev/video0").
 * @param width Desired width of the capture in pixels.
 * @param height Desired height of the capture in pixels.
 * @param fps Desired frames per second (fps) for video capture.
 * @param quit Atomic flag to indicate if capturing should stop; if set to non-zero, capturing stops.
 * 
 * @return 0 on success, or a non-zero error code on failure.
 *
 * Error codes:
 * 1 - Failure due to memory allocation, codec initialization, device setup, or other internal issues.
 */
int start_capture(const char* pathToCamera, uint32_t width, uint32_t height, uint32_t fps, atomic_int *quit);

#endif