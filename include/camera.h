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

#ifndef CAMERA_IMPLEMENTATION
typedef struct camera_t camera_t;
#endif

typedef enum camera_pixel_format
{
/*
The following code for camera_pixel_format enumeration was
transposed from videodev2.h in Linux Kernel development branch which
can be found online at the following link:

https://github.com/torvalds/linux/blob/eb55307e6716b1a02f7db05e27d60e8ca2289c03/include/uapi/linux/videodev2.h#L540

The code at the time of writing on 10/30/2023 10:45PM -7GMT
was licensed under BSD-3-Clause in the following notice:

SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR BSD-3-Clause)
Video for Linux Two header file

Copyright (C) 1999-2012 the contributors

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Alternatively you can redistribute this file under the terms of the
BSD license as stated below:
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
3. The names of its contributors may not be used to endorse or promote
    products derived from this software without specific prior written
    permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Header file for v4l or V4L2 drivers and applications
with public API.
All kernel-specific stuff were moved to media/v4l2-dev.h, so
no #if __KERNEL tests are allowed here

See https://linuxtv.org for more info

Author: Bill Dirks <bill@thedirks.org>
Justin Schoeman
Hans Verkuil <hverkuil@xs4all.nl>
et al.
*/
    /* RGB formats (1 or 2 bytes per pixel) */
    camera_pixel_format_unknown,
    camera_pixel_format_RGB332,
    camera_pixel_format_RGB444,
    camera_pixel_format_ARGB444,
    camera_pixel_format_XRGB444,
    camera_pixel_format_RGBA444,
    camera_pixel_format_RGBX444,
    camera_pixel_format_ABGR444,
    camera_pixel_format_XBGR444,
    camera_pixel_format_BGRA444,
    camera_pixel_format_BGRX444,
    camera_pixel_format_RGB555,
    camera_pixel_format_ARGB555,
    camera_pixel_format_XRGB555,
    camera_pixel_format_RGBA555,
    camera_pixel_format_RGBX555,
    camera_pixel_format_ABGR555,
    camera_pixel_format_XBGR555,
    camera_pixel_format_BGRA555,
    camera_pixel_format_BGRX555,
    camera_pixel_format_RGB565,
    camera_pixel_format_RGB555X,
    camera_pixel_format_ARGB555X,
    camera_pixel_format_XRGB555X,
    camera_pixel_format_RGB565X,

    /* RGB formats (3 or 4 bytes per pixel) */
    camera_pixel_format_BGR666,
    camera_pixel_format_BGR24,
    camera_pixel_format_RGB24,
    camera_pixel_format_BGR32,
    camera_pixel_format_ABGR32,
    camera_pixel_format_XBGR32,
    camera_pixel_format_BGRA32,
    camera_pixel_format_BGRX32,
    camera_pixel_format_RGB32,
    camera_pixel_format_RGBA32,
    camera_pixel_format_RGBX32,
    camera_pixel_format_ARGB32,
    camera_pixel_format_XRGB32,
    camera_pixel_format_RGBX1010102,
    camera_pixel_format_RGBA1010102,
    camera_pixel_format_ARGB2101010,

    /* RGB formats (6 or 8 bytes per pixel) */
    camera_pixel_format_BGR48_12,
    camera_pixel_format_ABGR64_12,

    /* Grey formats */
    camera_pixel_format_GREY,
    camera_pixel_format_Y4,
    camera_pixel_format_Y6,
    camera_pixel_format_Y10,
    camera_pixel_format_Y12,
    camera_pixel_format_Y012,
    camera_pixel_format_Y14,
    camera_pixel_format_Y16,
    camera_pixel_format_Y16_BE,

    /* Grey bit-packed formats */
    camera_pixel_format_Y10BPACK,
    camera_pixel_format_Y10P,
    camera_pixel_format_IPU3_Y10,

    /* Palette formats */
    camera_pixel_format_PAL8,

    /* Chrominance formats */
    camera_pixel_format_UV8,

    /* Luminance+Chrominance formats */
    camera_pixel_format_YUYV,
    camera_pixel_format_YYUV,
    camera_pixel_format_YVYU,
    camera_pixel_format_UYVY,
    camera_pixel_format_VYUY,
    camera_pixel_format_Y41P,
    camera_pixel_format_YUV444,
    camera_pixel_format_YUV555,
    camera_pixel_format_YUV565,
    camera_pixel_format_YUV24,
    camera_pixel_format_YUV32,
    camera_pixel_format_AYUV32,
    camera_pixel_format_XYUV32,
    camera_pixel_format_VUYA32,
    camera_pixel_format_VUYX32,
    camera_pixel_format_YUVA32,
    camera_pixel_format_YUVX32,
    camera_pixel_format_M420,
    camera_pixel_format_YUV48_12,

    /*
     * YCbCr packed format. For each Y2xx format, xx bits of valid data occupy the MSBs
     * of the 16 bit components, and 16-xx bits of zero padding occupy the LSBs.
     */
    camera_pixel_format_Y210,
    camera_pixel_format_Y212,
    camera_pixel_format_Y216,

    /* two planes -- one Y, one Cr + Cb interleaved  */
    camera_pixel_format_NV12,
    camera_pixel_format_NV21,
    camera_pixel_format_NV16,
    camera_pixel_format_NV61,
    camera_pixel_format_NV24,
    camera_pixel_format_NV42,
    camera_pixel_format_P010,
    camera_pixel_format_P012,

    /* two non contiguous planes - one Y, one Cr + Cb interleaved  */
    camera_pixel_format_NV12M,
    camera_pixel_format_NV21M,
    camera_pixel_format_NV16M,
    camera_pixel_format_NV61M,
    camera_pixel_format_P012M,

    /* three planes - Y Cb, Cr */
    camera_pixel_format_YUV410,
    camera_pixel_format_YVU410,
    camera_pixel_format_YUV411P,
    camera_pixel_format_YUV420,
    camera_pixel_format_YVU420,
    camera_pixel_format_YUV422P,

    /* three non contiguous planes - Y, Cb, Cr */
    camera_pixel_format_YUV420M,
    camera_pixel_format_YVU420M,
    camera_pixel_format_YUV422M,
    camera_pixel_format_YVU422M,
    camera_pixel_format_YUV444M,
    camera_pixel_format_YVU444M,

    /* Tiled YUV formats */
    camera_pixel_format_NV12_4L4,
    camera_pixel_format_NV12_16L16,
    camera_pixel_format_NV12_32L32,
    camera_pixel_format_P010_4L4,
    camera_pixel_format_NV12_8L128,
    camera_pixel_format_NV12_10BE_8L128,

    /* Tiled YUV formats, non contiguous planes */
    camera_pixel_format_NV12MT,
    camera_pixel_format_NV12MT_16X16,
    camera_pixel_format_NV12M_8L128,
    camera_pixel_format_NV12M_10BE_8L128,

    /* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
    camera_pixel_format_SBGGR8,
    camera_pixel_format_SGBRG8,
    camera_pixel_format_SGRBG8,
    camera_pixel_format_SRGGB8,
    camera_pixel_format_SBGGR10,
    camera_pixel_format_SGBRG10,
    camera_pixel_format_SGRBG10,
    camera_pixel_format_SRGGB10,
    /* 10bit raw bayer packed, 5 bytes for every 4 pixels */
    camera_pixel_format_SBGGR10P,
    camera_pixel_format_SGBRG10P,
    camera_pixel_format_SGRBG10P,
    camera_pixel_format_SRGGB10P,
    /* 10bit raw bayer a-law compressed to 8 bits */
    camera_pixel_format_SBGGR10ALAW8,
    camera_pixel_format_SGBRG10ALAW8,
    camera_pixel_format_SGRBG10ALAW8,
    camera_pixel_format_SRGGB10ALAW8,
    /* 10bit raw bayer DPCM compressed to 8 bits */
    camera_pixel_format_SBGGR10DPCM8,
    camera_pixel_format_SGBRG10DPCM8,
    camera_pixel_format_SGRBG10DPCM8,
    camera_pixel_format_SRGGB10DPCM8,
    camera_pixel_format_SBGGR12,
    camera_pixel_format_SGBRG12,
    camera_pixel_format_SGRBG12,
    camera_pixel_format_SRGGB12,
    /* 12bit raw bayer packed, 6 bytes for every 4 pixels */
    camera_pixel_format_SBGGR12P,
    camera_pixel_format_SGBRG12P,
    camera_pixel_format_SGRBG12P,
    camera_pixel_format_SRGGB12P,
    camera_pixel_format_SBGGR14,
    camera_pixel_format_SGBRG14,
    camera_pixel_format_SGRBG14,
    camera_pixel_format_SRGGB14,
    /* 14bit raw bayer packed, 7 bytes for every 4 pixels */
    camera_pixel_format_SBGGR14P,
    camera_pixel_format_SGBRG14P,
    camera_pixel_format_SGRBG14P,
    camera_pixel_format_SRGGB14P,
    camera_pixel_format_SBGGR16,
    camera_pixel_format_SGBRG16,
    camera_pixel_format_SGRBG16,
    camera_pixel_format_SRGGB16,

    /* HSV formats */
    camera_pixel_format_HSV24,
    camera_pixel_format_HSV32,

    /* compressed formats */
    camera_pixel_format_MJPEG,
    camera_pixel_format_JPEG,
    camera_pixel_format_DV,
    camera_pixel_format_MPEG,
    camera_pixel_format_H264,
    camera_pixel_format_H264_NO_SC,
    camera_pixel_format_H264_MVC,
    camera_pixel_format_H263,
    camera_pixel_format_MPEG1,
    camera_pixel_format_MPEG2,
    camera_pixel_format_MPEG2_SLICE,
    camera_pixel_format_MPEG4,
    camera_pixel_format_XVID,
    camera_pixel_format_VC1_ANNEX_G,
    camera_pixel_format_VC1_ANNEX_L,
    camera_pixel_format_VP8,
    camera_pixel_format_VP8_FRAME,
    camera_pixel_format_VP9,
    camera_pixel_format_VP9_FRAME,
    camera_pixel_format_HEVC,
    camera_pixel_format_FWHT,
    camera_pixel_format_FWHT_STATELESS,
    camera_pixel_format_H264_SLICE,
    camera_pixel_format_HEVC_SLICE,
    camera_pixel_format_SPK,
    camera_pixel_format_RV30,
    camera_pixel_format_RV40,

    /*  Vendor-specific formats   */
    camera_pixel_format_CPIA1,
    camera_pixel_format_WNVA,
    camera_pixel_format_SN9C10X,
    camera_pixel_format_SN9C20X_I420,
    camera_pixel_format_PWC1,
    camera_pixel_format_PWC2,
    camera_pixel_format_ET61X251,
    camera_pixel_format_SPCA501,
    camera_pixel_format_SPCA505,
    camera_pixel_format_SPCA508,
    camera_pixel_format_SPCA561,
    camera_pixel_format_PAC207,
    camera_pixel_format_MR97310A,
    camera_pixel_format_JL2005BCD,
    camera_pixel_format_SN9C2028,
    camera_pixel_format_SQ905C,
    camera_pixel_format_PJPG,
    camera_pixel_format_OV511,
    camera_pixel_format_OV518,
    camera_pixel_format_STV0680,
    camera_pixel_format_TM6000,
    camera_pixel_format_CIT_YYVYUY,
    camera_pixel_format_KONICA420,
    camera_pixel_format_JPGL,
    camera_pixel_format_SE401,
    camera_pixel_format_S5C_UYVY_JPG,
    camera_pixel_format_Y8I,
    camera_pixel_format_Y12I,
    camera_pixel_format_Z16,
    camera_pixel_format_MT21C,
    camera_pixel_format_MM21,
    camera_pixel_format_INZI,
    camera_pixel_format_CNF4,
    camera_pixel_format_HI240,
    camera_pixel_format_QC08C,
    camera_pixel_format_QC10C,
    camera_pixel_format_AJPG,
} camera_pixel_format;

