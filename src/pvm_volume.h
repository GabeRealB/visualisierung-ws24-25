#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

#pragma warning(push, 3)
#include <glm/glm.hpp>
#pragma warning(pop)

/**
 * Simple helper class for loading and handling PVM volumes.
 */
class PVMVolume {
public:
    PVMVolume(const std::filesystem::path& volume_path);
    PVMVolume(const PVMVolume&);
    PVMVolume(PVMVolume&&) noexcept = default;
    ~PVMVolume() noexcept = default;

    PVMVolume& operator=(const PVMVolume&);
    PVMVolume& operator=(PVMVolume&&) noexcept = default;

    /**
     * Checks if the volume is a scalar field.
     * @return volume is a scalar field
     */
    bool is_scalar_field() const;

    /**
     * Checks if the volume is a vector field.
     * @return volume is a vector field
     */
    bool is_vector_field() const;

    /**
     * Returns the number of components for each voxel.
     * @return components for each voxel
     */
    std::size_t components() const;

    /**
     * Returns the number of voxels in the x direction.
     * @return number of voxels
     */
    std::size_t size_x() const;

    /**
     * Returns the number of voxels in the y direction.
     * @return number of voxels
     */
    std::size_t size_y() const;

    /**
     * Returns the number of voxels in the z direction.
     * @return number of voxels
     */
    std::size_t size_z() const;

    /**
    * Returns the extents (size_x, size_y, size_z) of the volume.
    * @return volume extents
    */
    glm::vec<3, std::size_t> extents() const;

    /**
     * Returns the size of a voxel in the x direction.
     * @return voxel size
     */
    float scale_x() const;

    /**
     * Returns the size of a voxel in the y direction.
     * @return voxel size
     */
    float scale_y() const;

    /**
     * Returns the size of a voxel in the z direction.
     * @return voxel size
     */
    float scale_z() const;

    /**
     * Returns the size of a voxel.
     * @return voxel size
     */
    glm::vec3 scale() const;

    /**
     * Returns the start position of the voxel.
     * @param x x grid position
     * @param y y grid position
     * @param z z grid position
     * @return voxel start position
     */
    glm::vec3 voxel_position_start(std::size_t x, std::size_t y, std::size_t z) const;

    /**
     * Returns the end position of the voxel.
     * @param x x grid position
     * @param y y grid position
     * @param z z grid position
     * @return voxel end position
     */
    glm::vec3 voxel_position_end(std::size_t x, std::size_t y, std::size_t z) const;

    /**
     * Returns the center position of the voxel.
     * @param x x grid position
     * @param y y grid position
     * @param z z grid position
     * @return voxel center position
     */
    glm::vec3 voxel_position_center(std::size_t x, std::size_t y, std::size_t z) const;

    /**
     * Returns the non-normalized voxel value of the first component
     * @param x x grid position
     * @param y y grid position
     * @param z z grid position
     * @return voxel value
     */
    float voxel(std::size_t x, std::size_t y, std::size_t z) const;

    /**
     * Returns the non-normalized voxel value
     * @param x x grid position
     * @param y y grid position
     * @param z z grid position
     * @param component voxel component
     * @return voxel value
     */
    float voxel(std::size_t x, std::size_t y, std::size_t z, std::size_t component) const;

    /**
     * Returns the normalized voxel value of the first component
     * @param x x grid position
     * @param y y grid position
     * @param z z grid position
     * @return normalized voxel value
     */
    float voxel_normalized(std::size_t x, std::size_t y, std::size_t z) const;

    /**
     * Returns the normalized voxel value
     * @param x x grid position
     * @param y y grid position
     * @param z z grid position
     * @param component voxel component
     * @return normalized voxel value
     */
    float voxel_normalized(std::size_t x, std::size_t y, std::size_t z, std::size_t component) const;

private:
    std::unique_ptr<glm::vec2[]> m_component_ranges;
    std::unique_ptr<float[]> m_data;
    std::string m_name;
    std::size_t m_size_x;
    std::size_t m_size_y;
    std::size_t m_size_z;
    std::size_t m_components;
    float m_scale_x;
    float m_scale_y;
    float m_scale_z;
};