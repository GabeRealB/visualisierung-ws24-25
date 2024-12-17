#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include <application_base.h>
#include <pvm_volume.h>

/**
 * A color with 8-bit rgba values.
 * Each channel is in the range [0, 255].
 */
struct Color {
    uint8_t r; // Red
    uint8_t g; // Green
    uint8_t b; // Blue
    uint8_t a; // Alpha
};

/**
 * Grid values of a voxel cell.
 * Each value is in the range [0.0, 1.0].
 */
struct VoxelCell {
    float bottom_front_left;
    float bottom_front_right;
    float bottom_back_left;
    float bottom_back_right;
    float top_front_left;
    float top_front_right;
    float top_back_left;
    float top_back_right;
};

/**
 * Extents of the slice plane.
 */
struct Plane {
    glm::vec3 top_left; // Top left coordinate
    glm::vec3 bottom_left; // Bottom left coordinate
    glm::vec3 bottom_right; // Bottom right coordinate
};

/**
 * Available datasets.
 */
enum class Dataset {
    Baby,
    CT_Head,
    Fuel
};

/**
 * Possible plane orientations.
 */
enum class SlicePlane {
    Axial,
    Sagittal,
    Coronal,
};

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
    void on_resize() override;

private:
    void init_slice_texture();

    /**
     * Computes the value inside the cell by applying a trilinear interpolation
     * of the grid values of the voxel cell.
     *
     * @param cell grid values around the cell
     * @param t_x cell-local x coordinate, in range [0.0, 1.0].
     * @param t_y cell-local y coordinate, in range [0.0, 1.0].
     * @param t_z cell-local z coordinate, in range [0.0, 1.0].
     * @return interpolated value
     */
    float interpolate_trilinear(VoxelCell cell, float t_x, float t_y, float t_z) const;

    /**
     * Samples the transfer function at a given position.
     * The transfer function is a continuous grayscale color map, where position
     * t=0.0 corresponds to black, wheras position t=1.0 is white.
     *
     * @param t sample position, in range [0.0, 1.0].
     * @return sampled color
     */
    Color sample_transfer_function(float t) const;

    /**
     * Computes the color at a given position by sampling the provided volume.
     * The position is given in voxel space, i.e., a position is inside of the
     * volume if (0.0, 0.0, 0.0) <= position <= (sizeX - 1, sizeY - 1, sizeZ - 1).
     * If the position lies inside of the given volume, the color is computed
     * by sampling the transfer function at the interpolated normalized voxel
     * value. If the position is outside of the volume, it returns the color red.
     *
     * @param volume volume to sample from
     * @param position position to sample at
     * @return sampled color
     */
    Color color_at_position(const PVMVolume& volume, glm::vec3 position) const;

    /**
     * Samples a slice from the provided volume.
     * The samples are written into the color buffer of size buffer_width * buffer_height.
     * The index 0 of the color buffer corresponds to the sample at the bottom left
     * position of the provided plane, while the last index corresponds to the upper
     * right position of the plane. The color buffer is given in row-major order, i.e.,
     * consecutive elements of a row are contiguous in memory.
     *
     * @param volume volume to slice
     * @param color_buffer buffer where the slice is written to
     * @param plane slicing plane
     * @param buffer_width width of the color buffer
     * @param buffer_height height of the color buffer
     */
    void compute_slice(const PVMVolume& volume, std::span<Color> color_buffer, Plane plane,
        uint32_t buffer_width, uint32_t buffer_height) const;

    wgpu::ShaderModule m_shader_module;
    wgpu::BindGroupLayout m_bind_group_layout;
    wgpu::PipelineLayout m_pipeline_layout;
    wgpu::RenderPipeline m_render_pipeline;
    wgpu::Texture m_slice_texture;
    bool m_slice_texture_changed;
    std::optional<PVMVolume> m_volume;
    Dataset m_dataset;
    SlicePlane m_plane;
    float m_plane_offset;
    float m_plane_rotation;
};