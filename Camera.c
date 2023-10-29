#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <time.h>

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

int write_buffer_to_file(const char *file_path, unsigned char *buffer, size_t size) {
    FILE *file = fopen(file_path, "wb");
    if (file == NULL) {
        perror("Could not open file for writing");
        return -1;
    }

    size_t bytes_written = fwrite(buffer, 1, size, file);
    if (bytes_written != size) {
        perror("Failed to write the entire buffer to file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int write_rgb_to_bmp(const char *file_path, unsigned char *rgb_buffer, int width, int height) {
    FILE *file = fopen(file_path, "wb");
    if (file == NULL) {
        perror("Could not open file for writing");
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

int PrintCapsOfWebcam(int fd, uint32_t width, uint32_t height)
{
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        perror("Querying Capabilities");
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
    frmsize.pixel_format = V4L2_PIX_FMT_YUYV;  // replace with your pixel format
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

int decode_packet(AVCodecContext *codec_context, struct SwsContext* sws_ctx, AVPacket *packet, AVFrame *frame, AVFrame *rgb_frame, unsigned char *rgb_buffer, int width, int height) {
    int response = avcodec_send_packet(codec_context, packet);
    if (response < 0) {
        perror("Error while sending a packet to the decoder");
        return response;
    }
    while (response >= 0) {
        response = avcodec_receive_frame(codec_context, frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            perror("Error while receiving a frame from the decoder");
            return response;
        }

        if (response >= 0) {
            // Perform the scaling.
            sws_scale(sws_ctx, frame->data, frame->linesize, 0, height, rgb_frame->data, rgb_frame->linesize);

            // Now rgb_frame contains the image in RGB format. You can copy it to your buffer.
            int numBytes = av_image_get_buffer_size(rgb_frame->format, rgb_frame->width, rgb_frame->height, 1);
            av_image_copy_to_buffer(rgb_buffer, numBytes, (const uint8_t * const *)rgb_frame->data, rgb_frame->linesize, rgb_frame->format, rgb_frame->width, rgb_frame->height, 1);
        }
    }
    return 0;
}

int main() {
    struct timeval initStart;
    gettimeofday(&initStart, NULL);
    const uint32_t width = 640;
    const uint32_t height = 480;
    AVCodec* vidCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!vidCodec) {
        perror("Unable to find codec!\n");
        return 1;
    }

    AVCodecContext* vidcodec_context = avcodec_alloc_context3(vidCodec);
    if (avcodec_open2(vidcodec_context, vidCodec, NULL) < 0) {
        perror("Unable to open codec!\n");
        return 1;
    }
    vidcodec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    printf("Pixel format enum value: %d\n", vidcodec_context->pix_fmt);
    printf("Received codec!\n");
    struct SwsContext* sws_ctx = sws_getContext(width, height, vidcodec_context->pix_fmt,
                                        width, height, AV_PIX_FMT_RGB24,
                                        SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx) {
        fprintf(stderr, "Could not initialize the conversion context\n");
        return 1;
    }
    printf("Sws ctx initialized\n");

    int fd = open("/dev/video0", O_RDWR);
    if (fd == -1) {
        perror("Opening video device");
        return 1;
    }
    PrintCapsOfWebcam(fd, width, height);

    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        perror("Setting Pixel Format");
        return 1;
    }

    struct v4l2_streamparm setfps;
    memset(&setfps, 0, sizeof(struct v4l2_streamparm));
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;
    setfps.parm.capture.timeperframe.denominator = 24;

    if (ioctl(fd, VIDIOC_S_PARM, &setfps) == -1) {
        // Handle error
    }

    // Requesting buffer
    struct v4l2_requestbuffers req;
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        perror("Requesting Buffer");
        return 1;
    }

    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
        perror("Querying Buffer");
        return 1;
    }

    unsigned char* buffer = (unsigned char*)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    unsigned char rgb_buffer[width * height * 4];

    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        perror("Unable to find codec!\n");
        return 1;
    }

    AVCodecContext* codec_context = avcodec_alloc_context3(codec);
    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        perror("Unable to open codec!\n");
        return 1;
    }
    // Capture loop
    AVPacket packet;
    av_init_packet(&packet);
    AVFrame* frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();
    if (!rgb_frame) {
        fprintf(stderr, "Could not allocate RGB frame\n");
        return -1;
    }

    rgb_frame->format = AV_PIX_FMT_RGB24;
    rgb_frame->width  = width;
    rgb_frame->height = height;

    av_image_alloc(rgb_frame->data, rgb_frame->linesize, width, height, AV_PIX_FMT_RGB24, 1);
    struct timeval initEnd;
    gettimeofday(&initEnd, NULL);
    struct timeval start_time[100];
    struct timeval end_time[100];
    struct timeval totalStart;
    struct timeval totalEnd;
    gettimeofday(&totalStart, NULL);
    for (int i = 0; i < 100; i++) {
        // Put the buffer in the incoming queue.
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            perror("Queue Buffer");
            return 1;
        }

        // Execute capture
        if (ioctl(fd, VIDIOC_STREAMON, &buf.type) == -1) {
            perror("Start Capture");
            return 1;
        }
        
        // Dequeue the buffer
        gettimeofday(&start_time[i], NULL);
        if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
            perror("Retrieving Frame");
            return 1;
        }
        gettimeofday(&end_time[i], NULL);
        packet.data = buffer;
        packet.size = buf.bytesused;
        if (decode_packet(codec_context, sws_ctx, &packet, frame, rgb_frame, rgb_buffer, width, height))
        {
            perror("Failed to decode packet!\n");
            return 1;
        }
        //char path[512];
        //sprintf(path, "img%i.bmp", i);
        //write_rgb_to_bmp(path, rgb_buffer, width, height);
    }
    gettimeofday(&totalEnd, NULL);
    double totalElapsed = 0;
    for (int i = 0; i < 100; ++i)
    {
        double elapsed_us = (end_time[i].tv_sec - start_time[i].tv_sec) * 1000000LL +
                (end_time[i].tv_usec - start_time[i].tv_usec);
        elapsed_us /= 1000000.0;
        totalElapsed += elapsed_us;
        printf("Elapsed time: %.5f seconds\n", elapsed_us);
    }
    printf("Loop Elapsed: %.5f seconds\n", totalElapsed);
    double elapsed_us = (totalEnd.tv_sec - totalStart.tv_sec) * 1000000LL +
                (totalEnd.tv_usec - totalStart.tv_usec);
    elapsed_us /= 1000000.0;
    printf("Total Elapsed Time: %.5f seconds\n", elapsed_us);
    elapsed_us = (initEnd.tv_sec - initStart.tv_sec) * 1000000LL +
                (initEnd.tv_usec - initStart.tv_usec);
    elapsed_us /= 1000000.0;
    printf("Total Initialization Elapsed Time: %.5f seconds\n", elapsed_us);

    av_frame_free(frame);
    av_packet_free(&packet);
    av_freep(&rgb_frame->data[0]);
    av_frame_free(&rgb_frame);
    // Clean up
    close(fd);
    return 0;
}