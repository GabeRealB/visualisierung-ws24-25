#include <application.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <utility>

Application::Application()
    : ApplicationBase { "Exercise 10" }
    , m_shader_module { nullptr }
    , m_bind_group_layout { nullptr }
    , m_pipeline_layout { nullptr }
    , m_render_pipeline { nullptr }
    , m_slice_texture { nullptr }
    , m_slice_texture_changed { false }
    , m_volume { std::nullopt }
    , m_dataset { Dataset::Baby }
    , m_plane { SlicePlane::Axial }
    , m_plane_offset { 0.0f }
    , m_plane_rotation { 0.0f }
{
    wgpu::ShaderModuleWGSLDescriptor wgsl_module_desc { wgpu::Default };
    wgsl_module_desc.code = R"(
        @group(0)
        @binding(0)
        var slice_texture: texture_2d<f32>;

        struct VertexOutput {
            @builtin(position) position: vec4<f32>,
            @location(0) uv: vec2<f32>
        }

        @vertex
        fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> VertexOutput {
            var VERTEX_BUFFER = array<vec2<f32>, 6>(
                vec2<f32>(-1.0, -1.0),
                vec2<f32>(1.0, -1.0),
                vec2<f32>(-1.0, 1.0),
                vec2<f32>(1.0, -1.0),
                vec2<f32>(1.0, 1.0),
                vec2<f32>(-1.0, 1.0),
            );
            var UV_BUFFER = array<vec2<f32>, 6>(
                vec2<f32>(0.0, 0.0),
                vec2<f32>(1.0, 0.0),
                vec2<f32>(0.0, 1.0),
                vec2<f32>(1.0, 0.0),
                vec2<f32>(1.0, 1.0),
                vec2<f32>(0.0, 1.0),
            );

            let pos = vec4(VERTEX_BUFFER[in_vertex_index], 0.0, 1.0);
            let uv = UV_BUFFER[in_vertex_index];
            return VertexOutput(pos, uv);
        }

        @fragment
        fn fs_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
            let dimensions = vec2<f32>(textureDimensions(slice_texture));
            let texel = vec2<u32>(dimensions * uv);
            return textureLoad(slice_texture, texel, 0);
        }
    )";
    wgpu::ShaderModuleDescriptor module_desc { wgpu::Default };
    module_desc.nextInChain = reinterpret_cast<wgpu::ChainedStruct*>(&wgsl_module_desc);
    this->m_shader_module = this->device().createShaderModule(module_desc);
    if (!this->m_shader_module) {
        std::cerr << "Failed to create the shader module" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::array<wgpu::BindGroupLayoutEntry, 1> binding_layout_entries { wgpu::Default };
    binding_layout_entries[0].binding = 0;
    binding_layout_entries[0].visibility = wgpu::ShaderStage::Fragment;
    binding_layout_entries[0].texture.sampleType = wgpu::TextureSampleType::Float;
    binding_layout_entries[0].texture.viewDimension = wgpu::TextureViewDimension::_2D;

    wgpu::BindGroupLayoutDescriptor group_layout_desc { wgpu::Default };
    group_layout_desc.entryCount = binding_layout_entries.size();
    group_layout_desc.entries = binding_layout_entries.data();
    this->m_bind_group_layout = this->device().createBindGroupLayout(group_layout_desc);
    if (!this->m_bind_group_layout) {
        std::cerr << "Failed to create the bind group layout" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    wgpu::PipelineLayoutDescriptor layout_desc { wgpu::Default };
    layout_desc.bindGroupLayoutCount = 1;
    layout_desc.bindGroupLayouts = (WGPUBindGroupLayout*)&this->m_bind_group_layout;
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

    this->init_slice_texture();
}

Application::Application(Application&& app)
    : ApplicationBase { std::move(app) }
    , m_shader_module { std::exchange(app.m_shader_module, nullptr) }
    , m_bind_group_layout { std::exchange(app.m_bind_group_layout, nullptr) }
    , m_pipeline_layout { std::exchange(app.m_pipeline_layout, nullptr) }
    , m_render_pipeline { std::exchange(app.m_render_pipeline, nullptr) }
    , m_slice_texture { std::exchange(app.m_slice_texture, nullptr) }
    , m_slice_texture_changed { std::exchange(app.m_slice_texture_changed, false) }
    , m_volume { std::exchange(app.m_volume, std::nullopt) }
    , m_dataset { std::exchange(app.m_dataset, Dataset::Baby) }
    , m_plane { std::exchange(app.m_plane, SlicePlane::Axial) }
    , m_plane_offset { std::exchange(app.m_plane_offset, 0.0f) }
    , m_plane_rotation { std::exchange(app.m_plane_rotation, 0.0f) }
{
}

Application::~Application()
{
    if (this->m_slice_texture) {
        this->m_slice_texture.release();
    }

    if (this->m_render_pipeline) {
        this->m_render_pipeline.release();
    }

    if (this->m_pipeline_layout) {
        this->m_pipeline_layout.release();
    }

    if (this->m_bind_group_layout) {
        this->m_bind_group_layout.release();
    }

    if (this->m_shader_module) {
        this->m_shader_module.release();
    }
}

void Application::on_frame(wgpu::CommandEncoder& encoder, wgpu::TextureView& frame)
{
    ImGui::Begin("Config");
    ImGui::Text("Dataset");
    ImGui::SameLine();
    int dataset = static_cast<int>(this->m_dataset);
    bool dataset_changed = ImGui::RadioButton("Baby", &dataset, 0);
    ImGui::SameLine();
    dataset_changed |= ImGui::RadioButton("CT-Head", &dataset, 1);
    ImGui::SameLine();
    dataset_changed |= ImGui::RadioButton("Fuel", &dataset, 2);
    dataset_changed |= !this->m_volume.has_value();
    if (dataset_changed) {
        this->m_dataset = static_cast<Dataset>(dataset);
        switch (this->m_dataset) {
        case Dataset::Baby:
            this->m_volume = PVMVolume { "resources/Baby.pvm" };
            break;
        case Dataset::CT_Head:
            this->m_volume = PVMVolume { "resources/CT-Head.pvm" };
            break;
        case Dataset::Fuel:
            this->m_volume = PVMVolume { "resources/Fuel.pvm" };
            break;
        default:
            std::cerr << "Invalid dataset!" << std::endl;
        }
        auto extents { this->m_volume->extents() };
        glm::vec3 extents_float { extents.x, extents.y, extents.z };

        this->m_plane = SlicePlane::Axial;
        this->m_plane_offset = 0.0f;
        this->m_plane_rotation = 0.0f;
    }
    bool plane_changed = ImGui::SliderFloat("Plane offset (%)", &this->m_plane_offset, 0.0f, 100.0f);
    plane_changed |= ImGui::SliderFloat("Plane rotation", &this->m_plane_rotation, 0.0f, 360.0f);

    if (ImGui::Button("Axial")) {
        plane_changed = true;
        this->m_plane = SlicePlane::Axial;
        this->m_plane_offset = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Sagittal")) {
        plane_changed = true;
        this->m_plane = SlicePlane::Sagittal;
        this->m_plane_offset = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Coronal")) {
        plane_changed = true;
        this->m_plane = SlicePlane::Coronal;
        this->m_plane_offset = 0.0f;
    }

    ImGui::End();

    bool update_slice_texture = this->m_slice_texture_changed || dataset_changed || plane_changed;
    this->m_slice_texture_changed = false;
    if (update_slice_texture) {
        // Compute the plane.
        glm::vec4 normal { 0.0f };
        glm::vec4 top_left { 0.0f };
        glm::vec4 bottom_left { 0.0f };
        glm::vec4 bottom_right { 0.0f };

        auto extents { this->m_volume->extents() };
        glm::vec3 extents_f { static_cast<float>(extents.x), static_cast<float>(extents.y), static_cast<float>(extents.z) };
        glm::vec3 plane_max { extents_f - glm::vec3 { 1.0f } };

        switch (this->m_plane) {
        case SlicePlane::Axial: {
            normal = glm::vec4 { 0.0f, 0.0f, 1.0f, 0.0f };
            glm::vec4 offset { normal * plane_max.z * (this->m_plane_offset / 100.0f) };
            top_left = glm::vec4 { 0.0f, plane_max.y, 0.0f, 1.0f } + offset;
            bottom_left = glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f } + offset;
            bottom_right = glm::vec4 { plane_max.x, 0.0f, 0.0f, 1.0f } + offset;
            break;
        }
        case SlicePlane::Sagittal: {
            normal = glm::vec4 { 1.0f, 0.0f, 0.0f, 0.0f };
            glm::vec4 offset { normal * plane_max.x * (this->m_plane_offset / 100.0f) };
            top_left = glm::vec4 { 0.0f, 0.0f, plane_max.z, 1.0f } + offset;
            bottom_left = glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f } + offset;
            bottom_right = glm::vec4 { 0.0f, plane_max.y, 0.0f, 1.0f } + offset;
            break;
        }
        case SlicePlane::Coronal: {
            normal = glm::vec4 { 0.0f, 1.0f, 0.0f, 0.0f };
            glm::vec4 offset { normal * plane_max.y * (this->m_plane_offset / 100.0f) };
            top_left = glm::vec4 { 0.0f, 0.0f, plane_max.z, 1.0f } + offset;
            bottom_left = glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f } + offset;
            bottom_right = glm::vec4 { plane_max.x, 0.0f, 0.0f, 1.0f } + offset;
            break;
        }
        }

        auto plane_rotation = glm::radians(this->m_plane_rotation);
        glm::quat rotation { glm::quat(normal * plane_rotation) };
        glm::vec4 center { (top_left + bottom_right) / 2.0f };
        top_left = center + rotation * (top_left - center);
        bottom_left = center + rotation * (bottom_left - center);
        bottom_right = center + rotation * (bottom_right - center);

        Plane plane {
            .top_left = top_left,
            .bottom_left = bottom_left,
            .bottom_right = bottom_right,
        };

        // Allocate memory for the color buffer.
        uint32_t width { this->scaled_surface_width() };
        uint32_t height { this->scaled_surface_height() };
        size_t size { static_cast<size_t>(width) * static_cast<size_t>(height) };
        std::vector<Color> color_buffer { size, Color {} };

        // Fill the color buffer.
        this->compute_slice(*this->m_volume, color_buffer, plane, width, height);

        // Copy the color buffer to the gpu texture.
        wgpu::Device& device { this->device() };
        wgpu::Queue queue { device.getQueue() };

        wgpu::ImageCopyTexture destination { wgpu::Default };
        destination.texture = this->m_slice_texture;
        destination.mipLevel = 0;
        destination.origin = wgpu::Origin3D { 0, 0, 0 };
        destination.aspect = wgpu::TextureAspect::All;
        wgpu::TextureDataLayout data_layout { wgpu::Default };
        data_layout.offset = 0;
        data_layout.bytesPerRow = width * 4;
        data_layout.rowsPerImage = height;
        wgpu::Extent3D write_size { width, height, 1 };
        queue.writeTexture(destination, &color_buffer.data()->r, 4 * size, data_layout, write_size);
    }

    wgpu::TextureView slice_texture_view = this->m_slice_texture.createView();

    std::array<wgpu::BindGroupEntry, 1> bind_group_entries { wgpu::Default };
    bind_group_entries[0].binding = 0;
    bind_group_entries[0].textureView = slice_texture_view;

    wgpu::BindGroupDescriptor bind_group_desc { wgpu::Default };
    bind_group_desc.layout = this->m_bind_group_layout;
    bind_group_desc.entryCount = bind_group_entries.size();
    bind_group_desc.entries = bind_group_entries.data();
    wgpu::BindGroup bind_group = this->device().createBindGroup(bind_group_desc);

    auto color_attachments = std::array { wgpu::RenderPassColorAttachment { wgpu::Default } };
    color_attachments[0].view = frame;
    color_attachments[0].loadOp = wgpu::LoadOp::Clear;
    color_attachments[0].storeOp = wgpu::StoreOp::Store;
    color_attachments[0].clearValue = wgpu::Color { 0.45f, 0.55f, 0.60f, 1.0f };

    wgpu::RenderPassDescriptor pass_desc { wgpu::Default };
    pass_desc.colorAttachmentCount = color_attachments.size();
    pass_desc.colorAttachments = color_attachments.data();
    auto pass_encoder = encoder.beginRenderPass(pass_desc);
    if (!pass_encoder) {
        std::cerr << "Could not create render pass!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    pass_encoder.setPipeline(this->m_render_pipeline);
    pass_encoder.setBindGroup(0, bind_group, 0, nullptr);
    pass_encoder.draw(6, 1, 0, 0);
    pass_encoder.end();
    pass_encoder.release();
    bind_group.release();
}

