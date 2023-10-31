## TODO

- Implement a function that prints Camera Desc object
- Implements Webcam Process Networking Protocol (Namely UDP Protocol and IPC)
- Test Vulkan Computation for Webcam Process ensuring that FFMpeg and Vulkan does not conflicts
- Implement GUI Monitor as priority going forward
- Implement Fishbowl Len Correction Algorithm
- Implement Compute Shaders in SPIR-V for Webcam Process to identify X/Y coordination of a radius of colored pixels
- Implement Vectorized Code for Transformation in 3D space using MVP transformation matrix in Monitor Process
- Implement velocity and anti-jitter algorithms
- Implement Ray Scoping Optimization in Compute Shader
- Implement OpenVR in Monitor Process to feed tracker information to SteamVR
- Implement Mediapipe optionally for easier tracker placement on body
- Write documentation/manual
- Provide C#/Python/Crystal programming languages binding to demonstrate FFI

## Info

- Webcam Process will be the Worker under Controller-Worker Architecture
    Webcam will simply provide X/Y coordinations in datagram packets to Monitor process
- Monitor Process will be the end-user GUI application that operates
    as a Controller in Controller-Worker  Architecture