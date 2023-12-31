project('vrwebtrack', 'c', default_options: ['c_std=c11'])

# Uncomment below if you need a more verbose log
# add_project_arguments('-DDEBUG_MODE', language: 'c')

glfw_dep = dependency('glfw3')
avcodec_dep = dependency('libavcodec')
avformat_dep = dependency('libavformat')
avutil_dep = dependency('libavutil')
swscale_dep = dependency('libswscale')
usb_dep = dependency('libusb-1.0')
vulkan_dep = dependency('vulkan')

camera_src = ['src/camera/camera_core.c']
if host_machine.system() == 'linux'
    camera_src += 'src/camera/camera_linux.c'
elif host_machine.system() == 'windows'
    # Windows specific source file
endif

camera_deps = [avcodec_dep, avformat_dep, avutil_dep, swscale_dep, usb_dep]
camera_include_dirs = ['./include']

camera_exec = executable('camera', [camera_src, 'src/camera/camera_main.c'], dependencies: camera_deps, include_directories: camera_include_dirs)
camera_lib = shared_library('camera', camera_src, dependencies: camera_deps, include_directories: camera_include_dirs)

monitor_src = ['src/monitor/monitor.c']
monitor_deps = [glfw_dep]
monitor_include_dirs = ['./include']

monitor_exec = executable('vrwebtrack', monitor_src, dependencies: monitor_deps, include_directories: monitor_include_dirs)

# Unit Testing

if host_machine.system() == 'linux'
    camera_test_exec = executable('test_camera_linux', [camera_src, 'tests/test_camera_linux.c' ], dependencies: camera_deps, include_directories: camera_include_dirs)
    test('Test List All Camera Devices', camera_test_exec, args: ['test_list_all_camera_devices'])
elif host_machine.system() == 'windows'
    # Windows specific source file
endif
