#define CAMERA_IMPLEMENTATION
#include "camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>

/**
 * @brief Print the capabilities of a webcam.
 *
 * @param fd File descriptor of the webcam.
 * @param width Desired width of the image in pixels.
 * @param height Desired height of the image in pixels.
 * @return 0 on success, or error code on failure.
 */
int print_caps_of_webcam(int fd, uint32_t width, uint32_t height)
{
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        fprintf(stderr,"Querying Capabilities");
        return 1;
    }

    printf("Driver: %s\n", cap.driver);
    printf("Card: %s\n", cap.card);
    printf("Bus: %s\n", cap.bus_info);
    printf("Version: %d.%d.%d\n", (cap.version >> 16) & 0xFF, (cap.version >> 8) & 0xFF, cap.version & 0xFF);
    printf("Capabilities: %08x\n", cap.capabilities);

    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        printf("Format: %s\n", fmtdesc.description);
        fmtdesc.index++;
    }

    struct v4l2_frmsizeenum frmsize;
    frmsize.pixel_format = V4L2_PIX_FMT_H264;  // replace with your pixel format
    frmsize.index = 0;

    while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            printf("Size: %dx%d\n", frmsize.discrete.width, frmsize.discrete.height);
        }
        frmsize.index++;
    }

    struct v4l2_frmivalenum frmival;
    frmival.index = 0;
    frmival.pixel_format = V4L2_PIX_FMT_H264;  // replace with your pixel format
    frmival.width = 1024;  // replace with your frame width
    frmival.height = 576;  // replace with your frame height

    while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
        if (frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
            printf("Frame rate: %d/%d\n", frmival.discrete.numerator, frmival.discrete.denominator);
        }
        frmival.index++;
    }
    return 0;
}

/**
 * @brief Clones a substring of the given C-string into a newly allocated string.
 *
 * This function allocates a new null-terminated string of size `size + 1` and
 * copies up to `size` characters from the source string `src` into it. The function
 * returns a pointer to the new string.
 *
 * The caller is responsible for freeing the allocated string using `free()`.
 *
 * @param[in] src  Pointer to the source C-string to be cloned.
 * @param[in] size Number of characters to be copied from the source string.
 *                 The function will allocate `size + 1` bytes to store these characters
 *                 along with a null-terminator.
 *
 * @return Pointer to the newly allocated string containing the cloned characters,
 *         or NULL if the memory allocation fails.
 *
 * @note The function uses `calloc()` to initialize the new string, so the extra bytes
 *       will be null-terminated.
 *
 * Example usage:
 * @code{.c}
 * const char* original = "Hello, world!";
 * char* clone = strclone(original, 5);  // clone will contain "Hello"
 * // Do something with the cloned string
 * // ...
 * // Don't forget to free the allocated string when done
 * free(clone);
 * @endcode
 */
static inline char* strclone(const char* src, size_t size)
{
    char* newstr = (char*)calloc(1, size + 1);
    strncpy(newstr, src, size);
    return newstr;
}

/**
 * @brief Reads the content of a file into a newly allocated C-string.
 *
 * This function opens a file in binary mode at the specified path, reads its
 * entire content, and stores it in a newly allocated null-terminated string.
 * The caller is responsible for freeing the allocated string using `free()`.
 *
 * @param[in] path The file path to read.
 *
 * @return A pointer to a newly allocated null-terminated string containing the
 *         file content, or NULL if the file couldn't be read or memory couldn't
 *         be allocated.
 *
 * @note The function reads the file in binary mode to determine its exact size.
 *       It null-terminates the read content to create a valid C-string.
 * 
 * Example usage:
 * @code{.c}
 * const char* file_path = "example.txt";
 * char* file_content = read_file_content(file_path);
 * if (file_content == NULL) {
 *     // Handle error here
 * } else {
 *     // Do something with the file content
 *     // ...
 *     // Don't forget to free the allocated string when done
 *     free(file_content);
 * }
 * @endcode
 */
static inline char* read_file_content(const char* path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size_t file_length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = malloc(file_length + 1);
    if (buffer == NULL) {
        fclose(fp);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, file_length, fp);
    if (read_size != file_length) {
        free(buffer);
        fclose(fp);
        return NULL;
    }

    buffer[file_length] = '\0';

    fclose(fp);

    return buffer;
}

