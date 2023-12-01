#pragma once

#include <boilerplate/application_base.h>

class Application final : public ApplicationBase {
public:
    Application();
    Application(const Application&) = delete;
    Application(Application&&);
    ~Application();

    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;

protected:
    void update(wgpu::CommandEncoder&) override;
    void render(wgpu::CommandEncoder&, wgpu::TextureView&) override;

private:
    wgpu::ShaderModule m_shader_module;
    wgpu::PipelineLayout m_pipeline_layout;
    wgpu::RenderPipeline m_render_pipeline;
};