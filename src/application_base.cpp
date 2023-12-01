#include <boilerplate/application_base.h>

#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

#include <glfw3webgpu.h>

ApplicationBase::ApplicationBase(const char* title)
    : m_window { nullptr }
    , m_instance { nullptr }
    , m_surface { nullptr }
    , m_error_callback { nullptr }
    , m_device { nullptr }
    , m_surface_format { wgpu::TextureFormat::Undefined }
    , m_window_width { 640 }
    , m_window_height { 480 }
{
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
    auto adapter = this->m_instance.requestAdapter({});
    if (!adapter) {
        std::cerr << "Could not create WebGPU adapter!" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    this->inspect_adapter(adapter);
    this->inspect_surface(adapter, this->m_surface);
    this->m_surface_format = this->m_surface.getPreferredFormat(adapter);

    wgpu::DeviceDescriptor device_desc { wgpu::Default };
    device_desc.label = "Application Device";
    device_desc.requiredLimits = nullptr;
    device_desc.defaultQueue.label = "Default application queue";
    this->m_device = adapter.requestDevice(device_desc);
    adapter.release();

    if (!this->m_device) {
        std::cerr << "Could not create WebGPU device!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    auto on_device_error = [](wgpu::ErrorType type, const char* message) {
        std::cerr << "Uncaptured device error: type " << type;
        if (message) {
            std::cerr << " (" << message << ")";
        }
        std::cerr << std::endl;
    };
    this->m_error_callback = this->m_device.setUncapturedErrorCallback(on_device_error);

    this->configure_surface();
}

ApplicationBase::ApplicationBase(ApplicationBase&& app)
    : m_window { std::exchange(app.m_window, nullptr) }
    , m_instance { std::exchange(app.m_instance, nullptr) }
    , m_surface { std::exchange(app.m_surface, nullptr) }
    , m_error_callback { std::exchange(app.m_error_callback, nullptr) }
    , m_device { std::exchange(app.m_device, nullptr) }
    , m_surface_format { std::exchange(app.m_surface_format, wgpu::TextureFormat::Undefined) }
    , m_window_width { std::exchange(app.m_window_width, 0) }
    , m_window_height { std::exchange(app.m_window_height, 0) }
{
    if (this->m_window) {
        glfwSetWindowUserPointer(this->m_window, static_cast<void*>(this));
    }
}

ApplicationBase::~ApplicationBase()
{
    if (this->m_device) {
        this->m_device.destroy();
        this->m_device.release();
    }

    if (this->m_error_callback) {
        this->m_error_callback.reset();
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
    if (!this->m_window) {
        std::cerr << "No window associated with the application!" << std::endl;
        return;
    }

    if (!this->m_device) {
        std::cerr << "No device associated with the application!" << std::endl;
        return;
    }

    auto queue = this->m_device.getQueue();
    while (!glfwWindowShouldClose(this->m_window)) {
        glfwPollEvents();

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

        wgpu::CommandEncoderDescriptor desc { wgpu::Default };
        auto command_encoder = this->m_device.createCommandEncoder(desc);
        if (!command_encoder) {
            std::cerr << "Could create the frame command encoder" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        this->update(command_encoder);
        this->render(command_encoder, surface_texture_view);

        auto command_buffer = command_encoder.finish({ wgpu::Default });
        queue.submit(command_buffer);
        this->m_surface.present();

        command_buffer.release();
        command_encoder.release();
        surface_texture_view.release();
        texture.release();
    }
}

void ApplicationBase::update(wgpu::CommandEncoder&) { }

void ApplicationBase::render(wgpu::CommandEncoder&, wgpu::TextureView&) { }

void ApplicationBase::on_resize()
{
    if (!this->m_window) {
        return;
    }

    int width, height;
    glfwGetWindowSize(this->m_window, &width, &height);

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
    config.width = this->m_window_width;
    config.height = this->m_window_height;
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

    wgpu::AdapterProperties properties {};
    adapter.getProperties(&properties);
    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << properties.vendorID << std::endl;
    std::cout << " - deviceID: " << properties.deviceID << std::endl;
    std::cout << " - name: " << properties.name << std::endl;
    if (properties.driverDescription) {
        std::cout << " - driverDescription: " << properties.driverDescription << std::endl;
    }
    std::cout << " - adapterType: " << properties.adapterType << std::endl;
    std::cout << " - backendType: " << properties.backendType << std::endl;
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