enum camera_pixel_format v4l2_to_camera_pixel_format(uint32_t v4l2_pixfmt)
{
    switch (v4l2_pixfmt)
    {
        case V4L2_PIX_FMT_RGB332:
            return camera_pixel_format_RGB332;
        case V4L2_PIX_FMT_RGB444:
            return camera_pixel_format_RGB444;
        case V4L2_PIX_FMT_ARGB444:
            return camera_pixel_format_ARGB444;
        case V4L2_PIX_FMT_XRGB444:
            return camera_pixel_format_XRGB444;
        case V4L2_PIX_FMT_RGBA444:
            return camera_pixel_format_RGBA444;
        case V4L2_PIX_FMT_RGBX444:
            return camera_pixel_format_RGBX444;
        case V4L2_PIX_FMT_ABGR444:
            return camera_pixel_format_ABGR444;
        case V4L2_PIX_FMT_XBGR444:
            return camera_pixel_format_XBGR444;
        case V4L2_PIX_FMT_BGRA444:
            return camera_pixel_format_BGRA444;
        case V4L2_PIX_FMT_BGRX444:
            return camera_pixel_format_BGRX444;
        case V4L2_PIX_FMT_RGB555:
            return camera_pixel_format_RGB555;
        case V4L2_PIX_FMT_ARGB555:
            return camera_pixel_format_ARGB555;
        case V4L2_PIX_FMT_XRGB555:
            return camera_pixel_format_XRGB555;
        case V4L2_PIX_FMT_RGBA555:
            return camera_pixel_format_RGBA555;
        case V4L2_PIX_FMT_RGBX555:
            return camera_pixel_format_RGBX555;
        case V4L2_PIX_FMT_ABGR555:
            return camera_pixel_format_ABGR555;
        case V4L2_PIX_FMT_XBGR555:
            return camera_pixel_format_XBGR555;
        case V4L2_PIX_FMT_BGRA555:
            return camera_pixel_format_BGRA555;
        case V4L2_PIX_FMT_BGRX555:
            return camera_pixel_format_BGRX555;
        case V4L2_PIX_FMT_RGB565:
            return camera_pixel_format_RGB565;
        case V4L2_PIX_FMT_RGB555X:
            return camera_pixel_format_RGB555X;
        case V4L2_PIX_FMT_ARGB555X:
            return camera_pixel_format_ARGB555X;
        case V4L2_PIX_FMT_XRGB555X:
            return camera_pixel_format_XRGB555X;
        case V4L2_PIX_FMT_RGB565X:
            return camera_pixel_format_RGB565X;
        case V4L2_PIX_FMT_BGR666:
            return camera_pixel_format_BGR666;
        case V4L2_PIX_FMT_BGR24:
            return camera_pixel_format_BGR24;
        case V4L2_PIX_FMT_RGB24:
            return camera_pixel_format_RGB24;
        case V4L2_PIX_FMT_BGR32:
            return camera_pixel_format_BGR32;
        case V4L2_PIX_FMT_ABGR32:
            return camera_pixel_format_ABGR32;
        case V4L2_PIX_FMT_XBGR32:
            return camera_pixel_format_XBGR32;
        case V4L2_PIX_FMT_BGRA32:
            return camera_pixel_format_BGRA32;
        case V4L2_PIX_FMT_BGRX32:
            return camera_pixel_format_BGRX32;
        case V4L2_PIX_FMT_RGB32:
            return camera_pixel_format_RGB32;
        case V4L2_PIX_FMT_RGBA32:
            return camera_pixel_format_RGBA32;
        case V4L2_PIX_FMT_RGBX32:
            return camera_pixel_format_RGBX32;
        case V4L2_PIX_FMT_ARGB32:
            return camera_pixel_format_ARGB32;
        case V4L2_PIX_FMT_XRGB32:
            return camera_pixel_format_XRGB32;
        case V4L2_PIX_FMT_RGBX1010102:
            return camera_pixel_format_RGBX1010102;
        case V4L2_PIX_FMT_RGBA1010102:
            return camera_pixel_format_RGBA1010102;
        case V4L2_PIX_FMT_ARGB2101010:
            return camera_pixel_format_ARGB2101010;
        case V4L2_PIX_FMT_BGR48_12:
            return camera_pixel_format_BGR48_12;
        case V4L2_PIX_FMT_ABGR64_12:
            return camera_pixel_format_ABGR64_12;
        case V4L2_PIX_FMT_GREY:
            return camera_pixel_format_GREY;
        case V4L2_PIX_FMT_Y4:
            return camera_pixel_format_Y4;
        case V4L2_PIX_FMT_Y6:
            return camera_pixel_format_Y6;
        case V4L2_PIX_FMT_Y10:
            return camera_pixel_format_Y10;
        case V4L2_PIX_FMT_Y12:
            return camera_pixel_format_Y12;
        case V4L2_PIX_FMT_Y012:
            return camera_pixel_format_Y012;
        case V4L2_PIX_FMT_Y14:
            return camera_pixel_format_Y14;
        case V4L2_PIX_FMT_Y16:
            return camera_pixel_format_Y16;
        case V4L2_PIX_FMT_Y16_BE:
            return camera_pixel_format_Y16_BE;
        case V4L2_PIX_FMT_Y10BPACK:
            return camera_pixel_format_Y10BPACK;
        case V4L2_PIX_FMT_Y10P:
            return camera_pixel_format_Y10P;
        case V4L2_PIX_FMT_IPU3_Y10:
            return camera_pixel_format_IPU3_Y10;
        case V4L2_PIX_FMT_PAL8:
            return camera_pixel_format_PAL8;
        case V4L2_PIX_FMT_UV8:
            return camera_pixel_format_UV8;
        case V4L2_PIX_FMT_YUYV:
            return camera_pixel_format_YUYV;
        case V4L2_PIX_FMT_YYUV:
            return camera_pixel_format_YYUV;
        case V4L2_PIX_FMT_YVYU:
            return camera_pixel_format_YVYU;
        case V4L2_PIX_FMT_UYVY:
            return camera_pixel_format_UYVY;
        case V4L2_PIX_FMT_VYUY:
            return camera_pixel_format_VYUY;
        case V4L2_PIX_FMT_Y41P:
            return camera_pixel_format_Y41P;
        case V4L2_PIX_FMT_YUV444:
            return camera_pixel_format_YUV444;
        case V4L2_PIX_FMT_YUV555:
            return camera_pixel_format_YUV555;
        case V4L2_PIX_FMT_YUV565:
            return camera_pixel_format_YUV565;
        case V4L2_PIX_FMT_YUV24:
            return camera_pixel_format_YUV24;
        case V4L2_PIX_FMT_YUV32:
            return camera_pixel_format_YUV32;
        case V4L2_PIX_FMT_AYUV32:
            return camera_pixel_format_AYUV32;
        case V4L2_PIX_FMT_XYUV32:
            return camera_pixel_format_XYUV32;
        case V4L2_PIX_FMT_VUYA32:
            return camera_pixel_format_VUYA32;
        case V4L2_PIX_FMT_VUYX32:
            return camera_pixel_format_VUYX32;
        case V4L2_PIX_FMT_YUVA32:
            return camera_pixel_format_YUVA32;
        case V4L2_PIX_FMT_YUVX32:
            return camera_pixel_format_YUVX32;
        case V4L2_PIX_FMT_M420:
            return camera_pixel_format_M420;
        case V4L2_PIX_FMT_YUV48_12:
            return camera_pixel_format_YUV48_12;
        case V4L2_PIX_FMT_Y210:
            return camera_pixel_format_Y210;
        case V4L2_PIX_FMT_Y212:
            return camera_pixel_format_Y212;
        case V4L2_PIX_FMT_Y216:
            return camera_pixel_format_Y216;
        case V4L2_PIX_FMT_NV12:
            return camera_pixel_format_NV12;
        case V4L2_PIX_FMT_NV21:
            return camera_pixel_format_NV21;
        case V4L2_PIX_FMT_NV16:
            return camera_pixel_format_NV16;
        case V4L2_PIX_FMT_NV61:
            return camera_pixel_format_NV61;
        case V4L2_PIX_FMT_NV24:
            return camera_pixel_format_NV24;
        case V4L2_PIX_FMT_NV42:
            return camera_pixel_format_NV42;
        case V4L2_PIX_FMT_P010:
            return camera_pixel_format_P010;
        case V4L2_PIX_FMT_P012:
            return camera_pixel_format_P012;
        case V4L2_PIX_FMT_NV12M:
            return camera_pixel_format_NV12M;
        case V4L2_PIX_FMT_NV21M:
            return camera_pixel_format_NV21M;
        case V4L2_PIX_FMT_NV16M:
            return camera_pixel_format_NV16M;
        case V4L2_PIX_FMT_NV61M:
            return camera_pixel_format_NV61M;
        case V4L2_PIX_FMT_P012M:
            return camera_pixel_format_P012M;
        case V4L2_PIX_FMT_YUV410:
            return camera_pixel_format_YUV410;
        case V4L2_PIX_FMT_YVU410:
            return camera_pixel_format_YVU410;
        case V4L2_PIX_FMT_YUV411P:
            return camera_pixel_format_YUV411P;
        case V4L2_PIX_FMT_YUV420:
            return camera_pixel_format_YUV420;
        case V4L2_PIX_FMT_YVU420:
            return camera_pixel_format_YVU420;
        case V4L2_PIX_FMT_YUV422P:
            return camera_pixel_format_YUV422P;
        case V4L2_PIX_FMT_YUV420M:
            return camera_pixel_format_YUV420M;
        case V4L2_PIX_FMT_YVU420M:
            return camera_pixel_format_YVU420M;
        case V4L2_PIX_FMT_YUV422M:
            return camera_pixel_format_YUV422M;
        case V4L2_PIX_FMT_YVU422M:
            return camera_pixel_format_YVU422M;
        case V4L2_PIX_FMT_YUV444M:
            return camera_pixel_format_YUV444M;
        case V4L2_PIX_FMT_YVU444M:
            return camera_pixel_format_YVU444M;
        case V4L2_PIX_FMT_NV12_4L4:
            return camera_pixel_format_NV12_4L4;
        case V4L2_PIX_FMT_NV12_16L16:
            return camera_pixel_format_NV12_16L16;
        case V4L2_PIX_FMT_NV12_32L32:
            return camera_pixel_format_NV12_32L32;
        case V4L2_PIX_FMT_P010_4L4:
            return camera_pixel_format_P010_4L4;
        case V4L2_PIX_FMT_NV12_8L128:
            return camera_pixel_format_NV12_8L128;
        case V4L2_PIX_FMT_NV12_10BE_8L128:
            return camera_pixel_format_NV12_10BE_8L128;
        case V4L2_PIX_FMT_NV12MT:
            return camera_pixel_format_NV12MT;
        case V4L2_PIX_FMT_NV12MT_16X16:
            return camera_pixel_format_NV12MT_16X16;
        case V4L2_PIX_FMT_NV12M_8L128:
            return camera_pixel_format_NV12M_8L128;
        case V4L2_PIX_FMT_NV12M_10BE_8L128:
            return camera_pixel_format_NV12M_10BE_8L128;
        case V4L2_PIX_FMT_SBGGR8:
            return camera_pixel_format_SBGGR8;
        case V4L2_PIX_FMT_SGBRG8:
            return camera_pixel_format_SGBRG8;
        case V4L2_PIX_FMT_SGRBG8:
            return camera_pixel_format_SGRBG8;
        case V4L2_PIX_FMT_SRGGB8:
            return camera_pixel_format_SRGGB8;
        case V4L2_PIX_FMT_SBGGR10:
            return camera_pixel_format_SBGGR10;
        case V4L2_PIX_FMT_SGBRG10:
            return camera_pixel_format_SGBRG10;
        case V4L2_PIX_FMT_SGRBG10:
            return camera_pixel_format_SGRBG10;
        case V4L2_PIX_FMT_SRGGB10:
            return camera_pixel_format_SRGGB10;
        case V4L2_PIX_FMT_SBGGR10P:
            return camera_pixel_format_SBGGR10P;
        case V4L2_PIX_FMT_SGBRG10P:
            return camera_pixel_format_SGBRG10P;
        case V4L2_PIX_FMT_SGRBG10P:
            return camera_pixel_format_SGRBG10P;
        case V4L2_PIX_FMT_SRGGB10P:
            return camera_pixel_format_SRGGB10P;
        case V4L2_PIX_FMT_SBGGR10ALAW8:
            return camera_pixel_format_SBGGR10ALAW8;
        case V4L2_PIX_FMT_SGBRG10ALAW8:
            return camera_pixel_format_SGBRG10ALAW8;
        case V4L2_PIX_FMT_SGRBG10ALAW8:
            return camera_pixel_format_SGRBG10ALAW8;
        case V4L2_PIX_FMT_SRGGB10ALAW8:
            return camera_pixel_format_SRGGB10ALAW8;
        case V4L2_PIX_FMT_SBGGR10DPCM8:
            return camera_pixel_format_SBGGR10DPCM8;
        case V4L2_PIX_FMT_SGBRG10DPCM8:
            return camera_pixel_format_SGBRG10DPCM8;
        case V4L2_PIX_FMT_SGRBG10DPCM8:
            return camera_pixel_format_SGRBG10DPCM8;
        case V4L2_PIX_FMT_SRGGB10DPCM8:
            return camera_pixel_format_SRGGB10DPCM8;
        case V4L2_PIX_FMT_SBGGR12:
            return camera_pixel_format_SBGGR12;
        case V4L2_PIX_FMT_SGBRG12:
            return camera_pixel_format_SGBRG12;
        case V4L2_PIX_FMT_SGRBG12:
            return camera_pixel_format_SGRBG12;
        case V4L2_PIX_FMT_SRGGB12:
            return camera_pixel_format_SRGGB12;
        case V4L2_PIX_FMT_SBGGR12P:
            return camera_pixel_format_SBGGR12P;
        case V4L2_PIX_FMT_SGBRG12P:
            return camera_pixel_format_SGBRG12P;
        case V4L2_PIX_FMT_SGRBG12P:
            return camera_pixel_format_SGRBG12P;
        case V4L2_PIX_FMT_SRGGB12P:
            return camera_pixel_format_SRGGB12P;
        case V4L2_PIX_FMT_SBGGR14:
            return camera_pixel_format_SBGGR14;
        case V4L2_PIX_FMT_SGBRG14:
            return camera_pixel_format_SGBRG14;
        case V4L2_PIX_FMT_SGRBG14:
            return camera_pixel_format_SGRBG14;
        case V4L2_PIX_FMT_SRGGB14:
            return camera_pixel_format_SRGGB14;
        case V4L2_PIX_FMT_SBGGR14P:
            return camera_pixel_format_SBGGR14P;
        case V4L2_PIX_FMT_SGBRG14P:
            return camera_pixel_format_SGBRG14P;
        case V4L2_PIX_FMT_SGRBG14P:
            return camera_pixel_format_SGRBG14P;
        case V4L2_PIX_FMT_SRGGB14P:
            return camera_pixel_format_SRGGB14P;
        case V4L2_PIX_FMT_SBGGR16:
            return camera_pixel_format_SBGGR16;
        case V4L2_PIX_FMT_SGBRG16:
            return camera_pixel_format_SGBRG16;
        case V4L2_PIX_FMT_SGRBG16:
            return camera_pixel_format_SGRBG16;
        case V4L2_PIX_FMT_SRGGB16:
            return camera_pixel_format_SRGGB16;
        case V4L2_PIX_FMT_HSV24:
            return camera_pixel_format_HSV24;
        case V4L2_PIX_FMT_HSV32:
            return camera_pixel_format_HSV32;
        case V4L2_PIX_FMT_MJPEG:
            return camera_pixel_format_MJPEG;
        case V4L2_PIX_FMT_JPEG:
            return camera_pixel_format_JPEG;
        case V4L2_PIX_FMT_DV:
            return camera_pixel_format_DV;
        case V4L2_PIX_FMT_MPEG:
            return camera_pixel_format_MPEG;
        case V4L2_PIX_FMT_H264:
            return camera_pixel_format_H264;
        case V4L2_PIX_FMT_H264_NO_SC:
            return camera_pixel_format_H264_NO_SC;
        case V4L2_PIX_FMT_H264_MVC:
            return camera_pixel_format_H264_MVC;
        case V4L2_PIX_FMT_H263:
            return camera_pixel_format_H263;
        case V4L2_PIX_FMT_MPEG1:
            return camera_pixel_format_MPEG1;
        case V4L2_PIX_FMT_MPEG2:
            return camera_pixel_format_MPEG2;
        case V4L2_PIX_FMT_MPEG2_SLICE:
            return camera_pixel_format_MPEG2_SLICE;
        case V4L2_PIX_FMT_MPEG4:
            return camera_pixel_format_MPEG4;
        case V4L2_PIX_FMT_XVID:
            return camera_pixel_format_XVID;
        case V4L2_PIX_FMT_VC1_ANNEX_G:
            return camera_pixel_format_VC1_ANNEX_G;
        case V4L2_PIX_FMT_VC1_ANNEX_L:
            return camera_pixel_format_VC1_ANNEX_L;
        case V4L2_PIX_FMT_VP8:
            return camera_pixel_format_VP8;
        case V4L2_PIX_FMT_VP8_FRAME:
            return camera_pixel_format_VP8_FRAME;
        case V4L2_PIX_FMT_VP9:
            return camera_pixel_format_VP9;
        case V4L2_PIX_FMT_VP9_FRAME:
            return camera_pixel_format_VP9_FRAME;
        case V4L2_PIX_FMT_HEVC:
            return camera_pixel_format_HEVC;
        case V4L2_PIX_FMT_FWHT:
            return camera_pixel_format_FWHT;
        case V4L2_PIX_FMT_FWHT_STATELESS:
            return camera_pixel_format_FWHT_STATELESS;
        case V4L2_PIX_FMT_H264_SLICE:
            return camera_pixel_format_H264_SLICE;
        case V4L2_PIX_FMT_HEVC_SLICE:
            return camera_pixel_format_HEVC_SLICE;
        case V4L2_PIX_FMT_SPK:
            return camera_pixel_format_SPK;
        case V4L2_PIX_FMT_RV30:
            return camera_pixel_format_RV30;
        case V4L2_PIX_FMT_RV40:
            return camera_pixel_format_RV40;
        case V4L2_PIX_FMT_CPIA1:
            return camera_pixel_format_CPIA1;
        case V4L2_PIX_FMT_WNVA:
            return camera_pixel_format_WNVA;
        case V4L2_PIX_FMT_SN9C10X:
            return camera_pixel_format_SN9C10X;
        case V4L2_PIX_FMT_SN9C20X_I420:
            return camera_pixel_format_SN9C20X_I420;
        case V4L2_PIX_FMT_PWC1:
            return camera_pixel_format_PWC1;
        case V4L2_PIX_FMT_PWC2:
            return camera_pixel_format_PWC2;
        case V4L2_PIX_FMT_ET61X251:
            return camera_pixel_format_ET61X251;
        case V4L2_PIX_FMT_SPCA501:
            return camera_pixel_format_SPCA501;
        case V4L2_PIX_FMT_SPCA505:
            return camera_pixel_format_SPCA505;
        case V4L2_PIX_FMT_SPCA508:
            return camera_pixel_format_SPCA508;
        case V4L2_PIX_FMT_SPCA561:
            return camera_pixel_format_SPCA561;
        case V4L2_PIX_FMT_PAC207:
            return camera_pixel_format_PAC207;
        case V4L2_PIX_FMT_MR97310A:
            return camera_pixel_format_MR97310A;
        case V4L2_PIX_FMT_JL2005BCD:
            return camera_pixel_format_JL2005BCD;
        case V4L2_PIX_FMT_SN9C2028:
            return camera_pixel_format_SN9C2028;
        case V4L2_PIX_FMT_SQ905C:
            return camera_pixel_format_SQ905C;
        case V4L2_PIX_FMT_PJPG:
            return camera_pixel_format_PJPG;
        case V4L2_PIX_FMT_OV511:
            return camera_pixel_format_OV511;
        case V4L2_PIX_FMT_OV518:
            return camera_pixel_format_OV518;
        case V4L2_PIX_FMT_STV0680:
            return camera_pixel_format_STV0680;
        case V4L2_PIX_FMT_TM6000:
            return camera_pixel_format_TM6000;
        case V4L2_PIX_FMT_CIT_YYVYUY:
            return camera_pixel_format_CIT_YYVYUY;
        case V4L2_PIX_FMT_KONICA420:
            return camera_pixel_format_KONICA420;
        case V4L2_PIX_FMT_JPGL:
            return camera_pixel_format_JPGL;
        case V4L2_PIX_FMT_SE401:
            return camera_pixel_format_SE401;
        case V4L2_PIX_FMT_S5C_UYVY_JPG:
            return camera_pixel_format_S5C_UYVY_JPG;
        case V4L2_PIX_FMT_Y8I:
            return camera_pixel_format_Y8I;
        case V4L2_PIX_FMT_Y12I:
            return camera_pixel_format_Y12I;
        case V4L2_PIX_FMT_Z16:
            return camera_pixel_format_Z16;
        case V4L2_PIX_FMT_MT21C:
            return camera_pixel_format_MT21C;
        case V4L2_PIX_FMT_MM21:
            return camera_pixel_format_MM21;
        case V4L2_PIX_FMT_INZI:
            return camera_pixel_format_INZI;
        case V4L2_PIX_FMT_CNF4:
            return camera_pixel_format_CNF4;
        case V4L2_PIX_FMT_HI240:
            return camera_pixel_format_HI240;
        case V4L2_PIX_FMT_QC08C:
            return camera_pixel_format_QC08C;
        case V4L2_PIX_FMT_QC10C:
            return camera_pixel_format_QC10C;
        case V4L2_PIX_FMT_AJPG:
            return camera_pixel_format_AJPG;
        default:
            return camera_pixel_format_unknown;
    }
}

