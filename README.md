## VRWebTrack: A Cost-Effective Full Body Tracking Solution for Virtual Reality
#### Overview

VRWebTrack offers an economical solution for full-body tracking in virtual reality using webcams. It is engineered for seamless integration with VRChat and offers scalable tracking capabilities. Utilizing C language for the Foreign Function Interface, VRWebTrack incorporates features such as anti-jitter tracking and GPU-based tracking through Vulkan Compute. The software prioritizes performance, targeting a tracking speed of 100 FPS, without the need for Artificial Intelligence.

#### Features

- Economical Full-Body Tracking with Webcam Integration
- Scalability: Capable of incorporating three or more webcams; scalability is limited only by your hardware
- Anti-Jitter Tracking Algorithms
- Developed in C Language for Foreign Function Interface Compatibility
- High Level of Customizability for End-Users
- GPU-based Tracking via Vulkan Compute (Compatible with Intel, AMD, Nvidia, and most GPUs that support Vulkan 1.0)
- Performance-Centric Design, Aiming for 100 FPS Camera Tracking
- Does not communicate to third party servers, this solution is designed for self-hosted configuration
#### Platform Compatibility

- Linux: First-Class Support
- Windows: Second-Class Support (Port Under Development)
- macOS: As Supported as a Sandcastle at High Tide; Inquiries Regarding macOS Support Will Not Be Entertained

#### Software Architecture

The architecture consists of two distinct software components, a design necessitated by certain limitations observed during development. Specifically, the FFMpeg library encounters decoding issues with H.264 frames when a GLFW window context is created, prompting the requirement for separate processes.

- Monitor Software: Facilitates end-user interaction, including the selection of colored objects for tracking, ray scoping optimization radius settings, camera calibration, feeding information to SteamVR, etc.

- Webcam Processing Software: This is a self-contained, locally hosted process responsible for webcam data processing. It communicates with the Monitor Software using Shared Memory and Interprocess Communication Protocol.

**Note:** VRWebTrack does not utilize the OpenCV library. Instead, it relies on the Video for Linux 2 API for webcam communication. The exclusion of OpenCV is intentional, as its C API has been deprecated, undermining the goals of this project. VRWebTrack aims for high customizability and extensibility, allowing users to implement different algorithms or optimizations without requiring a complete rewrite of the existing software.

#### License

Please be aware that this project is licensed under GNU Affero General Public License.