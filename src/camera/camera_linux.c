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

    uint32_t format_count = 0;
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        format_count++;
        fmtdesc.index++;
    }

    camera->formats_count = format_count;
    camera->formats = (camera_format*)malloc(sizeof(camera_format) * format_count);

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

            // Enumerate frame intervals
            struct v4l2_frmivalenum frmival;
            frmival.pixel_format = fmtdesc.pixelformat;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;
            frmival.index = 0;
            if (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
                camera->formats[i].fps_count = 1; // This could be more than 1, depending on the device
                camera->formats[i].fps.frame_rate_numerator = frmival.discrete.numerator;
                camera->formats[i].fps.frame_rate_denominator = frmival.discrete.denominator;
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