void free_camera_desc(camera_desc* camera)
{
    if (camera == NULL)
    {
        return;
    }

    if (camera->device_id.driver_info)
        free(camera->device_id.driver_info);
    if (camera->device_id.bus)
        free(camera->device_id.bus);
    if (camera->device_id.version)
        free(camera->device_id.version);
    if (camera->device_id.card)
        free(camera->device_id.card);
    if (camera->device_id.device_name)
        free(camera->device_id.device_name);
    if (camera->device_id.manufacturer)
        free(camera->device_id.manufacturer);
    if (camera->device_id.serial_number)
        free(camera->device_id.serial_number);
    if (camera->buffers)
        free(camera->buffers);
    free(camera);
}

static inline camera_desc* get_camera_device_desc(int fd, const char* devName)
{
    if (fd < 0)
    {
        fprintf(stderr, "Invalid file descriptor for get_camera_device_desc function!\n");
        return NULL;
    }

    camera_desc* camera = (camera_desc*)calloc(1, sizeof(camera_desc));
    if (camera == NULL)
    {
        fprintf(stderr, "Failed to allocate camera_desc!\n");
        return NULL;
    }

    // Obtain device informations
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        fprintf(stderr,"Failed to query capabilities\n");
        free(camera);
        return NULL;
    }

    camera->device_id.driver_info = strclone(cap.driver, sizeof(cap.driver));
    camera->device_id.bus = strclone(cap.bus_info, sizeof(cap.bus_info));
    camera->device_id.version = calloc(1, 13);
    snprintf(camera->device_id.version, 13, "%d.%d.%d", (cap.version >> 16) & 0xFF, (cap.version >> 8) & 0xFF, cap.version & 0xFF);
    camera->device_id.capabilities = (uint32_t)cap.capabilities;
    camera->device_id.card = strclone(cap.card, sizeof(cap.card));
    camera->device_id.device_name = strclone(cap.card, sizeof(cap.card));
    char pathToTry[512];
    snprintf(pathToTry, 512, "/sys/class/video4linux/%s/device/manufacturer", devName);
    camera->device_id.manufacturer = read_file_content(pathToTry);
    snprintf(pathToTry, 512, "/sys/class/video4linux/%s/device/serial", devName);
    camera->device_id.serial_number = read_file_content(pathToTry);

    struct v4l2_requestbuffers req;
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        fprintf(stderr, "Failed to request buffers from webcam drivers\n");
        free_camera_desc(camera);
        return NULL;
    }


    camera->buffers_count = req.count;
    camera->buffers = (camera_buffer_description*)malloc(sizeof(camera_buffer_description) * req.count);
    if (camera->buffers == NULL)
    {
        fprintf(stderr, "Failed to allocate camera buffer descriptions!\n");
        free_camera_desc(camera);
        return NULL;
    }

    // Query each buffer to get its size
    for (unsigned int i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            fprintf(stderr, "Failed to query buffer\n");
            free_camera_desc(camera);
            return NULL;
        }

        camera->buffers[i].buffer_size = buf.length;
    }

    // Free up allocated buffer from driver
    struct v4l2_requestbuffers req_free;
    req_free.count = 0;  // Setting count to 0 will deallocate all buffers
    req_free.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req_free.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req_free) == -1) {
        fprintf(stderr, "Failed to free buffers from webcam drivers\n");
        free_camera_desc(camera);
        return NULL;
    }

        struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        perror("Failed to query capabilities");
        close(fd);
        return 1;
    }

    camera->capabilities.can_capture = (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) != 0;
    camera->capabilities.supports_streaming = (cap.capabilities & V4L2_CAP_STREAMING) != 0;
    camera->capabilities.supports_async_io = (cap.capabilities & V4L2_CAP_ASYNCIO) != 0;
    camera->capabilities.has_tuner = (cap.capabilities & V4L2_CAP_TUNER) != 0;
    camera->capabilities.has_modulator = (cap.capabilities & V4L2_CAP_MODULATOR) != 0;
    camera->capabilities.has_hardware_acceleration = (cap.capabilities & V4L2_CAP_HW_FREQ_SEEK) != 0;

    // First, count the number of supported formats
    uint32_t format_count = 0;
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        format_count++;
        fmtdesc.index++;
    }

    // Allocate the formats array
    camera->formats_count = format_count;
    camera->formats = (camera_format*)calloc(format_count, sizeof(camera_format));

    // Now populate the formats array
    fmtdesc.index = 0;
    for (uint32_t i = 0; i < format_count; ++i) {
        ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc);

        // Fill in pixel format here, if needed

        // Enumerate frame sizes
        struct v4l2_frmsizeenum frmsize;
        frmsize.pixel_format = fmtdesc.pixelformat;
        frmsize.index = 0;
        if (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0) {
            camera->formats[i].width = frmsize.discrete.width;
            camera->formats[i].height = frmsize.discrete.height;

            // Enumerate frame intervals (frame rates)
            uint32_t fps_count = 0;
            struct v4l2_frmivalenum frmival;
            frmival.pixel_format = fmtdesc.pixelformat;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
            frmival.index = 0;
            while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
                fps_count++;
                frmival.index++;
            }

            camera->formats[i].fps_count = fps_count;
            camera->formats[i].fps = (frame_rate_fraction*)calloc(fps_count, sizeof(frame_rate_fraction));

            frmival.index = 0;
            for (uint32_t j = 0; j < fps_count; ++j) {
                ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival);
                camera->formats[i].fps[j].frame_rate_numerator = frmival.discrete.numerator;
                camera->formats[i].fps[j].frame_rate_denominator = frmival.discrete.denominator;
                frmival.index++;
            }
        }

        fmtdesc.index++;
    }
}

