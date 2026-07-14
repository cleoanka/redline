#ifndef YDS_METAL_TEXTURE_H
#define YDS_METAL_TEXTURE_H

#include "yds_texture.h"

#include "../include/yds_metal_device.h"

class ysMetalTexture : public ysTexture {
    friend class ysMetalDevice;

public:
    ysMetalTexture();
    ~ysMetalTexture();

    MTL::Texture* m_texture = nullptr;
};

#endif /* YDS_METAL_TEXTURE_H */
