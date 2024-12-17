#include "GLFW/glfw3.h"
#include <application_base.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

#include <glfw3webgpu.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>

ApplicationBase::ApplicationBase(const char* title)
    : m_window { nullptr }
    , m_imgui_context { nullptr }
    , m_instance { nullptr }
    , m_surface { nullptr }
    , m_device { nullptr }
    , m_surface_format { wgpu::TextureFormat::Undefined }
    , m_window_width { 1280 }
    , m_window_height { 720 }
    , m_window_width_scale { 1.0f }
    , m_window_height_scale { 1.0f }
{
    // Init GLFW and window
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    this->m_window = glfwCreateWindow(this->m_window_width, this->m_window_height, title, nullptr, nullptr);
    if (!this->m_window) {
        std::cerr << "Could not create a window!" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    glfwGetWindowContentScale(this->m_window, &this->m_window_width_scale, &this->m_window_height_scale);

    glfwSetWindowUserPointer(this->m_window, static_cast<void*>(this));
    glfwSetFramebufferSizeCallback(this->m_window, [](GLFWwindow* window, int width, int height) {
        if (width == 0 && height == 0) {
            return;
        }

        auto application = static_cast<ApplicationBase*>(glfwGetWindowUserPointer(window));
        if (application) {
            application->on_resize();
        }
    });

    // Init WebGPU
    this->m_instance = wgpu::createInstance({ wgpu::Default });
    if (!this->m_instance) {
        std::cerr << "Could not create WebGPU instance!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    this->m_surface = wgpu::Surface { glfwGetWGPUSurface(this->m_instance, this->m_window) };
    if (!this->m_surface) {
        std::cerr << "Could not create WebGPU surface!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    wgpu::RequestAdapterOptions adapter_opts { wgpu::Default };
    adapter_opts.compatibleSurface = this->m_surface;
    auto adapter = this->m_instance.requestAdapter(adapter_opts);
    if (!adapter) {
        std::cerr << "Could not create WebGPU adapter!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    wgpu::SurfaceCapabilities surface_cap { wgpu::Default };
    this->m_surface.getCapabilities(adapter, &surface_cap);
    this->m_surface_format = surface_cap.formats[0];

#if SHOW_WEBGPU_INFO != 0
    this->inspect_adapter(adapter);
    this->inspect_surface(adapter, this->m_surface);
#endif

    // Default limits from https://www.w3.org/TR/webgpu/#limits
    wgpu::RequiredLimits device_limits { wgpu::Default };
    device_limits.limits.maxTextureDimension1D = 8192;
    device_limits.limits.maxTextureDimension2D = 8192;
    device_limits.limits.maxTextureDimension3D = 2048;
    device_limits.limits.maxTextureArrayLayers = 256;
    device_limits.limits.maxBindGroups = 4;
    device_limits.limits.maxBindGroupsPlusVertexBuffers = 24;
    device_limits.limits.maxBindingsPerBindGroup = 1000;
    device_limits.limits.maxDynamicUniformBuffersPerPipelineLayout = 8;
    device_limits.limits.maxDynamicStorageBuffersPerPipelineLayout = 4;
    device_limits.limits.maxSampledTexturesPerShaderStage = 16;
    device_limits.limits.maxSamplersPerShaderStage = 16;
    device_limits.limits.maxStorageBuffersPerShaderStage = 8;
    device_limits.limits.maxStorageTexturesPerShaderStage = 4;
    device_limits.limits.maxUniformBuffersPerShaderStage = 12;
    device_limits.limits.maxUniformBufferBindingSize = 64 << 10;
    device_limits.limits.maxStorageBufferBindingSize = 128 << 20;
    device_limits.limits.minUniformBufferOffsetAlignment = 256;
    device_limits.limits.minStorageBufferOffsetAlignment = 256;
    device_limits.limits.maxVertexBuffers = 8;
    device_limits.limits.maxBufferSize = 256 << 20;
    device_limits.limits.maxVertexAttributes = 16;
    device_limits.limits.maxVertexBufferArrayStride = 2048;
    device_limits.limits.maxInterStageShaderComponents = 60;
    device_limits.limits.maxInterStageShaderVariables = 16;
    device_limits.limits.maxColorAttachments = 8;
    device_limits.limits.maxColorAttachmentBytesPerSample = 32;
    device_limits.limits.maxComputeWorkgroupStorageSize = 16 << 10;
    device_limits.limits.maxComputeInvocationsPerWorkgroup = 256;
    device_limits.limits.maxComputeWorkgroupSizeX = 256;
    device_limits.limits.maxComputeWorkgroupSizeY = 256;
    device_limits.limits.maxComputeWorkgroupSizeZ = 64;
    device_limits.limits.maxComputeWorkgroupsPerDimension = 65535;

    auto on_device_error = [](WGPUErrorType type, const char* message, void*) {
        std::cerr << "Uncaptured device error: type " << type;
        if (message) {
            std::cerr << " (" << message << ")";
        }
        std::cerr << std::endl;
    };

    wgpu::DeviceDescriptor device_desc { wgpu::Default };
    device_desc.label = "Application Device";
    device_desc.requiredLimits = &device_limits;
    device_desc.defaultQueue.label = "Default application queue";
    device_desc.uncapturedErrorCallbackInfo.callback = on_device_error;
    this->m_device = adapter.requestDevice(device_desc);
    adapter.release();

    if (!this->m_device) {
        std::cerr << "Could not create WebGPU device!" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    this->configure_surface();

    // Init Dear ImGUI
    IMGUI_CHECKVERSION();
    this->m_imgui_context = ImGui::CreateContext();
    if (!this->m_imgui_context) {
        std::cerr << "Could not create Dear ImGui context!" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    ImGui::SetCurrentContext(this->m_imgui_context);
    ImGui_ImplGlfw_InitForOther(this->m_window, true);
    ImGui_ImplWGPU_Init(this->m_device, 3, this->m_surface_format);
}

ApplicationBase::ApplicationBase(ApplicationBase&& app)
    : m_window { std::exchange(app.m_window, nullptr) }
    , m_imgui_context { std::exchange(app.m_imgui_context, nullptr) }
    , m_instance { std::exchange(app.m_instance, nullptr) }
    , m_surface { std::exchange(app.m_surface, nullptr) }
    , m_device { std::exchange(app.m_device, nullptr) }
    , m_surface_format { std::exchange(app.m_surface_format, wgpu::TextureFormat::Undefined) }
    , m_window_width { std::exchange(app.m_window_width, 0) }
    , m_window_height { std::exchange(app.m_window_height, 0) }
    , m_window_width_scale { std::exchange(app.m_window_width_scale, 1.0f) }
    , m_window_height_scale { std::exchange(app.m_window_height_scale, 1.0f) }
{
    if (this->m_window) {
        glfwSetWindowUserPointer(this->m_window, static_cast<void*>(this));
    }
}

ApplicationBase::~ApplicationBase()
{
    if (this->m_imgui_context) {
        ImGui::SetCurrentContext(this->m_imgui_context);
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(this->m_imgui_context);
    }

    if (this->m_device) {
        this->m_device.destroy();
        this->m_device.release();
    }

    if (this->m_surface) {
        this->m_surface.release();
    }

    if (this->m_instance) {
        this->m_instance.release();
    }

    if (this->m_window) {
        glfwDestroyWindow(this->m_window);
        glfwTerminate();
    }
}

void ApplicationBase::run()
{
    // Check that everything is initialized.
    if (!this->m_window) {
        std::cerr << "No window associated with the application!" << std::endl;
        return;
    }
    if (!this->m_imgui_context) {
        std::cerr << "No Dear ImGui context associated with the application!" << std::endl;
        return;
    }
    if (!this->m_device) {
        std::cerr << "No device associated with the application!" << std::endl;
        return;
    }

    // Bind the Dear ImGui context.
    ImGui::SetCurrentContext(this->m_imgui_context);

    auto queue = this->m_device.getQueue();
    while (!glfwWindowShouldClose(this->m_window)) {
        glfwPollEvents();

        // Get a render target texture.
        wgpu::SurfaceTexture surface_texture { wgpu::Default };
        this->m_surface.getCurrentTexture(&surface_texture);
        wgpu::Texture texture { surface_texture.texture };
        switch (surface_texture.status) {
        case WGPUSurfaceGetCurrentTextureStatus_Success:
            break;
        case WGPUSurfaceGetCurrentTextureStatus_Timeout:
        case WGPUSurfaceGetCurrentTextureStatus_Outdated:
        case WGPUSurfaceGetCurrentTextureStatus_Lost:
            if (texture) {
                texture.release();
            }
            this->configure_surface();
            continue;
        case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
        case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
        case WGPUSurfaceGetCurrentTextureStatus_Force32:
        default:
            std::cerr << "Could not acquire the current surface texture" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        auto surface_texture_view = texture.createView();

        // Init a Dear ImGui frame.
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Init a command encoder for the frame.
        wgpu::CommandEncoderDescriptor desc { wgpu::Default };
        auto command_encoder = this->m_device.createCommandEncoder(desc);
        if (!command_encoder) {
            std::cerr << "Could create the frame command encoder" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        this->on_frame(command_encoder, surface_texture_view);

        // Finish the Dear ImGui frame.
        auto color_attachments = std::array { wgpu::RenderPassColorAttachment { wgpu::Default } };
        color_attachments[0].view = surface_texture_view;
        color_attachments[0].loadOp = wgpu::LoadOp::Load;
        color_attachments[0].storeOp = wgpu::StoreOp::Store;
        color_attachments[0].clearValue = wgpu::Color { 0.0, 1.0, 0.0, 1.0 };

        wgpu::RenderPassDescriptor imgui_pass_desc { wgpu::Default };
        imgui_pass_desc.colorAttachmentCount = color_attachments.size();
        imgui_pass_desc.colorAttachments = color_attachments.data();
        auto imgui_pass_encoder = command_encoder.beginRenderPass(imgui_pass_desc);
        if (!imgui_pass_encoder) {
            std::cerr << "Could not create Dear ImGui render pass!" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), imgui_pass_encoder);
        imgui_pass_encoder.end();

        // Enqueue comands.
        auto command_buffer = command_encoder.finish({ wgpu::Default });
        imgui_pass_encoder.release();
        command_encoder.release();

        queue.submit(command_buffer);
        this->m_surface.present();

        command_buffer.release();
        surface_texture_view.release();
        texture.release();
    }
}

void ApplicationBase::on_frame(wgpu::CommandEncoder&, wgpu::TextureView&) { }

void ApplicationBase::on_resize()
{
    if (!this->m_window) {
        return;
    }

    int width, height;
    glfwGetWindowSize(this->m_window, &width, &height);
    glfwGetWindowContentScale(this->m_window, &this->m_window_width_scale, &this->m_window_height_scale);

    if ((width == 0 && height == 0) || (static_cast<uint32_t>(width) == this->m_window_width && static_cast<uint32_t>(height) == this->m_window_height)) {
        return;
    }
    this->m_window_width = static_cast<uint32_t>(width);
    this->m_window_height = static_cast<uint32_t>(height);
    this->configure_surface();
}

wgpu::Device& ApplicationBase::device()
{
    return this->m_device;
}

const wgpu::Device& ApplicationBase::device() const
{
    return this->m_device;
}

uint32_t ApplicationBase::scaled_surface_width() const
{
    return static_cast<std::uint32_t>(this->m_window_width * this->m_window_width_scale);
}

uint32_t ApplicationBase::scaled_surface_height() const
{
    return static_cast<std::uint32_t>(this->m_window_height * this->m_window_height_scale);
}

wgpu::TextureFormat ApplicationBase::surface_format() const
{
    return this->m_surface_format;
}

void ApplicationBase::configure_surface()
{
    if (!this->m_device || !this->m_surface) {
        return;
    }

    wgpu::SurfaceConfiguration config { wgpu::Default };
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.format = this->m_surface_format;
    config.width = this->scaled_surface_width();
    config.height = this->scaled_surface_height();
    config.presentMode = wgpu::PresentMode::Fifo;
    config.alphaMode = wgpu::CompositeAlphaMode::Opaque;
    config.device = this->m_device;
    this->m_surface.configure(config);
}

void ApplicationBase::inspect_adapter(wgpu::Adapter& adapter) const
{
    std::vector<wgpu::FeatureName> features {};
    features.resize(adapter.enumerateFeatures(nullptr), wgpu::FeatureName::Undefined);
    adapter.enumerateFeatures(features.data());

    std::cout << "Adapter features:" << std::endl;
    for (const auto& feature : features) {
        std::cout << " - " << feature << std::endl;
    }

    wgpu::SupportedLimits limits {};
    if (adapter.getLimits(&limits)) {
        std::cout << "Adapter limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
        std::cout << " - maxBindGroups: " << limits.limits.maxBindGroups << std::endl;
        std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
        std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
        std::cout << " - maxSampledTexturesPerShaderStage: " << limits.limits.maxSampledTexturesPerShaderStage << std::endl;
        std::cout << " - maxSamplersPerShaderStage: " << limits.limits.maxSamplersPerShaderStage << std::endl;
        std::cout << " - maxStorageBuffersPerShaderStage: " << limits.limits.maxStorageBuffersPerShaderStage << std::endl;
        std::cout << " - maxStorageTexturesPerShaderStage: " << limits.limits.maxStorageTexturesPerShaderStage << std::endl;
        std::cout << " - maxUniformBuffersPerShaderStage: " << limits.limits.maxUniformBuffersPerShaderStage << std::endl;
        std::cout << " - maxUniformBufferBindingSize: " << limits.limits.maxUniformBufferBindingSize << std::endl;
        std::cout << " - maxStorageBufferBindingSize: " << limits.limits.maxStorageBufferBindingSize << std::endl;
        std::cout << " - minUniformBufferOffsetAlignment: " << limits.limits.minUniformBufferOffsetAlignment << std::endl;
        std::cout << " - minStorageBufferOffsetAlignment: " << limits.limits.minStorageBufferOffsetAlignment << std::endl;
        std::cout << " - maxVertexBuffers: " << limits.limits.maxVertexBuffers << std::endl;
        std::cout << " - maxVertexAttributes: " << limits.limits.maxVertexAttributes << std::endl;
        std::cout << " - maxVertexBufferArrayStride: " << limits.limits.maxVertexBufferArrayStride << std::endl;
        std::cout << " - maxInterStageShaderComponents: " << limits.limits.maxInterStageShaderComponents << std::endl;
        std::cout << " - maxComputeWorkgroupStorageSize: " << limits.limits.maxComputeWorkgroupStorageSize << std::endl;
        std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.limits.maxComputeInvocationsPerWorkgroup << std::endl;
        std::cout << " - maxComputeWorkgroupSizeX: " << limits.limits.maxComputeWorkgroupSizeX << std::endl;
        std::cout << " - maxComputeWorkgroupSizeY: " << limits.limits.maxComputeWorkgroupSizeY << std::endl;
        std::cout << " - maxComputeWorkgroupSizeZ: " << limits.limits.maxComputeWorkgroupSizeZ << std::endl;
        std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.limits.maxComputeWorkgroupsPerDimension << std::endl;
    }

    wgpu::AdapterInfo info {};
    adapter.getInfo(&info);
    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << info.vendorID << std::endl;
    std::cout << " - vendor: " << info.vendor << std::endl;
    std::cout << " - deviceID: " << info.deviceID << std::endl;
    std::cout << " - device: " << info.device << std::endl;
    std::cout << " - driverDescription: " << info.description << std::endl;
    std::cout << " - adapterType: " << info.adapterType << std::endl;
    std::cout << " - backendType: " << info.backendType << std::endl;
}

void ApplicationBase::inspect_surface(wgpu::Adapter& adapter, wgpu::Surface& surface) const
{
    wgpu::SurfaceCapabilities capabilities { wgpu::Default };
    surface.getCapabilities(adapter, &capabilities);

    std::cout << "Surface formats:" << std::endl;
    for (size_t i { 0 }; i < capabilities.formatCount; i++) {
        std::cout << " - " << capabilities.formats[i] << std::endl;
    }

    std::cout << "Surface present modes:" << std::endl;
    for (size_t i { 0 }; i < capabilities.presentModeCount; i++) {
        std::cout << " - " << capabilities.presentModes[i] << std::endl;
    }

    std::cout << "Surface alpha modes:" << std::endl;
    for (size_t i { 0 }; i < capabilities.alphaModeCount; i++) {
        std::cout << " - " << capabilities.alphaModes[i] << std::endl;
    }

    capabilities.freeMembers();
}