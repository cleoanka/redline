#ifndef YDS_METAL_SHADER_H
#define YDS_METAL_SHADER_H

#include "../include/yds_metal_device.h"
#include "yds_shader.h"

class ysMetalShader : public ysShader {
    friend class ysMetalDevice;

public:
    ysMetalShader();
    virtual ~ysMetalShader();

protected:

    MTL::Library* m_shader = nullptr;
    MTL::RenderPipelineState* m_pipelineState = nullptr;
    
};

#endif /* YDS_METAL_SHADER_H */
