#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include "GLFW/glfw3.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

const uint32_t width = 800;
const uint32_t height = 600;

int main() {
    for (int i = 0; i < rgb_buffer_count; ++i)
        rgb_buffer[i] = calloc(1, width * height * 3);
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(width, height, "FreeTrack", NULL, NULL);
    glfwMakeContextCurrent(window);
    while (!glfwWindowShouldClose(window)){
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}