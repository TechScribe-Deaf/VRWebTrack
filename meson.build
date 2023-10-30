project('vrwebtrack', 'c')

glfw_dep = dependency('glfw3')
avcodec_dep = dependency('libavcodec')
avformat_dep = dependency('libavformat')
avutil_dep = dependency('libavutil')
swscale_dep = dependency('libswscale')
usb_dep = dependency('libusb-1.0')
vulkan_dep = dependency('vulkan')

camera_src = ['src/camera_core.c', 'src/camera_linux.c']
camera_deps = [avcodec_dep, avformat_dep, avutil_dep, swscale_dep, usb_dep]
camera_include_dirs = ['./include']

camera_exec = executable('camera', camera_src, dependencies: camera_deps, include_directories: camera_include_dirs)

monitor_src = ['src/monitor.c']
monitor_deps = [glfw_dep]
monitor_include_dirs = ['./include']

monitor_exec = executable('vrwebtrack', monitor_src, dependencies: monitor_deps, include_directories: monitor_include_dirs)