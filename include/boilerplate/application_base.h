#pragma once

#include <cstdint>
#include <memory>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.hpp>

class ApplicationBase {
public:
    ApplicationBase(const char* title);
    ApplicationBase(const ApplicationBase&) = delete;
    ApplicationBase(ApplicationBase&&);
    virtual ~ApplicationBase();

    ApplicationBase& operator=(const ApplicationBase&) = delete;
    ApplicationBase& operator=(ApplicationBase&&) = delete;

    void run();

protected:
    virtual void update(wgpu::CommandEncoder&);
    virtual void render(wgpu::CommandEncoder&, wgpu::TextureView&);

    virtual void on_resize();

    wgpu::Device& device();
    const wgpu::Device& device() const;

    wgpu::TextureFormat surface_format() const;

private:
    void configure_surface();
    void inspect_adapter(wgpu::Adapter&) const;
    void inspect_surface(wgpu::Adapter&, wgpu::Surface&) const;

    GLFWwindow* m_window;
    wgpu::Instance m_instance;
    wgpu::Surface m_surface;
    std::unique_ptr<wgpu::ErrorCallback> m_error_callback;
    wgpu::Device m_device;
    wgpu::TextureFormat m_surface_format;
    uint32_t m_window_width;
    uint32_t m_window_height;
};