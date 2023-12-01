#include <boilerplate/application.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <utility>

Application::Application()
    : ApplicationBase { "Test application" }
    , m_shader_module { nullptr }
    , m_pipeline_layout { nullptr }
    , m_render_pipeline { nullptr }
    , m_f { 0.0f }
    , m_counter { 0 }
    , m_show_demo_window { true }
    , m_show_another_window { false }
    , m_clear_color { 0.45f, 0.55f, 0.60f, 1.0f }
{
    wgpu::ShaderModuleWGSLDescriptor wgsl_module_desc { wgpu::Default };
    wgsl_module_desc.code = R"(
        @vertex
        fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
            let x = f32(i32(in_vertex_index) - 1);
            let y = f32(i32(in_vertex_index & 1u) * 2 - 1);
            return vec4<f32>(x, y, 0.0, 1.0);
        }

        @fragment
            fn fs_main() -> @location(0) vec4<f32> {
            return vec4<f32>(1.0, 0.0, 0.0, 1.0);
        }
    )";
    wgpu::ShaderModuleDescriptor module_desc { wgpu::Default };
    module_desc.nextInChain = reinterpret_cast<wgpu::ChainedStruct*>(&wgsl_module_desc);
    this->m_shader_module = this->device().createShaderModule(module_desc);
    if (!this->m_shader_module) {
        std::cerr << "Failed to create the shader module" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    wgpu::PipelineLayoutDescriptor layout_desc { wgpu::Default };
    this->m_pipeline_layout = this->device().createPipelineLayout(layout_desc);
    if (!this->m_pipeline_layout) {
        std::cerr << "Failed to create the pipeline layout" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    wgpu::RenderPipelineDescriptor pipeline_desc { wgpu::Default };
    pipeline_desc.layout = this->m_pipeline_layout;
    pipeline_desc.vertex.module = this->m_shader_module;
    pipeline_desc.vertex.entryPoint = "vs_main";

    auto fragment_targets = std::array { wgpu::ColorTargetState { wgpu::Default } };
    fragment_targets[0].format = this->surface_format();
    fragment_targets[0].writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState fragment_state { wgpu::Default };
    fragment_state.module = this->m_shader_module;
    fragment_state.entryPoint = "fs_main";
    fragment_state.targetCount = fragment_targets.size();
    fragment_state.targets = fragment_targets.data();
    fragment_state.constantCount = 0;
    fragment_state.constants = nullptr;
    pipeline_desc.fragment = &fragment_state;
    pipeline_desc.depthStencil = nullptr;
    pipeline_desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipeline_desc.multisample.count = 1;
    pipeline_desc.multisample.mask = 0xFFFFFFFF;
    this->m_render_pipeline = this->device().createRenderPipeline(pipeline_desc);
    if (!this->m_render_pipeline) {
        std::cerr << "Failed to create the render pipeline" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

Application::Application(Application&& app)
    : ApplicationBase { std::move(app) }
    , m_shader_module { std::exchange(app.m_shader_module, nullptr) }
    , m_pipeline_layout { std::exchange(app.m_pipeline_layout, nullptr) }
    , m_render_pipeline { std::exchange(app.m_render_pipeline, nullptr) }
    , m_f { std::exchange(app.m_f, 0.0f) }
    , m_counter { std::exchange(app.m_counter, 0) }
    , m_show_demo_window { std::exchange(app.m_show_demo_window, true) }
    , m_show_another_window { std::exchange(app.m_show_another_window, false) }
    , m_clear_color { std::exchange(app.m_clear_color, { 0.45f, 0.55f, 0.60f, 1.00f }) }
{
}

Application::~Application()
{
    if (this->m_render_pipeline) {
        this->m_render_pipeline.release();
    }

    if (this->m_pipeline_layout) {
        this->m_pipeline_layout.release();
    }

    if (this->m_shader_module) {
        this->m_shader_module.release();
    }
}

void Application::on_frame(wgpu::CommandEncoder& encoder, wgpu::TextureView& frame)
{
    ImGui::Begin("Hello, world!"); // Create a window called "Hello, World!".

    ImGui::Text("This is some useful text."); // Display a string.
    ImGui::Checkbox("Demo Window", &this->m_show_demo_window); // Booleans can be modified with checkboxes.
    ImGui::Checkbox("Another Window", &this->m_show_another_window);

    ImGui::SliderFloat("float", &this->m_f, 0.0f, 1.0f);
    ImGui::ColorEdit3("clear color", (float*)&this->m_clear_color);

    if (ImGui::Button("Button")) {
        this->m_counter++;
    }
    ImGui::SameLine();
    ImGui::Text("counter = %d", this->m_counter);

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

    ImGui::End();

    auto color_attachments = std::array { wgpu::RenderPassColorAttachment { wgpu::Default } };
    color_attachments[0].view = frame;
    color_attachments[0].loadOp = wgpu::LoadOp::Clear;
    color_attachments[0].storeOp = wgpu::StoreOp::Store;
    color_attachments[0].clearValue = wgpu::Color { this->m_clear_color.x, this->m_clear_color.y, this->m_clear_color.z, this->m_clear_color.w };

    wgpu::RenderPassDescriptor pass_desc { wgpu::Default };
    pass_desc.colorAttachmentCount = color_attachments.size();
    pass_desc.colorAttachments = color_attachments.data();
    auto pass_encoder = encoder.beginRenderPass(pass_desc);
    if (!pass_encoder) {
        std::cerr << "Could not create render pass!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    pass_encoder.setPipeline(this->m_render_pipeline);
    pass_encoder.draw(3, 1, 0, 0);
    pass_encoder.end();
    pass_encoder.release();
}