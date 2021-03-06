#pragma once

#include "../TestBase.h"
#include "frame-graph/FrameGraph.h"
#include "frame-graph/Resource.h"
#include "gfx-base/GFXCommandBuffer.h"
#include "gfx-base/GFXDef-common.h"

namespace cc {

namespace standard {
constexpr uint CAMERA = 0;
constexpr uint COLOR  = 1;
constexpr uint MVP    = 2;
} // namespace standard

struct StandardUniformBuffers {
    StandardUniformBuffers();
    ~StandardUniformBuffers();
    DISABLE_COPY_SEMANTICS(StandardUniformBuffers)
    ENABLE_MOVE_SEMANTICS(StandardUniformBuffers)

    vector<std::unique_ptr<gfx::Buffer>> bufferViews;

    inline float *getBuffer(uint binding) { return &_rootBuffer[_bufferViewOffsets[binding] / sizeof(float)]; }
    inline void   update() { _rootUBO->update(_rootBuffer.data(), _rootBuffer.size() * sizeof(float)); }

    std::unique_ptr<gfx::DescriptorSet> descriptorSet{nullptr};

    static std::unique_ptr<gfx::DescriptorSetLayout> descriptorSetLayout;
    static std::unique_ptr<gfx::PipelineLayout>      pipelineLayout;

private:
    friend void createStandardUniformBuffers(gfx::Device *device, StandardUniformBuffers *out);

    std::unique_ptr<gfx::Buffer> _rootUBO{nullptr};
    vector<float>                _rootBuffer;
    vector<uint>                 _bufferViewOffsets;

    static uint refCount;
};

struct StandardForwardPipeline {
    std::unique_ptr<gfx::Shader>        shader{nullptr};
    std::unique_ptr<gfx::RenderPass>    renderPass{nullptr};
    std::unique_ptr<gfx::PipelineState> pipelineState{nullptr};
    std::unique_ptr<gfx::Framebuffer>   framebuffer{nullptr};

    template <typename Fn>
    void recordCommandBuffer(gfx::Device *device, gfx::CommandBuffer *commandBuffer,
                             const gfx::Rect &renderArea, const gfx::Color *clearColors, Fn execute);
};

struct StandardDeferredPipeline {
    std::unique_ptr<gfx::Shader>        gbufferShader{nullptr};
    std::unique_ptr<gfx::RenderPass>    gbufferRenderPass{nullptr};
    std::unique_ptr<gfx::PipelineState> gbufferPipelineState{nullptr};

    vector<std::unique_ptr<gfx::Texture>> gbufferTextures;
    std::unique_ptr<gfx::Texture>         gbufferDepthStencilTexture{nullptr};
    std::unique_ptr<gfx::Framebuffer>     gbufferFramebuffer{nullptr};

    // lighting pass
    std::unique_ptr<gfx::Shader>              lightingShader{nullptr};
    std::unique_ptr<gfx::DescriptorSetLayout> lightingDescriptorSetLayout{nullptr};
    std::unique_ptr<gfx::PipelineLayout>      lightingPipelineLayout{nullptr};
    std::unique_ptr<gfx::PipelineState>       lightingPipelineState{nullptr};
    std::unique_ptr<gfx::Buffer>              lightingVertexBuffer{nullptr};
    std::unique_ptr<gfx::InputAssembler>      lightingInputAssembler{nullptr};
    std::unique_ptr<gfx::DescriptorSet>       lightingDescriptorSet{nullptr};
    std::unique_ptr<gfx::Sampler>             sampler{nullptr};