int list_all_camera_devices(camera_desc** outCameras, size_t* outCamerasCount)
{
    int fd;
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir("/dev/")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, "video") == ent->d_name) {  // Filtering for names that start with "video"
                char dev_name[261];
                snprintf(dev_name, sizeof(dev_name), "/dev/%s", ent->d_name);

                if ((fd = open(dev_name, O_RDWR)) == -1) {
                    fprintf(stderr, "Unable to open video device\n");
                    continue;  // Skip if the device couldn't be opened
                }

                
                close(fd);
            }
        }
        closedir(dir);
    } else {
        fprintf(stderr, "Could not open directory\n");
        return -1;
    }

    return 0;
}

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
static inline int decode_packet(AVCodecContext *codec_context, struct SwsContext* sws_ctx, AVPacket *packet, AVFrame *frame, AVFrame *rgb_frame, unsigned char *rgb_buffer, int width, int height) {
    int response = avcodec_send_packet(codec_context, packet);
    if (response < 0) {
        fprintf(stderr,"Error while sending a packet to the decoder");
        return response;
    }
    while (response >= 0) {
        response = avcodec_receive_frame(codec_context, frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            fprintf(stderr,"Error while receiving a frame from the decoder");
            return response;
        }

        if (response >= 0) {
            // Perform the scaling.
            sws_scale(sws_ctx, (const uint8_t* const*) frame->data, frame->linesize, 0, height, rgb_frame->data, rgb_frame->linesize);

            // Now rgb_frame contains the image in RGB format. You can copy it to your buffer.
            int numBytes = av_image_get_buffer_size(rgb_frame->format, rgb_frame->width, rgb_frame->height, 1);
            av_image_copy_to_buffer(rgb_buffer, numBytes, (const uint8_t * const *)rgb_frame->data, rgb_frame->linesize, rgb_frame->format, rgb_frame->width, rgb_frame->height, 1);
        }
    }
    return 0;
}

