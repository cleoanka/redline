#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define NS_PRIVATE_IMPLEMENTATION
#include "../include/yds_metal_device.h"
#include "../include/yds_sdl_window.h"
#include "../include/yds_metal_context.h"
#include "../include/yds_metal_render_target.h"
#include "../include/yds_metal_texture.h"
#include "../include/yds_metal_shader.h"
#include "../include/yds_metal_input_layout.h"
#include "../include/yds_metal_shader_program.h"
#include "../include/yds_metal_gpu_buffer.h"

#include "../include/yds_render_geometry_channel.h"

#include "../include/yds_file.h"
#include "../engines/basic/include/safe_string.h" 
#include <SDL.h>


#include <vector>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <objc/runtime.h>
#include <objc/message.h>


ysMetalDevice::ysMetalDevice() : ysDevice(ysContextObject::DeviceAPI::Metal){
    m_device = nullptr;
    m_queue = nullptr;
    _semaphore = dispatch_semaphore_create( kMaxFramesInFlight );
    _frame = 0;
    pPool = NS::AutoreleasePool::alloc()->init();
    swapchain = nullptr;
    clearColor = MTL::ClearColor(0.0, 0.0, 0.0, 1.0);
}

ysMetalDevice::~ysMetalDevice() {
    /* void */
}

