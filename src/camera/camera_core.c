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

const char* camera_pixel_format_to_str(camera_pixel_format format)
{
    switch (format)
    {
        case camera_pixel_format_RGB332:
            return "RGB332";
        case camera_pixel_format_RGB444:
            return "RGB444";
        case camera_pixel_format_ARGB444:
            return "ARGB444";
        case camera_pixel_format_XRGB444:
            return "XRGB444";
        case camera_pixel_format_RGBA444:
            return "RGBA444";
        case camera_pixel_format_RGBX444:
            return "RGBX444";
        case camera_pixel_format_ABGR444:
            return "ABGR444";
        case camera_pixel_format_XBGR444:
            return "XBGR444";
        case camera_pixel_format_BGRA444:
            return "BGRA444";
        case camera_pixel_format_BGRX444:
            return "BGRX444";
        case camera_pixel_format_RGB555:
            return "RGB555";
        case camera_pixel_format_ARGB555:
            return "ARGB555";
        case camera_pixel_format_XRGB555:
            return "XRGB555";
        case camera_pixel_format_RGBA555:
            return "RGBA555";
        case camera_pixel_format_RGBX555:
            return "RGBX555";
        case camera_pixel_format_ABGR555:
            return "ABGR555";
        case camera_pixel_format_XBGR555:
            return "XBGR555";
        case camera_pixel_format_BGRA555:
            return "BGRA555";
        case camera_pixel_format_BGRX555:
            return "BGRX555";
        case camera_pixel_format_RGB565:
            return "RGB565";
        case camera_pixel_format_RGB555X:
            return "RGB555X";
        case camera_pixel_format_ARGB555X:
            return "ARGB555X";
        case camera_pixel_format_XRGB555X:
            return "XRGB555X";
        case camera_pixel_format_RGB565X:
            return "RGB565X";
        case camera_pixel_format_BGR666:
            return "BGR666";
        case camera_pixel_format_BGR24:
            return "BGR24";
        case camera_pixel_format_RGB24:
            return "RGB24";
        case camera_pixel_format_BGR32:
            return "BGR32";
        case camera_pixel_format_ABGR32:
            return "ABGR32";
        case camera_pixel_format_XBGR32:
            return "XBGR32";
        case camera_pixel_format_BGRA32:
            return "BGRA32";
        case camera_pixel_format_BGRX32:
            return "BGRX32";
        case camera_pixel_format_RGB32:
            return "RGB32";
        case camera_pixel_format_RGBA32:
            return "RGBA32";
        case camera_pixel_format_RGBX32:
            return "RGBX32";
        case camera_pixel_format_ARGB32:
            return "ARGB32";
        case camera_pixel_format_XRGB32:
            return "XRGB32";
        case camera_pixel_format_RGBX1010102:
            return "RGBX1010102";
        case camera_pixel_format_RGBA1010102:
            return "RGBA1010102";
        case camera_pixel_format_ARGB2101010:
            return "ARGB2101010";
        case camera_pixel_format_BGR48_12:
            return "BGR48_12";
        case camera_pixel_format_ABGR64_12:
            return "ABGR64_12";
        case camera_pixel_format_GREY:
            return "GREY";
        case camera_pixel_format_Y4:
            return "Y4";
        case camera_pixel_format_Y6:
            return "Y6";
        case camera_pixel_format_Y10:
            return "Y10";
        case camera_pixel_format_Y12:
            return "Y12";
        case camera_pixel_format_Y012:
            return "Y012";
        case camera_pixel_format_Y14:
            return "Y14";
        case camera_pixel_format_Y16:
            return "Y16";
        case camera_pixel_format_Y16_BE:
            return "Y16_BE";
        case camera_pixel_format_Y10BPACK:
            return "Y10BPACK";
        case camera_pixel_format_Y10P:
            return "Y10P";
        case camera_pixel_format_IPU3_Y10:
            return "IPU3_Y10";
        case camera_pixel_format_PAL8:
            return "PAL8";
        case camera_pixel_format_UV8:
            return "UV8";
        case camera_pixel_format_YUYV:
            return "YUYV";
        case camera_pixel_format_YYUV:
            return "YYUV";
        case camera_pixel_format_YVYU:
            return "YVYU";
        case camera_pixel_format_UYVY:
            return "UYVY";
        case camera_pixel_format_VYUY:
            return "VYUY";
        case camera_pixel_format_Y41P:
            return "Y41P";
        case camera_pixel_format_YUV444:
            return "YUV444";
        case camera_pixel_format_YUV555:
            return "YUV555";
        case camera_pixel_format_YUV565:
            return "YUV565";
        case camera_pixel_format_YUV24:
            return "YUV24";
        case camera_pixel_format_YUV32:
            return "YUV32";
        case camera_pixel_format_AYUV32:
            return "AYUV32";
        case camera_pixel_format_XYUV32:
            return "XYUV32";
        case camera_pixel_format_VUYA32:
            return "VUYA32";
        case camera_pixel_format_VUYX32:
            return "VUYX32";
        case camera_pixel_format_YUVA32:
            return "YUVA32";
        case camera_pixel_format_YUVX32:
            return "YUVX32";
        case camera_pixel_format_M420:
            return "M420";
        case camera_pixel_format_YUV48_12:
            return "YUV48_12";
        case camera_pixel_format_Y210:
            return "Y210";
        case camera_pixel_format_Y212:
            return "Y212";
        case camera_pixel_format_Y216:
            return "Y216";
        case camera_pixel_format_NV12:
            return "NV12";
        case camera_pixel_format_NV21:
            return "NV21";
        case camera_pixel_format_NV16:
            return "NV16";
        case camera_pixel_format_NV61:
            return "NV61";
        case camera_pixel_format_NV24:
            return "NV24";
        case camera_pixel_format_NV42:
            return "NV42";
        case camera_pixel_format_P010:
            return "P010";
        case camera_pixel_format_P012:
            return "P012";
        case camera_pixel_format_NV12M:
            return "NV12M";
        case camera_pixel_format_NV21M:
            return "NV21M";
        case camera_pixel_format_NV16M:
            return "NV16M";
        case camera_pixel_format_NV61M:
            return "NV61M";
        case camera_pixel_format_P012M:
            return "P012M";
        case camera_pixel_format_YUV410:
            return "YUV410";
        case camera_pixel_format_YVU410:
            return "YVU410";
        case camera_pixel_format_YUV411P:
            return "YUV411P";
        case camera_pixel_format_YUV420:
            return "YUV420";
        case camera_pixel_format_YVU420:
            return "YVU420";
        case camera_pixel_format_YUV422P:
            return "YUV422P";
        case camera_pixel_format_YUV420M:
            return "YUV420M";
        case camera_pixel_format_YVU420M:
            return "YVU420M";
        case camera_pixel_format_YUV422M:
            return "YUV422M";
        case camera_pixel_format_YVU422M:
            return "YVU422M";
        case camera_pixel_format_YUV444M:
            return "YUV444M";
        case camera_pixel_format_YVU444M:
            return "YVU444M";
        case camera_pixel_format_NV12_4L4:
            return "NV12_4L4";
        case camera_pixel_format_NV12_16L16:
            return "NV12_16L16";
        case camera_pixel_format_NV12_32L32:
            return "NV12_32L32";
        case camera_pixel_format_P010_4L4:
            return "P010_4L4";
        case camera_pixel_format_NV12_8L128:
            return "NV12_8L128";
        case camera_pixel_format_NV12_10BE_8L128:
            return "NV12_10BE_8L128";
        case camera_pixel_format_NV12MT:
            return "NV12MT";
        case camera_pixel_format_NV12MT_16X16:
            return "NV12MT_16X16";
        case camera_pixel_format_NV12M_8L128:
            return "NV12M_8L128";
        case camera_pixel_format_NV12M_10BE_8L128:
            return "NV12M_10BE_8L128";
        case camera_pixel_format_SBGGR8:
            return "SBGGR8";
        case camera_pixel_format_SGBRG8:
            return "SGBRG8";
        case camera_pixel_format_SGRBG8:
            return "SGRBG8";
        case camera_pixel_format_SRGGB8:
            return "SRGGB8";
        case camera_pixel_format_SBGGR10:
            return "SBGGR10";
        case camera_pixel_format_SGBRG10:
            return "SGBRG10";
        case camera_pixel_format_SGRBG10:
            return "SGRBG10";
        case camera_pixel_format_SRGGB10:
            return "SRGGB10";
        case camera_pixel_format_SBGGR10P:
            return "SBGGR10P";
        case camera_pixel_format_SGBRG10P:
            return "SGBRG10P";
        case camera_pixel_format_SGRBG10P:
            return "SGRBG10P";
        case camera_pixel_format_SRGGB10P:
            return "SRGGB10P";
        case camera_pixel_format_SBGGR10ALAW8:
            return "SBGGR10ALAW8";
        case camera_pixel_format_SGBRG10ALAW8:
            return "SGBRG10ALAW8";
        case camera_pixel_format_SGRBG10ALAW8:
            return "SGRBG10ALAW8";
        case camera_pixel_format_SRGGB10ALAW8:
            return "SRGGB10ALAW8";
        case camera_pixel_format_SBGGR10DPCM8:
            return "SBGGR10DPCM8";
        case camera_pixel_format_SGBRG10DPCM8:
            return "SGBRG10DPCM8";
        case camera_pixel_format_SGRBG10DPCM8:
            return "SGRBG10DPCM8";
        case camera_pixel_format_SRGGB10DPCM8:
            return "SRGGB10DPCM8";
        case camera_pixel_format_SBGGR12:
            return "SBGGR12";
        case camera_pixel_format_SGBRG12:
            return "SGBRG12";
        case camera_pixel_format_SGRBG12:
            return "SGRBG12";
        case camera_pixel_format_SRGGB12:
            return "SRGGB12";
        case camera_pixel_format_SBGGR12P:
            return "SBGGR12P";
        case camera_pixel_format_SGBRG12P:
            return "SGBRG12P";
        case camera_pixel_format_SGRBG12P:
            return "SGRBG12P";
        case camera_pixel_format_SRGGB12P:
            return "SRGGB12P";
        case camera_pixel_format_SBGGR14:
            return "SBGGR14";
        case camera_pixel_format_SGBRG14:
            return "SGBRG14";
        case camera_pixel_format_SGRBG14:
            return "SGRBG14";
        case camera_pixel_format_SRGGB14:
            return "SRGGB14";
        case camera_pixel_format_SBGGR14P:
            return "SBGGR14P";
        case camera_pixel_format_SGBRG14P:
            return "SGBRG14P";
        case camera_pixel_format_SGRBG14P:
            return "SGRBG14P";
        case camera_pixel_format_SRGGB14P:
            return "SRGGB14P";
        case camera_pixel_format_SBGGR16:
            return "SBGGR16";
        case camera_pixel_format_SGBRG16:
            return "SGBRG16";
        case camera_pixel_format_SGRBG16:
            return "SGRBG16";
        case camera_pixel_format_SRGGB16:
            return "SRGGB16";
        case camera_pixel_format_HSV24:
            return "HSV24";
        case camera_pixel_format_HSV32:
            return "HSV32";
        case camera_pixel_format_MJPEG:
            return "MJPEG";
        case camera_pixel_format_JPEG:
            return "JPEG";
        case camera_pixel_format_DV:
            return "DV";
        case camera_pixel_format_MPEG:
            return "MPEG";
        case camera_pixel_format_H264:
            return "H264";
        case camera_pixel_format_H264_NO_SC:
            return "H264_NO_SC";
        case camera_pixel_format_H264_MVC:
            return "H264_MVC";
        case camera_pixel_format_H263:
            return "H263";
        case camera_pixel_format_MPEG1:
            return "MPEG1";
        case camera_pixel_format_MPEG2:
            return "MPEG2";
        case camera_pixel_format_MPEG2_SLICE:
            return "MPEG2_SLICE";
        case camera_pixel_format_MPEG4:
            return "MPEG4";
        case camera_pixel_format_XVID:
            return "XVID";
        case camera_pixel_format_VC1_ANNEX_G:
            return "VC1_ANNEX_G";
        case camera_pixel_format_VC1_ANNEX_L:
            return "VC1_ANNEX_L";
        case camera_pixel_format_VP8:
            return "VP8";
        case camera_pixel_format_VP8_FRAME:
            return "VP8_FRAME";
        case camera_pixel_format_VP9:
            return "VP9";
        case camera_pixel_format_VP9_FRAME:
            return "VP9_FRAME";
        case camera_pixel_format_HEVC:
            return "HEVC";
        case camera_pixel_format_FWHT:
            return "FWHT";
        case camera_pixel_format_FWHT_STATELESS:
            return "FWHT_STATELESS";
        case camera_pixel_format_H264_SLICE:
            return "H264_SLICE";
        case camera_pixel_format_HEVC_SLICE:
            return "HEVC_SLICE";
        case camera_pixel_format_SPK:
            return "SPK";
        case camera_pixel_format_RV30:
            return "RV30";
        case camera_pixel_format_RV40:
            return "RV40";
        case camera_pixel_format_CPIA1:
            return "CPIA1";
        case camera_pixel_format_WNVA:
            return "WNVA";
        case camera_pixel_format_SN9C10X:
            return "SN9C10X";
        case camera_pixel_format_SN9C20X_I420:
            return "SN9C20X_I420";
        case camera_pixel_format_PWC1:
            return "PWC1";
        case camera_pixel_format_PWC2:
            return "PWC2";
        case camera_pixel_format_ET61X251:
            return "ET61X251";
        case camera_pixel_format_SPCA501:
            return "SPCA501";
        case camera_pixel_format_SPCA505:
            return "SPCA505";
        case camera_pixel_format_SPCA508:
            return "SPCA508";
        case camera_pixel_format_SPCA561:
            return "SPCA561";
        case camera_pixel_format_PAC207:
            return "PAC207";
        case camera_pixel_format_MR97310A:
            return "MR97310A";
        case camera_pixel_format_JL2005BCD:
            return "JL2005BCD";
        case camera_pixel_format_SN9C2028:
            return "SN9C2028";
        case camera_pixel_format_SQ905C:
            return "SQ905C";
        case camera_pixel_format_PJPG:
            return "PJPG";
        case camera_pixel_format_OV511:
            return "OV511";
        case camera_pixel_format_OV518:
            return "OV518";
        case camera_pixel_format_STV0680:
            return "STV0680";
        case camera_pixel_format_TM6000:
            return "TM6000";
        case camera_pixel_format_CIT_YYVYUY:
            return "CIT_YYVYUY";
        case camera_pixel_format_KONICA420:
            return "KONICA420";
        case camera_pixel_format_JPGL:
            return "JPGL";
        case camera_pixel_format_SE401:
            return "SE401";
        case camera_pixel_format_S5C_UYVY_JPG:
            return "S5C_UYVY_JPG";
        case camera_pixel_format_Y8I:
            return "Y8I";
        case camera_pixel_format_Y12I:
            return "Y12I";
        case camera_pixel_format_Z16:
            return "Z16";
        case camera_pixel_format_MT21C:
            return "MT21C";
        case camera_pixel_format_MM21:
            return "MM21";
        case camera_pixel_format_INZI:
            return "INZI";
        case camera_pixel_format_CNF4:
            return "CNF4";
        case camera_pixel_format_HI240:
            return "HI240";
        case camera_pixel_format_QC08C:
            return "QC08C";
        case camera_pixel_format_QC10C:
            return "QC10C";
        case camera_pixel_format_AJPG:
            return "AJPG";
        default:
            return "Unknown";
    }
}