// Device Identification
typedef struct
{
    char *card;
    char *device_name;
    char *driver_info;
    char *version;
    char *manufacturer;
    char *bus;
    char *serial_number;
    uint32_t capabilities; // Represented as a bitmask

} camera_device_id;

// Capabilities
typedef struct
{
    bool can_capture;
    bool supports_streaming;
    bool supports_async_io;
    bool has_tuner;
    bool has_modulator;
    bool has_hardware_acceleration;
} camera_capabilities;

// Frame Rate Fraction
typedef struct {
    uint32_t frame_rate_numerator;
    uint32_t frame_rate_denominator;
} frame_rate_fraction;

// Format and Layout
typedef struct
{
    camera_pixel_format pixel_format;
    uint32_t width;
    uint32_t height;
    uint32_t fps_count;
    frame_rate_fraction* fps;
} camera_format;

// Controls
typedef struct
{
    int32_t brightness;
    int32_t contrast;
    int32_t saturation;
    int32_t hue;
    int32_t auto_white_balance;
    int32_t exposure_mode;
    int32_t gain;
} camera_controls;

// I/O Methods
typedef enum
{
    MMAP,
    USER_PTR,
    READ_WRITE
} camera_io_method;

// Buffers
typedef struct
{
    uint32_t buffer_size;
} camera_buffer_description;