void Application::on_resize()
{
    ApplicationBase::on_resize();
    this->init_slice_texture();
}

void Application::init_slice_texture()
{
    if (this->m_slice_texture) {
        this->m_slice_texture.release();
        this->m_slice_texture = { nullptr };
    }

    wgpu::Device& device = this->device();
    uint32_t width { this->scaled_surface_width() };
    uint32_t height { this->scaled_surface_height() };

    wgpu::TextureDescriptor desc { wgpu::Default };
    desc.label = "slice texture";
    desc.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::TextureBinding;
    desc.dimension = wgpu::TextureDimension::_2D;
    desc.size = wgpu::Extent3D { width, height, 1 };
    desc.format = wgpu::TextureFormat::RGBA8Unorm;
    desc.mipLevelCount = 1;
    desc.sampleCount = 1;
    desc.viewFormatCount = 0;
    desc.viewFormats = nullptr;
    this->m_slice_texture = device.createTexture(desc);
    this->m_slice_texture_changed = true;
    if (!this->m_slice_texture) {
        std::cerr << "Could not create slice texture!" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

float Application::interpolate_trilinear(VoxelCell cell, float t_x, float t_y, float t_z) const
{
    /// Replace with the interpolation
    return (t_x + t_y + t_z) / 3.0f;
}

Color Application::sample_transfer_function(float t) const
{
    /// Replace with the transfer function.
    return Color { .r = 0, .g = 255, .b = 255, .a = 255 };
}

Color Application::color_at_position(const PVMVolume& volume, glm::vec3 position) const
{
    /// Replace with the sampling.
    return Color { .r = 0, .g = 255, .b = 0, .a = 255 };
}

void Application::compute_slice(const PVMVolume& volume, std::span<Color> color_buffer, Plane plane,
    uint32_t buffer_width, uint32_t buffer_height) const
{
    /// Replace with the implementation of the sampling.
    std::fill(std::begin(color_buffer), std::end(color_buffer), Color { .r = 0, .g = 0, .b = 255, .a = 255 });
}