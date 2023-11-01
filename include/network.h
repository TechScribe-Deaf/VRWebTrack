#ifndef NETWORK_H
#define NETWORK_H
#include <stdint.h>

typedef struct CoordinationDatagram
{
    uint32_t CameraID;
    uint32_t X;
    uint32_t Y;
} CoordinationDatagram;

typedef enum GLSLPurpose
{
    None,
    UpdateTrackingCompute
} GLSLPurpose;

typedef struct UpdateGLSLCode
{
    uint64_t length;
    uint8_t* buffer;
    GLSLPurpose purpose;
} UpdateGLSLCode;

#endif