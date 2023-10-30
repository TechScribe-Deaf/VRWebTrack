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

int decode_packet(AVCodecContext *codec_context, struct SwsContext* sws_ctx, AVPacket *packet, AVFrame *frame, AVFrame *rgb_frame, unsigned char *rgb_buffer, int width, int height) {
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

int initialize_avcodec(AVCodec** outVidCodec, AVCodecContext** outVidCodecContext)
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

int initialize_swscale(uint32_t width, uint32_t height, AVCodecContext* vidcodecContext, struct SwsContext** outSwsContext)
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

int set_v4l2_videocapture_format_fps(int fd, uint32_t width, uint32_t height, uint32_t fps)
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

int start_capture(const char* pathToCamera, uint32_t width, uint32_t height, uint32_t fps, atomic_int *quit)
{
    int ret = 0;

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