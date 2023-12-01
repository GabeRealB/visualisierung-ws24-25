#pragma once

#include <application_base.h>

class Application final : public ApplicationBase {
public:
    Application();
    Application(const Application&) = delete;
    Application(Application&&);
    ~Application();

    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;

protected:
    void on_frame(wgpu::CommandEncoder&, wgpu::TextureView&) override;

private:
    wgpu::ShaderModule m_shader_module;
    wgpu::PipelineLayout m_pipeline_layout;
    wgpu::RenderPipeline m_render_pipeline;

    float m_f;
    int m_counter;
    bool m_show_demo_window;
    bool m_show_another_window;
    ImVec4 m_clear_color;
};