void print_camera_desc(camera_desc* camera)
{
    if (camera->device_id.device_name)
        printf("Name: %s\n", camera->device_id.device_name);
    if (camera->device_id.driver_info)
        printf("Driver Info: %s\n", camera->device_id.driver_info);
    if (camera->device_id.version)
        printf("Version: %s\n", camera->device_id.version);
    if (camera->device_id.bus)
        printf("Bus: %s\n", camera->device_id.bus);
    if (camera->device_id.card)
        printf("Card: %s\n", camera->device_id.card);
    printf("Capabilities: %u\n", camera->device_id.capabilities);
    if (camera->device_id.manufacturer)
        printf("Manufacturer: %s\n", camera->device_id.manufacturer);
    if (camera->device_id.serial_number)
        printf("Serial Number: %s\n", camera->device_id.serial_number);
    if (camera->device_id.devPath)
        printf("Device Path: %s\n", camera->device_id.devPath);

    if (camera->buffers && camera->buffers_count > 0)
    {
        printf("Buffer Count: %u\n", camera->buffers_count);
        for (uint32_t i = 0; i < camera->buffers_count; ++i)
        {
            printf("\tBuffer Size: %u\n", camera->buffers[i].buffer_size);
        }
    }

    printf("Camera Capabilities:\n");
    if (camera->capabilities.can_capture)
        printf("\tCan Capture: Yes\n");
    else
        printf("\tCan Capture: No\n");
    
    if (camera->capabilities.has_hardware_acceleration)
        printf("\tHas Hardware Acceleration: Yes\n");
    else
        printf("\tHas Hardware Acceleration: No\n");

    if (camera->capabilities.has_modulator)
        printf("\tHas Modulator: Yes\n");
    else
        printf("\tHas Modulator: No\n");

    if (camera->capabilities.has_tuner)
        printf("\tHas Tuner: Yes\n");
    else
        printf("\tHas Tuner: No\n");
    
    if (camera->capabilities.supports_async_io)
        printf("\tSupports Async IO: Yes\n");
    else
        printf("\tSupports Async IO: No\n");

    if (camera->capabilities.supports_streaming)
        printf("\tSupports Streaming: Yes\n");
    else
        printf("\tSupports Streaming: No\n");

    printf("Current Control Settings:\n");
    printf("\tAuto White Balance: %u\n", camera->controls.auto_white_balance);
    printf("\tBrightness: %u\n", camera->controls.brightness);
    printf("\tContrast: %u\n", camera->controls.contrast);
    printf("\tExposure Mode: %u\n", camera->controls.exposure_mode);
    printf("\tGain: %u\n", camera->controls.gain);
    printf("\tHue: %u\n", camera->controls.hue);
    printf("\tSaturation: %u\n", camera->controls.saturation);

    if (camera->formats && camera->formats_count > 0)
    {
        printf("Available Formats:\n");
        for (uint32_t formatIdx = 0; formatIdx < camera->formats_count; ++formatIdx)
        {
            printf("\tWidth: %u\n", camera->formats[formatIdx].width);
            printf("\tHeight: %u\n", camera->formats[formatIdx].height);
            printf("\tPixel Format: %s\n", camera_pixel_format_to_str(camera->formats[formatIdx].pixel_format));
            if (camera->formats[formatIdx].fps && camera->formats[formatIdx].fps_count > 0)
            {
                printf("\tAvailable Frame Per Second Configurations:\n");
                for (uint32_t fpsIdx = 0; fpsIdx < camera->formats[formatIdx].fps_count; ++fpsIdx)
                {
                    printf("\t\tFPS: %u / %u\n", 
                        camera->formats[formatIdx].fps[fpsIdx].frame_rate_numerator,
                        camera->formats[formatIdx].fps[fpsIdx].frame_rate_denominator);
                }
            }
        }
    }

    switch (camera->io_method)
    {
        case camera_io_method_MMAP:
        {
            printf ("IO Method: MMAP\n");
            break;
        }
        case camera_io_method_READ_WRITE:
        {
            printf("IO Method: Read and Write\n");
            break;
        }
        case camera_io_method_USER_PTR:
        {
            printf("IO Method: User PTR\n");
            break;
        }
        default:
        {
            printf("IO Method: Unknown\n");
            break;
        }
    }

    printf("Streaming Params\n");
    printf("\tCapture Time Per Frame: %f\n", camera->streaming_params.capture_time_per_frame);
    printf("\tFrame Interval: %f\n", camera->streaming_params.frame_interval);

    if (camera->tuning && camera->tuning_count > 0)
    {
        printf("Tuning Configurations:\n");
        for (uint32_t tuningIdx = 0; tuningIdx < camera->tuning_count; ++tuningIdx)
        {
            printf("\tTuning #%u\n", tuningIdx);
            printf("\t\tInput Frequency: %f\n", camera->tuning[tuningIdx].input_frequency);
            if (camera->tuning[tuningIdx].is_capturing)
                printf("\t\tIs Capturing: Yes\n");
            else
                printf("\t\tIs Capturing: No\n");
            if (camera->tuning[tuningIdx].signal_locked)
                printf("\t\tSignal Locked: Yes\n");
            else
                printf("\t\tSignal Locked: No\n");
            switch (camera->tuning[tuningIdx].tuning_standard)
            {
                case tuning_standard_TUNING_AUTO:
                {
                    printf("\t\tTuning Standard: Auto\n");
                    break;
                }
                case tuning_standard_TUNING_CUSTOM:
                {
                    printf("\t\tTuning Standard: Custom\n");
                    break;
                }
                case tuning_standard_TUNING_NONE:
                {
                    printf("\t\tTuning Standard: None\n");
                    break;
                }
                case tuning_standard_TUNING_NTSC:
                {
                    printf("\t\tTuning Standard: NTSC\n");
                    break;
                }
                case tuning_standard_TUNING_PAL:
                {
                    printf("\t\tTuning Standard: PAL\n");
                    break;
                }
                case tuning_standard_TUNING_SECAM:
                {
                    printf("\t\tTuning Standard: SECAM\n");
                    break;
                }
                default:
                {
                    printf("\t\tTuning Standard: Unknown\n");
                    break;
                }
            }
        }
    }
}

void print_list_camera_desc(camera_list* list)
{
    bool afterFirst = false;
    if (list && list->count > 0 && list->cameras)
    {
        for (size_t i = 0; i < list->count; ++i)
        {
            printf("Camera #%lu\n", i);
            print_camera_desc(list->cameras[i]);
            if (afterFirst)
                printf("\n");
            else
                afterFirst = true;
        }
    }
}