static inline int initialize_avcodec(AVCodec** outVidCodec, AVCodecContext** outVidCodecContext)
{
    *outVidCodec = (AVCodec*) avcodec_find_decoder(AV_CODEC_ID_H264);
    if (*outVidCodec == NULL) {
        fprintf(stderr,"Unable to find codec!\n");
        return 1;
    }

    *outVidCodecContext = avcodec_alloc_context3(*outVidCodec);
    if (avcodec_open2(*outVidCodecContext, *outVidCodec, NULL) < 0) {
        fprintf(stderr,"Unable to open codec!\n");
        return 1;
    }
    return 0;
}

static inline int initialize_swscale(uint32_t width, uint32_t height, AVCodecContext* vidcodecContext, struct SwsContext** outSwsContext)
{
    *outSwsContext = sws_getContext(width, height, vidcodecContext->pix_fmt,
                                        width, height, AV_PIX_FMT_RGB24,
                                        SWS_BILINEAR, NULL, NULL, NULL);

    if (*outSwsContext == NULL) {
        fprintf(stderr,"Could not initialize the conversion context\n");
        return 1;
    }
    return 0;
}

static inline int set_v4l2_videocapture_format_fps(int fd, uint32_t width, uint32_t height, uint32_t fps)
{
    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    //format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    //format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        fprintf(stderr,"Fail to set Pixel Format\n");
        return 1;
    }

    struct v4l2_streamparm setfps;
    memset(&setfps, 0, sizeof(struct v4l2_streamparm));
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;
    setfps.parm.capture.timeperframe.denominator = fps;

    if (ioctl(fd, VIDIOC_S_PARM, &setfps) == -1) {
        fprintf(stderr,"Fail to set frame rate\n");
        return 1;
    }
    return 0;
}