// Streaming Parameters
typedef struct
{
    double frame_interval;
    double capture_time_per_frame;
} camera_streaming_params;

// Enumeration for Tuning Standard
typedef enum
{
    TUNING_NONE = 0,            // No tuning
    TUNING_NTSC,                // National Television System Committee
    TUNING_PAL,                 // Phase Alternating Line
    TUNING_SECAM,               // Sequential Couleur Avec Memoire
    TUNING_AUTO,                // Auto-detect or multi-standard
    TUNING_CUSTOM              // Custom or proprietary standard
} tuning_standard_t;

// Tuner Status
typedef struct
{
    bool signal_locked;
    bool is_capturing;
    tuning_standard_t tuning_standard;    // Represented as an enum or integer code
    double input_frequency;                // Frequency for tuning
} camera_status_tuning;

// Main Camera Description Device Structure
typedef struct
{
    camera_device_id device_id;
    camera_capabilities capabilities;
    uint32_t formats_count;
    camera_format* formats;
    camera_controls controls;
    camera_io_method io_method;
    uint32_t buffers_count;
    camera_buffer_description* buffers;
    camera_streaming_params streaming_params;
    uint32_t tuning_count;
    camera_status_tuning* tuning;
} camera_desc;

typedef void (*decoded_rgb_frame_buffer_callback)(const uint8_t *rgb_buffer, uint32_t width, uint32_t height);

