#ifndef YDS_METAL_GPU_BUFFER_H
#define YDS_METAL_GPU_BUFFER_H

#include "yds_gpu_buffer.h"
#include "../include/yds_metal_device.h"

class ysMetalGPUBuffer : public ysGPUBuffer {
    friend class ysMetalDevice;

public:
    ysMetalGPUBuffer();
    virtual ~ysMetalGPUBuffer();

protected:
    MTL::Buffer* m_buffer[3] = {};   // zero-initialise; only [0..kMaxFramesInFlight) are allocated
    bool isConstantBuffer = false;
    char* constantBuffer = nullptr;
};

#endif /* YDS_METAL_GPU_BUFFER_H */
