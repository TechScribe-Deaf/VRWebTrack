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

}

int start_capture(const char* pathToCamera, uint32_t width, uint32_t height, uint32_t fps, decoded_rgb_frame_buffer_callback callback, atomic_int *quit)
{

}