/**
 * @brief Lists all available camera devices on the system.
 *
 * This function queries the system for all camera devices that are compatible
 * with the V4L2 API. It allocates an array of `camera_desc` structs, each
 * representing a different camera device, and returns this array along with
 * the total number of devices found.
 *
 * The caller is responsible for freeing the allocated `camera_desc` array
 * when it is no longer needed.
 *
 * @param[out] outCameras Pointer to an array of `camera_desc` structs. The array
 *                        is allocated by this function and should be freed by the caller.
 *                        Each element contains information about a camera device.
 * @param[out] outCamerasCount Pointer to a variable where the total number of found
 *                             cameras will be stored.
 *
 * @return 0 on success, or a negative error code on failure.
 *
 * Example usage:
 * @code{.c}
 * camera_desc* cameras = NULL;
 * size_t count;
 * int result = ;
 * if (list_all_camera_devices(&cameras, &count)) {
 *      // Handle error here
 * }
 * // Do something with the cameras array and count
 * // ...
 * // Don't forget to free the allocated array when done
 * free_camera_devices(cameras);
 * @endcode
 */
int list_all_camera_devices(camera_desc** outCameras, size_t* outCamerasCount);

/**
 * @brief Frees the memory allocated for an array of `camera_desc` structs.
 *
 * This function takes an array of `camera_desc` structs and its size, and
 * performs the necessary cleanup to free the allocated memory. This function
 * should be called to free the memory allocated by `list_all_camera_devices`.
 *
 * @param[in] cameras Pointer to the array of `camera_desc` structs to be freed.
 * @param[in] camerasCount The total number of elements in the `cameras` array.
 *
 * Example usage:
 * @code{.c}
 * camera_desc* cameras = NULL;
 * size_t count;
 * int result = ;
 * if (list_all_camera_devices(&cameras, &count)) {
 *      // Handle error here
 * }
 * // Do something with the cameras array and count
 * // ...
 * // Don't forget to free the allocated array when done
 * free_camera_devices(cameras);
 * @endcode
 */
void free_camera_devices(camera_desc* cameras, size_t camerasCount);

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
 * @brief Start capturing video using the V4L2 API and FFmpeg libraries.
 *
 * This function performs the following key operations:
 * - Initializes FFmpeg codecs for video decoding
 * - Sets up the video capture format and frame rate
 * - Manages buffers using mmap
 * - Handles video frame decoding and optional scaling
 * - Invokes the provided callback function upon successful frame decoding
 *
 * @param pathToCamera Path to the camera device (e.g., "/dev/video0").
 * @param width Desired width of the capture in pixels.
 * @param height Desired height of the capture in pixels.
 * @param fps Desired frames per second (fps) for video capture.
 * @param callback A function pointer to a callback function that will be invoked when a frame is successfully decoded and ready for further processing.
 * @param quit Atomic flag to indicate if capturing should stop; if set to non-zero, capturing stops.
 *
 * @return 0 on success, or a non-zero error code on failure.
 *
 * Error codes:
 * 1 - Failure due to memory allocation, codec initialization, device setup, or other internal issues.
 */
int start_capture(const char *pathToCamera, uint32_t width, uint32_t height, uint32_t fps, decoded_rgb_frame_buffer_callback callback, atomic_int *quit);

#endif