int start_capture(const char* pathToCamera, uint32_t width, uint32_t height, uint32_t fps, decoded_rgb_frame_buffer_callback callback, atomic_int *quit)
{
    int ret = 0;

    if (callback == NULL)
    {
        fprintf(stderr, "Callback for processing rgb frame buffer cannot be null!\n");
        return 1;
    }

    if (quit == NULL)
    {
        fprintf(stderr, "Atomic quit integer reference cannot be null!\n");
        return 1;
    }

    // Initialize FFmpeg and V4L2 related variables
    AVCodec *vidCodec = NULL;
    AVCodecContext *vidcodec_context = NULL;
    struct SwsContext* sws_ctx = NULL;
    int fd = -1;
    struct v4l2_requestbuffers req = {0};
    struct v4l2_buffer buf = {0};
    unsigned char* buffer = NULL;
    AVPacket *packet = NULL;
    AVFrame *frame = NULL;
    AVFrame *rgb_frame = NULL;

    // Allocate RGB buffer
    unsigned char* rgb_buffer = (unsigned char*) malloc(width * height * 3);
    if (!rgb_buffer)
    {
        fprintf(stderr, "Failed to allocate rgb_buffer\n");
        ret = 1;
        goto fail;
    }

    // Initialize video codec
    if (initialize_avcodec(&vidCodec, &vidcodec_context))
    {
        fprintf(stderr,"Failed to initialize avcodec!\n");
        ret = 1;
        goto cleanup_avcodec;
    }

    // Set pixel format
    vidcodec_context->pix_fmt = AV_PIX_FMT_YUV420P;

    // Initialize scaling context
    if (initialize_swscale(width, height, vidcodec_context, &sws_ctx))
    {
        fprintf(stderr,"Failed to initialize swscale\n");
        ret = 1;
        goto cleanup_swscale;
    }

    // Open video device
    fd = open(pathToCamera, O_RDWR);
    if (fd == -1) {
        fprintf(stderr,"Opening video device\n");
        ret = 1;
        goto cleanup_fd;
    }

    // Set video format and framerate
    if (set_v4l2_videocapture_format_fps(fd, width, height, fps))
    {
        fprintf(stderr,"Fail to set video capture format and fps\n");
        ret = 1;
        goto cleanup_fd;
    }

    // Request buffer for mmap
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        fprintf(stderr,"Fail to request buffer\n");
        ret = 1;
        goto cleanup_fd;
    }

    // Query buffer properties
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
        fprintf(stderr,"Fail to query buffer\n");
        ret = 1;
        goto cleanup_fd;
    }

    // Memory map the buffer
    buffer = (unsigned char*)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer == MAP_FAILED)
    {
        fprintf(stderr, "Failed to allocate mmap for buffer\n");
        ret = 1;
        goto cleanup_fd;
    }

    // Initialize AVPacket
    //av_init_packet(&packet);

    // Allocate YUV and RGB frames
    frame = av_frame_alloc();
    if (!frame)
    {
        fprintf(stderr,"Could not allocate frame\n");
        ret = 1;
        goto cleanup_frame;
    }

    rgb_frame = av_frame_alloc();
    if (!rgb_frame) {
        fprintf(stderr,"Could not allocate RGB frame\n");
        ret = 1;
        goto cleanup_rgb_frame;
    }

    // Set RGB frame properties
    rgb_frame->format = AV_PIX_FMT_RGB24;
    rgb_frame->width  = width;
    rgb_frame->height = height;

    av_image_alloc(rgb_frame->data, rgb_frame->linesize, width, height, AV_PIX_FMT_RGB24, 1);
    while (!*quit){

        // Queue buffer for capture
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            fprintf(stderr,"Fail to queue buffer\n");
            ret = 1;
            goto cleanup_rgb_frame;
        }

        // Start capturing
        if (ioctl(fd, VIDIOC_STREAMON, &buf.type) == -1) {
            fprintf(stderr,"Fail to start capture\n");
            ret = 1;
            goto cleanup_rgb_frame;
        }
        
        // Dequeue the filled buffer
        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            fprintf(stderr,"Fail to retrieve frame\n");
            ret = 1;
            goto cleanup_rgb_frame;
        }

        // Decode the received packet
        packet->data = buffer;
        packet->size = buf.bytesused;
        if (decode_packet(vidcodec_context, sws_ctx, packet, frame, rgb_frame, rgb_buffer, width, height))
        {
            fprintf(stderr,"Failed to decode packet!\n");
            ret = 1;
            goto cleanup_rgb_frame;
        }

        // Run the callback for further processing
        callback(rgb_buffer, width, height);
    }
cleanup_rgb_frame:
    if (rgb_frame) {
        av_freep(&rgb_frame->data[0]);
        av_frame_free(&rgb_frame);
        rgb_frame = NULL;
    }

cleanup_frame:
    if (frame) {
        av_frame_free(&frame);
        frame = NULL;
    }

    av_packet_free(&packet);
    if (buffer != NULL && buffer != MAP_FAILED) {
        if (munmap(buffer, buf.length) == -1) {
            fprintf(stderr, "Failed to unmap memory\n");
        }
        buffer = NULL;
    }

cleanup_fd:
    if (fd != -1) {
        close(fd);
    }

cleanup_swscale:
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = NULL;
    }

cleanup_avcodec:
    if (vidcodec_context) {
        avcodec_free_context(&vidcodec_context);
        vidcodec_context = NULL;
    }

    if (rgb_buffer) {
        free(rgb_buffer);
        rgb_buffer = NULL;
    }

fail:
    return ret;
}