ysError ysMetalDevice::InitializeDevice() {
    YDS_ERROR_DECLARE("InitializeDevice");
    printf("init metal device\n");

    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::DestroyDevice() {
    YDS_ERROR_DECLARE("DestroyDevice");


    return YDS_ERROR_RETURN(ysError::None);
}

bool ysMetalDevice::CheckSupport() {
    return false;
}

ysError ysMetalDevice::CreateRenderingContext(ysRenderingContext **renderingContext, ysWindow *window) {
    YDS_ERROR_DECLARE("CreateRenderingContext");
    if (renderingContext == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    *renderingContext = nullptr;
    ysSdlWindow *sdlWindow = static_cast<ysSdlWindow *>(window);
    swapchain = (CA::MetalLayer*)SDL_RenderGetMetalLayer(sdlWindow->GetRenderer());
    // Allow reading back the drawable texture (used by the env-var-gated frame capture
    // in Present(); harmless otherwise). The vendored metal-cpp CAMetalLayer binding
    // doesn't expose setFramebufferOnly:, so send the selector via the Obj-C runtime —
    // a metal-cpp object pointer is its underlying Obj-C id.
    ((void (*)(id, SEL, BOOL))objc_msgSend)(
        (id)swapchain, sel_registerName("setFramebufferOnly:"), NO);

    auto device = swapchain->device();
    m_drawable = swapchain->nextDrawable();
    auto name = device->name();
    printf("create metal swapchain, name: %s, type: %s\n", name->utf8String(), typeid(device).name());
    m_device = device;
    m_queue = MTL::make_owned(device->newCommandQueue());
    pCmd = MTL::make_owned(m_queue->commandBuffer());
    pRpd = MTL::make_owned(MTL::RenderPassDescriptor::renderPassDescriptor());

    MTL::DepthStencilDescriptor* pDsDesc = MTL::DepthStencilDescriptor::alloc()->init();
    pDsDesc->setDepthCompareFunction( MTL::CompareFunction::CompareFunctionLess );
    pDsDesc->setDepthWriteEnabled( true );

    p_depthStencilState = m_device->newDepthStencilState( pDsDesc );

    pDsDesc->release();
    
    
    MTL::RenderPassDepthAttachmentDescriptor* depthDescriptor = pRpd.get()->depthAttachment();
    MTL::TextureDescriptor* depthTextureDesc = MTL::TextureDescriptor::alloc()->init();
    depthTextureDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
    depthTextureDesc->setWidth(m_drawable->texture()->width());
    depthTextureDesc->setHeight(m_drawable->texture()->height());
    depthTextureDesc->setUsage(MTL::TextureUsageRenderTarget);
    depthDescriptor->setTexture(m_device->newTexture(depthTextureDesc));
    depthTextureDesc->release();
    MTL::RenderPassColorAttachmentDescriptorArray* colorAttachArray = pRpd->colorAttachments();
    auto colorAttachDesc = pRpd.get()->colorAttachments()->object(0);// MTL::make_owned(MTL::RenderPassColorAttachmentDescriptor::alloc()->init());
    colorAttachDesc->setTexture(m_drawable->texture());
    colorAttachDesc->setClearColor(clearColor);
    colorAttachArray->setObject(colorAttachDesc, 0);
    pEnc = pCmd->renderCommandEncoder( pRpd.get() );

    pEnc->setDepthStencilState(p_depthStencilState);
    ysMetalContext *newContext = m_renderingContexts.NewGeneric<ysMetalContext>();
    newContext->m_targetWindow = window;
    *renderingContext = static_cast<ysRenderingContext *>(newContext);
    newContext->Create(this, window);

    return YDS_ERROR_RETURN(ysError::None);

}

ysError ysMetalDevice::UpdateRenderingContext(ysRenderingContext *context) {
    YDS_ERROR_DECLARE("UpdateRenderingContext");


    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::DestroyRenderingContext(ysRenderingContext *&context) {
    YDS_ERROR_DECLARE("DestroyRenderingContext");

    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::SetContextMode(ysRenderingContext *context, ysRenderingContext::ContextMode mode) {
    return ysError();
}

ysError ysMetalDevice::CreateOnScreenRenderTarget(ysRenderTarget **newTarget, ysRenderingContext *context, bool depthBuffer) {
    YDS_ERROR_DECLARE("CreateOnScreenRenderTarget");

    if (newTarget == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    *newTarget = nullptr;

    if (context == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    if (context->GetAttachedRenderTarget() != nullptr) return YDS_ERROR_RETURN(ysError::ContextAlreadyHasRenderTarget);

    ysMetalRenderTarget *newRenderTarget = m_renderTargets.NewGeneric<ysMetalRenderTarget>();
    ysMetalContext *metalContext = static_cast<ysMetalContext *>(context);

    newRenderTarget->m_type = ysRenderTarget::Type::OnScreen;
    newRenderTarget->m_posX = 0;
    newRenderTarget->m_posY = 0;
    newRenderTarget->m_width = context->GetWindow()->GetGameWidth();
    newRenderTarget->m_height = context->GetWindow()->GetGameHeight();
    newRenderTarget->m_physicalWidth = context->GetWindow()->GetScreenWidth();
    newRenderTarget->m_physicalHeight = context->GetWindow()->GetScreenHeight();
    newRenderTarget->m_format = ysRenderTarget::Format::R8G8B8A8_UNORM;
    newRenderTarget->m_hasDepthBuffer = depthBuffer;
    newRenderTarget->m_associatedContext = context;

    *newTarget = newRenderTarget;
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::CreateOffScreenRenderTarget(ysRenderTarget **newTarget, int width, int height, ysRenderTarget::Format format, bool colorData, bool depthBuffer) {
    return ysError();
}

ysError ysMetalDevice::CreateSubRenderTarget(ysRenderTarget **newTarget, ysRenderTarget *parent, int x, int y, int width, int height) {
    YDS_ERROR_DECLARE("CreateSubRenderTarget");

    if (newTarget == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    if (parent == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    if (parent->GetType() == ysRenderTarget::Type::Subdivision) return YDS_ERROR_RETURN(ysError::InvalidParameter);

    // A subdivision target is a viewport region into its parent (the on-screen drawable).
    ysMetalRenderTarget *newRenderTarget = m_renderTargets.NewGeneric<ysMetalRenderTarget>();
    newRenderTarget->m_type = ysRenderTarget::Type::Subdivision;
    newRenderTarget->m_posX = x;
    newRenderTarget->m_posY = y;
    newRenderTarget->m_width = width;
    newRenderTarget->m_height = height;
    newRenderTarget->m_physicalWidth = width;
    newRenderTarget->m_physicalHeight = height;
    newRenderTarget->m_format = ysRenderTarget::Format::R8G8B8A8_UNORM;
    newRenderTarget->m_hasDepthBuffer = parent->HasDepthBuffer();
    newRenderTarget->m_associatedContext = parent->GetAssociatedContext();
    newRenderTarget->m_parent = parent;

    *newTarget = static_cast<ysRenderTarget *>(newRenderTarget);
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::ResizeRenderTarget(ysRenderTarget *target, int width, int height, int pwidth, int pheight) {
    YDS_ERROR_DECLARE("ResizeRenderTarget");
    // No GPU resource to rebuild for on-screen/subdivision targets — just update the
    // stored dimensions (the base implementation), which drive the viewport in
    // SetRenderTarget.
    return ysDevice::ResizeRenderTarget(target, width, height, pwidth, pheight);
}

ysError ysMetalDevice::DestroyRenderTarget(ysRenderTarget *&target) {
    return ysError();
}

ysError ysMetalDevice::SetRenderTarget(ysRenderTarget *target, int slot) {
    YDS_ERROR_DECLARE("SetRenderTarget");

    m_activeRenderTarget[slot] = target;
    if (target == nullptr || pEnc == nullptr) return YDS_ERROR_RETURN(ysError::None);
    if (slot != 0) return YDS_ERROR_RETURN(ysError::None);

    // Map this target to a viewport into the current drawable. On-screen targets cover the
    // whole drawable; subdivision targets (e.g. the engine view) cover their sub-region.
    // Metal, like DirectX, uses a top-left origin, and the application has already stored
    // posX/posY in top-left space, so no Y-flip is needed here.
    const int drawableW = (m_drawable != nullptr) ? (int)m_drawable->texture()->width() : 0;
    const int drawableH = (m_drawable != nullptr) ? (int)m_drawable->texture()->height() : 0;

    int px = target->GetPosX();
    int py = target->GetPosY();
    int pw = target->GetPhysicalWidth();
    int ph = target->GetPhysicalHeight();

    // Before the target has been sized, or for the on-screen target, cover the drawable.
    if (target->GetType() != ysRenderTarget::Type::Subdivision || pw <= 0 || ph <= 0) {
        px = 0; py = 0;
        pw = (drawableW > 0) ? drawableW : pw;
        ph = (drawableH > 0) ? drawableH : ph;
    }

    MTL::Viewport viewport;
    viewport.originX = (double)px;
    viewport.originY = (double)py;
    viewport.width = (double)pw;
    viewport.height = (double)ph;
    viewport.znear = 0.0;
    viewport.zfar = 1.0;
    pEnc->setViewport(viewport);

    // Clamp a scissor rectangle to the drawable so rasterisation is confined to the region.
    if (drawableW > 0 && drawableH > 0) {
        int sx = px < 0 ? 0 : px;
        int sy = py < 0 ? 0 : py;
        int sw = pw, sh = ph;
        if (sx + sw > drawableW) sw = drawableW - sx;
        if (sy + sh > drawableH) sh = drawableH - sy;
        if (sw > 0 && sh > 0) {
            MTL::ScissorRect scissor;
            scissor.x = (NS::UInteger)sx;
            scissor.y = (NS::UInteger)sy;
            scissor.width = (NS::UInteger)sw;
            scissor.height = (NS::UInteger)sh;
            pEnc->setScissorRect(scissor);
        }
    }

    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::SetDepthTestEnabled(ysRenderTarget *target, bool enable) {
   
    YDS_ERROR_DECLARE("SetDepthTestEnabled");
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::ReadRenderTarget(ysRenderTarget *src, uint8_t *target) {
    YDS_ERROR_DECLARE("ReadRenderTarget");
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::ClearBuffers(const float *_clearColor) {
    YDS_ERROR_DECLARE("ClearBuffers");
        clearColor = MTL::ClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::Present() {
    YDS_ERROR_DECLARE("Present");
    pEnc->endEncoding();

    // Env-var-gated offscreen frame capture — reads back the app's own drawable so the
    // render can be verified/recorded without macOS Screen-Recording permission. Format is
    // raw BGRA8: int32 width, int32 height, then width*height*4 bytes.
    //   REDLINE_CAPTURE=<path>                 base output path
    //   REDLINE_CAPTURE_FRAME=<n>              single frame n (default 90)
    //   REDLINE_CAPTURE_SEQ=start,stride,count dump a sequence to <path>.000, <path>.001, ...
    // No-op when REDLINE_CAPTURE is unset.
    static int s_redlineFrame = 0;
    ++s_redlineFrame;
    const char *redlineCapPath = std::getenv("REDLINE_CAPTURE");
    char redlineCapName[1200];
    redlineCapName[0] = '\0';
    if (redlineCapPath != nullptr) {
        const char *seq = std::getenv("REDLINE_CAPTURE_SEQ");
        if (seq != nullptr) {
            int start = 0, stride = 1, count = 1;
            if (std::sscanf(seq, "%d,%d,%d", &start, &stride, &count) == 3 && stride > 0
                    && s_redlineFrame >= start && (s_redlineFrame - start) % stride == 0) {
                const int idx = (s_redlineFrame - start) / stride;
                if (idx >= 0 && idx < count)
                    std::snprintf(redlineCapName, sizeof(redlineCapName), "%s.%03d", redlineCapPath, idx);
            }
        } else {
            const int frameN = std::getenv("REDLINE_CAPTURE_FRAME")
                ? std::atoi(std::getenv("REDLINE_CAPTURE_FRAME")) : 90;
            if (s_redlineFrame == frameN)
                std::snprintf(redlineCapName, sizeof(redlineCapName), "%s", redlineCapPath);
        }
    }
    MTL::Texture *redlineCapTex = nullptr;
    if (redlineCapName[0] != '\0') {
        MTL::Texture *src = m_drawable->texture();
        MTL::TextureDescriptor *td = MTL::TextureDescriptor::alloc()->init();
        td->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
        td->setWidth(src->width());
        td->setHeight(src->height());
        td->setStorageMode(MTL::StorageModeShared);
        td->setUsage(MTL::TextureUsageShaderRead);
        redlineCapTex = m_device->newTexture(td);
        td->release();
        MTL::BlitCommandEncoder *blit = pCmd->blitCommandEncoder();
        blit->copyFromTexture(src, 0, 0, MTL::Origin(0, 0, 0),
                              MTL::Size(src->width(), src->height(), 1),
                              redlineCapTex, 0, 0, MTL::Origin(0, 0, 0));
        blit->endEncoding();
    }

    pCmd->presentDrawable( m_drawable );
    pCmd->commit();

    if (redlineCapTex != nullptr) {
        pCmd->waitUntilCompleted();
        const int cw = (int)redlineCapTex->width();
        const int ch = (int)redlineCapTex->height();
        std::vector<uint8_t> pixels((size_t)cw * ch * 4);
        redlineCapTex->getBytes(pixels.data(), (NS::UInteger)(cw * 4),
                                MTL::Region::Make2D(0, 0, cw, ch), 0);
        FILE *f = std::fopen(redlineCapName, "wb");
        if (f != nullptr) {
            int32_t hdr[2] = { cw, ch };
            std::fwrite(hdr, sizeof(int32_t), 2, f);
            std::fwrite(pixels.data(), 1, pixels.size(), f);
            std::fclose(f);
            printf("[redline] captured frame %d -> %s (%dx%d)\n",
                   s_redlineFrame, redlineCapName, cw, ch);
            fflush(stdout);
        }
        redlineCapTex->release();
    }
    m_drawable->release();
    m_drawable = swapchain->nextDrawable();
    pCmd = MTL::make_owned(m_queue->commandBuffer());
    pEnc->release();
    MTL::RenderPassDepthAttachmentDescriptor* depthDescriptor = pRpd.get()->depthAttachment();
    depthDescriptor->texture()->release();
    pRpd = MTL::make_owned(MTL::RenderPassDescriptor::renderPassDescriptor());
    MTL::RenderPassColorAttachmentDescriptorArray* colorAttachArray = pRpd->colorAttachments();

    depthDescriptor = pRpd.get()->depthAttachment();
    MTL::TextureDescriptor* depthTextureDesc = MTL::TextureDescriptor::alloc()->init();
    depthTextureDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
    depthTextureDesc->setWidth(m_drawable->texture()->width());
    depthTextureDesc->setHeight(m_drawable->texture()->height());
    depthTextureDesc->setUsage(MTL::TextureUsageRenderTarget);
    depthDescriptor->setTexture(m_device->newTexture(depthTextureDesc));
    depthTextureDesc->release();

    auto colorAttachDesc = pRpd.get()->colorAttachments()->object(0);
    colorAttachDesc->setLoadAction(MTL::LoadAction::LoadActionClear);
    colorAttachDesc->setStoreAction(MTL::StoreAction::StoreActionStore);
    colorAttachDesc->setTexture(m_drawable->texture());
    colorAttachDesc->setClearColor(clearColor);
    colorAttachArray->setObject(colorAttachDesc, 0);
    pEnc = pCmd->renderCommandEncoder( pRpd.get() );
    pEnc->setDepthStencilState(p_depthStencilState);

    dispatch_semaphore_wait( _semaphore, DISPATCH_TIME_FOREVER );
    _frame = (_frame + 1) % kMaxFramesInFlight;
    pCmd->addCompletedHandler( ^void( MTL::CommandBuffer* pCmd ){
        dispatch_semaphore_signal(_semaphore);
    });
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::SetFaceCulling(bool faceCulling) {
    return ysError();
}

ysError ysMetalDevice::SetFaceCullingMode(CullMode cullMode) {
    return ysError();
}

ysError ysMetalDevice::CreateVertexBuffer(ysGPUBuffer **newBuffer, int size, char *data, bool mirrorToRam) {
    YDS_ERROR_DECLARE("CreateVertexBuffer");
    if (newBuffer == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    *newBuffer = nullptr;

    ysMetalGPUBuffer *newMetalBuffer = m_gpuBuffers.NewGeneric<ysMetalGPUBuffer>();

    newMetalBuffer->m_size = size;
    newMetalBuffer->m_mirrorToRAM = mirrorToRam;
    newMetalBuffer->m_bufferType = ysGPUBuffer::GPU_DATA_BUFFER;

    const size_t m_buffSize = size; //3 FramesInFlight
    //printf("create vertex buffer size: %d\n", size);
    for (int i = 0; i < kMaxFramesInFlight; ++i)
    {
        newMetalBuffer->m_buffer[i] = m_device->newBuffer(m_buffSize, MTL::ResourceStorageModeManaged);
        if (data != nullptr)
        {
            char* bufferData = reinterpret_cast<char*>(newMetalBuffer->m_buffer[i]->contents());
            memcpy(bufferData, data, size);
            newMetalBuffer->m_buffer[i]->didModifyRange(NS::Range::Make(0, size));
        }
    }

    if (mirrorToRam) {
        newMetalBuffer->m_RAMMirror = new char[size];
        if (data != nullptr) memcpy(newMetalBuffer->m_RAMMirror, data, size);
    }
    
    //char* bufferData = reinterpret_cast<char*>(newMetalBuffer->m_buffer[_frame]->contents());
    //memcpy(bufferData, data, size);

    *newBuffer = static_cast<ysGPUBuffer *>(newMetalBuffer);

    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::CreateIndexBuffer(ysGPUBuffer **newBuffer, int size, char *data, bool mirrorToRam) {
    YDS_ERROR_DECLARE("CreateIndexBuffer");
    if (newBuffer == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    *newBuffer = nullptr;

    ysMetalGPUBuffer *newMetalBuffer = m_gpuBuffers.NewGeneric<ysMetalGPUBuffer>();

    newMetalBuffer->m_size = size;
    newMetalBuffer->m_mirrorToRAM = mirrorToRam;
    newMetalBuffer->m_bufferType = ysGPUBuffer::GPU_DATA_BUFFER;

    const size_t m_buffSize = size; //3 FramesInFlight
    for (int i = 0; i < kMaxFramesInFlight; ++i)
    {
        newMetalBuffer->m_buffer[i] = m_device->newBuffer(m_buffSize, MTL::ResourceStorageModeManaged);
        if (data != nullptr)
        {
            char* bufferData = reinterpret_cast<char*>(newMetalBuffer->m_buffer[i]->contents());
            memcpy(bufferData, data, size);
            newMetalBuffer->m_buffer[i]->didModifyRange(NS::Range::Make(0, size));
        }
    }

    if (mirrorToRam) {
        newMetalBuffer->m_RAMMirror = new char[size];
        if (data != nullptr) memcpy(newMetalBuffer->m_RAMMirror, data, size);
    }

    *newBuffer = static_cast<ysGPUBuffer *>(newMetalBuffer);

    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::CreateConstantBuffer(ysGPUBuffer **newBuffer, int size, char *data, bool mirrorToRam) {
    YDS_ERROR_DECLARE("CreateConstantBuffer");
    if (newBuffer == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    *newBuffer = nullptr;

    ysMetalGPUBuffer *newMetalBuffer = m_gpuBuffers.NewGeneric<ysMetalGPUBuffer>();

    newMetalBuffer->m_size = size;
    newMetalBuffer->m_mirrorToRAM = mirrorToRam;
    newMetalBuffer->m_bufferType = ysGPUBuffer::GPU_CONSTANT_BUFFER;
    newMetalBuffer->isConstantBuffer = true;

    const size_t m_buffSize = size; //3 FramesInFlight
    newMetalBuffer->constantBuffer = new char[size];
    /*for (int i = 0; i < kMaxFramesInFlight; ++i)
    {
        newMetalBuffer->m_buffer[i] = m_device->newBuffer(m_buffSize, MTL::ResourceStorageModeManaged);
        if (data != nullptr)
        {
            char* bufferData = reinterpret_cast<char*>(newMetalBuffer->m_buffer[i]->contents());
            memcpy(bufferData, data, size);
            newMetalBuffer->m_buffer[i]->didModifyRange(NS::Range::Make(0, size));
        }
    }*/

    if (mirrorToRam) {
        newMetalBuffer->m_RAMMirror = new char[size];
        if (data != nullptr) memcpy(newMetalBuffer->m_RAMMirror, data, size);
    }
    

    *newBuffer = static_cast<ysGPUBuffer *>(newMetalBuffer);
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::UseVertexBuffer(ysGPUBuffer *buffer, int stride, int offset) {
    YDS_ERROR_DECLARE("UseVertexBuffer");
    m_activeVertexBuffer = buffer;
    ysMetalGPUBuffer* metalBuffer = static_cast<ysMetalGPUBuffer*>(buffer);
    MTL::Buffer* buf = metalBuffer->m_buffer[_frame];
    //printf("use metal vertex buff %p, ysBuff: %p, frame %d\n", &buf, &buffer, _frame);
    pEnc->setVertexBuffer(buf, /* offset */ offset, /* index */ 0 );
    metalBuffer->m_buffer[_frame]->didModifyRange(NS::Range::Make(0, buffer->GetSize()));
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::UseIndexBuffer(ysGPUBuffer *buffer, int offset) {
    YDS_ERROR_DECLARE("UseIndexBuffer");
    m_activeIndexBuffer = buffer;
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::UseConstantBuffer(ysGPUBuffer *buffer, int slot) {
    YDS_ERROR_DECLARE("UseConstantBuffer");
    //printf("use constant buffer: %d, frame: %d\n", slot, _frame);
    m_activeConstantBuffer = buffer;
    ysMetalGPUBuffer* metalBuffer = static_cast<ysMetalGPUBuffer*>(buffer);
    //MTL::Buffer* buf = metalBuffer->m_buffer[_frame];
    //printf("use metal vertex buff %p, ysBuff: %p\n", &buf, &buffer);
    //pEnc->setVertexBuffer(buf, /* offset */ 0, /* index */ slot + 1 ); //vertex buffer is always 0 in metal, so shift these up one
    //pEnc->setFragmentBuffer(buf, 0, slot + 1);

    pEnc->setVertexBytes(metalBuffer->constantBuffer, metalBuffer->GetSize(), slot + 1);
    pEnc->setFragmentBytes(metalBuffer->constantBuffer, metalBuffer->GetSize(), slot + 1);

    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::EditBufferDataRange(ysGPUBuffer *buffer, char *data, int size, int offset) {
    YDS_ERROR_DECLARE("EditBufferRange");
    ysMetalGPUBuffer* metalBuffer = static_cast<ysMetalGPUBuffer*>(buffer);
    if (!metalBuffer->isConstantBuffer)
    {
        MTL::Buffer* buf = metalBuffer->m_buffer[_frame];
        char* bufferData = reinterpret_cast<char*>(buf->contents());
        //bufferData = data;
        memcpy(&bufferData[offset], data, size);
        buf->didModifyRange(NS::Range::Make(offset, size));
    }
    else
    {
        memcpy(metalBuffer->constantBuffer, data, size);
    }
    
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::EditBufferData(ysGPUBuffer *buffer, char *data) {
    YDS_ERROR_DECLARE("EditBufferData");
    //printf("EditBufferData size: %d, buffer: %p\n", buffer->GetSize(), &buffer);
    ysMetalGPUBuffer* metalBuffer = static_cast<ysMetalGPUBuffer*>(buffer);
    if (!metalBuffer->isConstantBuffer)
    {
        MTL::Buffer* buf = metalBuffer->m_buffer[_frame];
        char* bufferData = reinterpret_cast<char*>(buf->contents());
        memcpy(bufferData, data, buffer->GetSize());
        buf->didModifyRange(NS::Range::Make(0, buffer->GetSize()));
    }
    else
    {
        memcpy(metalBuffer->constantBuffer, data, metalBuffer->GetSize());
    }
    
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::DestroyGPUBuffer(ysGPUBuffer *&buffer) {
    YDS_ERROR_DECLARE("destroybuffer");
    printf("destroy buffer\n");
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::CreateVertexShader(ysShader **newShader, const char *shaderFilename, const char *shaderName) {
    using NS::StringEncoding::UTF8StringEncoding;
    YDS_ERROR_DECLARE("CreateVertexShader");
    if (newShader == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    *newShader = nullptr;

    if (shaderFilename == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    if (shaderName == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);

    ysFile file;
    int fileLength;
    char *fileBuffer;

    NS::Error* pError = nullptr;
    YDS_NESTED_ERROR_CALL(file.OpenFile(shaderFilename, ysFile::FILE_BINARY | ysFile::FILE_READ));
    fileLength = file.GetFileLength();
    fileBuffer = new char[(unsigned long long)fileLength + 1];
    file.ReadFileToBuffer(fileBuffer);
    fileBuffer[fileLength] = 0;
    MTL::Library* pLibrary = m_device->newLibrary( NS::String::string(fileBuffer, UTF8StringEncoding), nullptr, &pError );
    delete[] fileBuffer;
    if ( !pLibrary )
    {
        __builtin_printf( "Metal Shader Error: %s\n", pError->localizedDescription()->utf8String() );
        assert( false );
    }
    ysMetalShader *newMetalShader = m_shaders.NewGeneric<ysMetalShader>();
    strcpy_s(newMetalShader->m_shaderName, 64, shaderName);
    strcpy_s(newMetalShader->m_filename, 256, shaderFilename);
    newMetalShader->m_shaderType = ysShader::ShaderType::Vertex;


    newMetalShader->m_shader = pLibrary;

    *newShader = static_cast<ysShader *>(newMetalShader);

    printf("metal shader ok\n");
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::CreatePixelShader(ysShader **newShader, const char *shaderFilename, const char *shaderName) {
    YDS_ERROR_DECLARE("CreatePixelShader");
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::DestroyShader(ysShader *&shader) {
    return ysError();
}

ysError ysMetalDevice::CreateShaderProgram(ysShaderProgram **newProgram) {
    YDS_ERROR_DECLARE("CreateShaderProgram");

    if (newProgram == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);

    ysMetalShaderProgram *metalProgram = m_shaderPrograms.NewGeneric<ysMetalShaderProgram>();
    *newProgram = static_cast<ysShaderProgram *>(metalProgram);
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::DestroyShaderProgram(ysShaderProgram *&shader, bool destroyShaders) {
    return ysError();
}

ysError ysMetalDevice::AttachShader(ysShaderProgram *program, ysShader *shader) {
    YDS_ERROR_DECLARE("AttachShader");
    if (shader != nullptr)
    {
        YDS_NESTED_ERROR_CALL(ysDevice::AttachShader(program, shader));
        ysMetalShaderProgram *metalProgram = static_cast<ysMetalShaderProgram *>(program);
        ysMetalShader *metalShader = static_cast<ysMetalShader *>(shader);
        metalProgram->m_shader = metalShader;
    }

    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::LinkProgram(ysShaderProgram *program) {
    YDS_ERROR_DECLARE("LinkProgram");
    /*nothing to do*/
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::UseShaderProgram(ysShaderProgram * program) {
    YDS_ERROR_DECLARE("UseShaderProgram");
    ysMetalShaderProgram* metalProgram = static_cast<ysMetalShaderProgram*>(program);
    pEnc->setRenderPipelineState(metalProgram->m_shader->m_pipelineState);
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::CreateInputLayout(ysInputLayout **newInputLayout, ysShader *shader, const ysRenderGeometryFormat *format) {
    YDS_ERROR_DECLARE("CreateInputLayout");
    NS::Error* pError;
    printf("metal create input layout\n");
    ysMetalShader *metalShader = static_cast<ysMetalShader *>(shader);
    auto vertexFunction = metalShader->m_shader->newFunction(NS::String::string("vertex_main", NS::UTF8StringEncoding));
    auto fragFunction = metalShader->m_shader->newFunction(NS::String::string("fragment_main", NS::UTF8StringEncoding));

    
    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction(vertexFunction);
    pDesc->setFragmentFunction(fragFunction);
    pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm);
    pDesc->setDepthAttachmentPixelFormat( MTL::PixelFormat::PixelFormatDepth32Float );

    MTL::VertexDescriptor* m_vertexDesc = MTL::VertexDescriptor::alloc()->init();

    ysMetalInputLayout *newLayout = m_inputLayouts.NewGeneric<ysMetalInputLayout>();
    newLayout->m_size = 0;
    int nChannels = format->GetChannelCount();
    MTL::VertexBufferLayoutDescriptorArray* m_layoutDescArray = m_vertexDesc->layouts();
    MTL::VertexBufferLayoutDescriptor* m_layoutDesc = m_layoutDescArray->object(0);
    m_layoutDesc->setStride(0);
    MTL::VertexAttributeDescriptorArray* m_vertexAttribDescArray = m_vertexDesc->attributes();
    

    for (int i = 0; i < nChannels; i++) {

        const ysRenderGeometryChannel *channel = format->GetChannel(i);
        MTL::VertexAttributeDescriptor* m_attributeDesc = MTL::VertexAttributeDescriptor::alloc()->init();
        m_attributeDesc->setFormat(ConvertInputLayoutFormat(channel->GetFormat()));
        m_attributeDesc->setBufferIndex(0);
        m_attributeDesc->setOffset(channel->GetOffset());
        m_vertexAttribDescArray->setObject(m_attributeDesc, i);
        m_layoutDesc->setStride(m_layoutDesc->stride() + GetStrideOfFormat(m_attributeDesc->format()));
    }
    printf("Stride: %d\n", m_layoutDesc->stride());
    
    //m_layoutDescArray->setObject(m_layoutDesc, 0);
    pDesc->setVertexDescriptor(m_vertexDesc);
    metalShader->m_pipelineState = m_device->newRenderPipelineState(pDesc, &pError);
    if (!metalShader->m_pipelineState)
    {
        printf("Shader Pipeline State Error: %s\n", pError->localizedDescription()->utf8String());
        assert(false);
    }
    

    *newInputLayout = static_cast<ysInputLayout *>(newLayout);

    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::UseInputLayout(ysInputLayout *layout) {
    YDS_ERROR_DECLARE("CreateInputLayout");
    //ysMetalShaderProgram* metalShader = static_cast<ysMetalShaderProgram*>(m_activeShaderProgram);
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::DestroyInputLayout(ysInputLayout *&layout) {
    return ysError();
}

ysError ysMetalDevice::CreateTexture(ysTexture **texture, const char *fname) {
    YDS_ERROR_DECLARE("CreateTexture");
    printf("texture1\n");
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::CreateTexture(ysTexture **texture, int width, int height, const unsigned char *buffer) {
    YDS_ERROR_DECLARE("CreateTexture");
    printf("texture2\n");
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::UpdateTexture(ysTexture *texture, const unsigned char *buffer) {
    return ysError();
}

ysError ysMetalDevice::CreateAlphaTexture(ysTexture **texture, int width, int height, const unsigned char *buffer) {
    YDS_ERROR_DECLARE("CreateTexture");
    if (texture == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);
    *texture = nullptr;
    ysMetalTexture *newTexture = m_textures.NewGeneric<ysMetalTexture>();

    strcpy_s(newTexture->m_filename, 257, "");
    newTexture->m_width = width;
    newTexture->m_height = height;
    //This data comes in as a binary map
    //We will convert it to be white and opaque on 1, and black and transparent on 0;
    int numBytes = width * height * 4;
    uint8_t* metalBuf = new uint8_t[numBytes];
    for (int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            size_t i = y * width + x;
            uint8_t c = buffer[i] > 0 ? 0xFF : 0x00;
            metalBuf[(i * 4)] = c; // blue
            metalBuf[(i * 4) + 1] = c; // green 
            metalBuf[(i * 4) + 2] = c; // red
            metalBuf[(i * 4) + 3] = buffer[i]; // alpha
        }
    }
    
    MTL::TextureDescriptor* pTextureDesc = MTL::TextureDescriptor::alloc()->init();
    pTextureDesc->setWidth(width);
    pTextureDesc->setHeight(height);
    pTextureDesc->setPixelFormat( MTL::PixelFormatRGBA8Unorm );
    pTextureDesc->setTextureType( MTL::TextureType2D );
    pTextureDesc->setStorageMode( MTL::StorageModeManaged );
    pTextureDesc->setUsage( MTL::ResourceUsageSample | MTL::ResourceUsageRead );

    MTL::Texture *pTexture = m_device->newTexture( pTextureDesc );
    newTexture->m_texture = pTexture;
    newTexture->m_texture->replaceRegion( MTL::Region( 0, 0, 0, width, height, 1 ), 0, metalBuf, width * 4); 
    pTextureDesc->release();
    
    *texture = static_cast<ysTexture *>(newTexture);

    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::DestroyTexture(ysTexture *&texture) {
    return ysError();
}

ysError ysMetalDevice::UseTexture(ysTexture *texture, int slot) {
    YDS_ERROR_DECLARE("UseTexture");
    if (texture != nullptr)
    {
        ysMetalTexture* metalTexture = static_cast<ysMetalTexture*>(texture);
        pEnc->setFragmentTexture(metalTexture->m_texture, slot);
    }
    
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysMetalDevice::UseRenderTargetAsTexture(ysRenderTarget *renderTarget, int slot) {
    printf("Use render target as texture\n");
    return ysError();
}

void ysMetalDevice::Draw(int numFaces, int indexOffset, int vertexOffset) {
    if (numFaces > 0)
    {
        ysMetalGPUBuffer* metalBuffer = static_cast<ysMetalGPUBuffer*>(m_activeIndexBuffer);
        MTL::Buffer* buf = metalBuffer->m_buffer[_frame];
        pEnc->drawIndexedPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, numFaces * 3, MTL::IndexType::IndexTypeUInt16, buf, indexOffset * 2, 1, vertexOffset, 0);
    }
    
}

MTL::VertexFormat ysMetalDevice::ConvertInputLayoutFormat(ysRenderGeometryChannel::ChannelFormat format) {
    switch (format) {
    case ysRenderGeometryChannel::ChannelFormat::R32G32B32A32_FLOAT:
        return MTL::VertexFormat::VertexFormatFloat4;
    case ysRenderGeometryChannel::ChannelFormat::R32G32B32_FLOAT:
        return MTL::VertexFormat::VertexFormatFloat3;
    case ysRenderGeometryChannel::ChannelFormat::R32G32_FLOAT:
        return MTL::VertexFormat::VertexFormatFloat2;
    case ysRenderGeometryChannel::ChannelFormat::R32G32B32A32_UINT:
        return MTL::VertexFormat::VertexFormatUInt4;
    case ysRenderGeometryChannel::ChannelFormat::R32G32B32_UINT:
        return MTL::VertexFormat::VertexFormatUInt3;
    case ysRenderGeometryChannel::ChannelFormat::Undefined:
    default:
        return MTL::VertexFormat::VertexFormatInvalid;
    }
}

int ysMetalDevice::GetStrideOfFormat(MTL::VertexFormat format)
{
    switch (format)
    {
        case MTL::VertexFormat::VertexFormatFloat4:
            return 4*4;
        case MTL::VertexFormat::VertexFormatFloat3:
            return 4*3;
        case MTL::VertexFormat::VertexFormatFloat2:
            return 4*2;
        case MTL::VertexFormat::VertexFormatUInt4:
            return 4;
        case MTL::VertexFormat::VertexFormatUInt3:
            return 3;
    }
}


// ctor magic to register as a subclass
static ysRegisterSubclass<ysMetalDevice> reg();