    template <typename Fn>
    void recordCommandBuffer(gfx::Device *device, gfx::CommandBuffer *commandBuffer,
                             const gfx::Rect &renderArea, const gfx::Color *clearColors, Fn execute);
};

extern void createStandardShader(gfx::Device *device, StandardForwardPipeline *out);
extern void createStandardShader(gfx::Device *device, StandardDeferredPipeline *out);
extern void createStandardUniformBuffers(gfx::Device *device, StandardUniformBuffers *out);
extern void createStandardPipelineResources(gfx::Device *device, StandardForwardPipeline *out, const StandardUniformBuffers &ubos);
extern void createStandardPipelineResources(gfx::Device *device, StandardDeferredPipeline *out, const StandardUniformBuffers &ubos);

///////////////////////////////////////////////////////////////////////////////

template <typename Fn>
void StandardForwardPipeline::recordCommandBuffer(gfx::Device * /*device*/, gfx::CommandBuffer *commandBuffer,
                                                  const gfx::Rect &renderArea, const gfx::Color *clearColors, Fn execute) {
    commandBuffer->beginRenderPass(renderPass.get(), framebuffer.get(), renderArea, clearColors, 1.0F, 0);
    commandBuffer->bindPipelineState(pipelineState.get());

    execute();

    commandBuffer->endRenderPass();
}

template <typename Fn>
void StandardDeferredPipeline::recordCommandBuffer(gfx::Device *device, gfx::CommandBuffer *commandBuffer,
                                                   const gfx::Rect &renderArea, const gfx::Color *clearColors, Fn execute) {
#if 0
    // Logic passes
    static const framegraph::StringHandle GBUFFER_PASS_NAME = framegraph::FrameGraph::stringToHandle("GBufferPass");
    static const framegraph::StringHandle SHADING_PASS_NAME = framegraph::FrameGraph::stringToHandle("ShadingPass");
    // Transient Resources
    static const framegraph::StringHandle GBUFFER_NAMES[]{
        framegraph::FrameGraph::stringToHandle("GBuffer0"),
        framegraph::FrameGraph::stringToHandle("GBuffer1"),
        framegraph::FrameGraph::stringToHandle("GBuffer2"),
        framegraph::FrameGraph::stringToHandle("GBuffer3"),
    };
    static const framegraph::StringHandle DEPTH_STENCIL_NAME = framegraph::FrameGraph::stringToHandle("DepthStencil");

    struct GBufferData {
        framegraph::TextureHandle gbuffers[4];
        framegraph::TextureHandle depthStencil;
    };

    auto gbufferPassSetup = [&](framegraph::PassNodeBuilder &builder, GBufferData &data) {
        builder.subpass(false, false);

        for (uint i = 0; i < 4; ++i) {
            // RGBA8 is suffice for albedo, emission & occlusion
            gfx::Format       format     = i % 3 ? gfx::Format::RGBA16F : gfx::Format::RGBA8;
            gfx::TextureUsage usage      = gfx::TextureUsageBit::INPUT_ATTACHMENT | gfx::TextureUsageBit::COLOR_ATTACHMENT;
            gfx::TextureFlags flags      = gfx::TextureFlagBit::NONE;
            gfx::AccessType   accessType = gfx::AccessType::FRAGMENT_SHADER_READ_COLOR_INPUT_ATTACHMENT;
            if (i == 3) { // use the emission buffer as output
                usage |= gfx::TextureUsageBit::TRANSFER_SRC;
                flags |= gfx::TextureFlagBit::GENERAL_LAYOUT;
                accessType = gfx::AccessType::TRANSFER_READ;
            }
            framegraph::Texture::Descriptor gbufferInfo;
            gbufferInfo.type   = gfx::TextureType::TEX2D;
            gbufferInfo.usage  = usage;
            gbufferInfo.format = format;
            gbufferInfo.width  = device->getWidth();
            gbufferInfo.height = device->getHeight();
            gbufferInfo.flags  = flags;
            data.gbuffers[i]   = builder.create<framegraph::Texture>(GBUFFER_NAMES[i], gbufferInfo);

            // Attachment Setup
            framegraph::RenderTargetAttachment::Descriptor gbufferAttachmentInfo;
            gbufferAttachmentInfo.loadOp      = gfx::LoadOp::CLEAR;
            gbufferAttachmentInfo.clearColor  = clearColors[i];
            gbufferAttachmentInfo.endAccesses = {accessType};

            data.gbuffers[i] = builder.write(data.gbuffers[i], gbufferAttachmentInfo);
            builder.writeToBlackboard(GBUFFER_NAMES[i], data.gbuffers[i]);
        }
        framegraph::Texture::Descriptor depthStencilInfo;
        depthStencilInfo.type   = gfx::TextureType::TEX2D;
        depthStencilInfo.usage  = gfx::TextureUsageBit::DEPTH_STENCIL_ATTACHMENT;
        depthStencilInfo.format = device->getDepthStencilFormat();
        depthStencilInfo.width  = device->getWidth();
        depthStencilInfo.height = device->getHeight();
        data.depthStencil       = builder.create<framegraph::Texture>(DEPTH_STENCIL_NAME, depthStencilInfo);

        // Attachment Setup
        framegraph::RenderTargetAttachment::Descriptor depthStencilAttachmentInfo;
        depthStencilAttachmentInfo.loadOp = gfx::LoadOp::CLEAR;
        depthStencilAttachmentInfo.endAccesses = {gfx::AccessType::DEPTH_STENCIL_ATTACHMENT_WRITE};
        depthStencilAttachmentInfo.usage       = framegraph::RenderTargetAttachment::Usage::DEPTH_STENCIL;

        data.depthStencil = builder.write(data.depthStencil, depthStencilAttachmentInfo);
        builder.writeToBlackboard(DEPTH_STENCIL_NAME, data.depthStencil);

        gfx::Viewport vp   = {renderArea.x, renderArea.y, renderArea.width, renderArea.height, 0, 1};
        gfx::Rect     rect = {renderArea.x, renderArea.y, renderArea.width, renderArea.height};
        builder.setViewport(vp, rect);
    };

    auto gbufferPassExec = [=](GBufferData & /*data*/, const framegraph::DevicePassResourceTable & /*table*/) {
        commandBuffer->bindPipelineState(gbufferPipelineState.get());
        execute();
    };

    struct ShadingData {
        framegraph::TextureHandle gbuffers[4];
        framegraph::TextureHandle depthStencil;
    };

    auto shadingPassSetup = [&](framegraph::PassNodeBuilder &builder, ShadingData &data) {
        builder.subpass(false, true);

        for (uint i = 0; i < 4; ++i) {
            data.gbuffers[i] = framegraph::TextureHandle(builder.readFromBlackboard(GBUFFER_NAMES[i]));
            data.gbuffers[i] = builder.read(data.gbuffers[i]);
        }
        framegraph::RenderTargetAttachment::Descriptor gbufferAttachmentInfo;
        gbufferAttachmentInfo.loadOp      = gfx::LoadOp::LOAD;
        gbufferAttachmentInfo.endAccesses = {gfx::AccessType::TRANSFER_READ};

        data.gbuffers[3] = builder.write(data.gbuffers[3], gbufferAttachmentInfo);
        builder.writeToBlackboard(GBUFFER_NAMES[3], data.gbuffers[3]);

        framegraph::RenderTargetAttachment::Descriptor depthStencilAttachmentInfo;
        depthStencilAttachmentInfo.loadOp = gfx::LoadOp::CLEAR;
        depthStencilAttachmentInfo.endAccesses = {gfx::AccessType::DEPTH_STENCIL_ATTACHMENT_WRITE};
        depthStencilAttachmentInfo.usage  = framegraph::RenderTargetAttachment::Usage::DEPTH_STENCIL;

        data.depthStencil = framegraph::TextureHandle(builder.readFromBlackboard(DEPTH_STENCIL_NAME));
        data.depthStencil = builder.write(data.depthStencil, depthStencilAttachmentInfo);
        builder.writeToBlackboard(DEPTH_STENCIL_NAME, data.depthStencil);
    };

    auto shadingPassExec = [=](ShadingData & /*data*/, const framegraph::DevicePassResourceTable & /*table*/) {
        commandBuffer->bindPipelineState(lightingPipelineState.get());
        commandBuffer->bindDescriptorSet(0, lightingDescriptorSet.get());
        commandBuffer->bindInputAssembler(lightingInputAssembler.get());
        commandBuffer->draw(lightingInputAssembler.get());
    };

    auto &fg = TestBaseI::fg;

    fg.reset();

    fg.addPass<GBufferData>(100, GBUFFER_PASS_NAME, gbufferPassSetup, gbufferPassExec);
    fg.addPass<ShadingData>(200, SHADING_PASS_NAME, shadingPassSetup, shadingPassExec);

    fg.presentFromBlackboard(GBUFFER_NAMES[3]);

    fg.compile();

    //fg.exportGraphViz("fg_vis.dot");

    fg.execute();
#else
    commandBuffer->beginRenderPass(gbufferRenderPass.get(), gbufferFramebuffer.get(), renderArea, clearColors, 1.0F, 0);
    commandBuffer->bindPipelineState(gbufferPipelineState.get());

    execute();

    commandBuffer->nextSubpass();

    commandBuffer->bindInputAssembler(lightingInputAssembler.get());
    commandBuffer->bindPipelineState(lightingPipelineState.get());
    commandBuffer->bindDescriptorSet(0, lightingDescriptorSet.get());
    commandBuffer->draw(lightingInputAssembler.get());

    commandBuffer->endRenderPass();

    gfx::TextureBlit region;
    region.srcExtent.width  = device->getWidth();
    region.srcExtent.height = device->getHeight();
    region.dstExtent.width  = device->getWidth();
    region.dstExtent.height = device->getHeight();
    commandBuffer->blitTexture(gbufferTextures[3].get(), nullptr, &region, 1, gfx::Filter::POINT);
#endif
}

} // namespace cc
