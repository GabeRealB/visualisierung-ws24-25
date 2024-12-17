#pragma once

#include <cstdint>
#include <memory>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <webgpu/webgpu.hpp>

#pragma warning(push, 3)
#include <glm/glm.hpp>
#pragma warning(pop)

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
    virtual void on_frame(wgpu::CommandEncoder&, wgpu::TextureView&);
    virtual void on_resize();

    wgpu::Device& device();
    const wgpu::Device& device() const;

    wgpu::TextureFormat surface_format() const;

private:
    void configure_surface();
    void inspect_adapter(wgpu::Adapter&) const;
    void inspect_surface(wgpu::Adapter&, wgpu::Surface&) const;

    GLFWwindow* m_window;
    ImGuiContext* m_imgui_context;
    wgpu::Instance m_instance;
    wgpu::Surface m_surface;
    wgpu::Device m_device;
    wgpu::TextureFormat m_surface_format;
    uint32_t m_window_width;
    uint32_t m_window_height;
    float m_window_width_scale;
    float m_window_height_scale;
};