#include <pvm_volume.h>

#include <algorithm>
#include <limits>
#include <stdexcept>

#include <volumeio.h>

PVMVolume::PVMVolume(const std::filesystem::path& volume_path)
    : m_component_ranges {}
    , m_data {}
    , m_name { volume_path.string() }
    , m_size_x { 0 }
    , m_size_y { 0 }
    , m_size_z { 0 }
    , m_components { 0 }
    , m_scale_x { 0.0f }
    , m_scale_y { 0.0f }
    , m_scale_z { 0.0f }
{
#ifdef _MSC_VER
    const char* c_volume_path = this->m_name.c_str();
#else
    const char* c_volume_path = volume_path.c_str();
#endif

    auto deleter = [](unsigned char* ptr) {
        if (ptr) {
            std::free(ptr);
        }
    };

    unsigned int width, height, depth, components;
    std::unique_ptr<unsigned char[], void (*)(unsigned char*)> data {
        readPVMvolume(c_volume_path, &width, &height,
            &depth, &components, &this->m_scale_x, &this->m_scale_y, &this->m_scale_z),
        deleter
    };
    if (!data) {
        throw std::runtime_error("could not read pvm volume");
    }

    this->m_size_x = static_cast<std::size_t>(width);
    this->m_size_y = static_cast<std::size_t>(height);
    this->m_size_z = static_cast<std::size_t>(depth);
    this->m_components = static_cast<std::size_t>(components);

    size_t data_size { this->m_size_x * this->m_size_y * this->m_size_z * this->m_components };
    this->m_component_ranges.reset(new glm::vec2[this->m_components]);
    this->m_data.reset(new float[data_size]);

    for (std::size_t i { 0 }; i < this->m_components; ++i) {
        static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 required");
        glm::vec2& component { this->m_component_ranges[i] };
        component.x = std::numeric_limits<float>::infinity();
        component.y = -std::numeric_limits<float>::infinity();
    }

    // Find the minimum/maximum of all components.
    for (std::size_t i { 0 }; i < data_size; ++i) {
        float value { static_cast<float>(data[i]) };
        std::size_t component_idx { this->m_components - (i % this->m_components) - 1 };
        glm::vec2& component { this->m_component_ranges[component_idx] };
        component.x = std::min({ component.x, value });
        component.y = std::max({ component.y, value });
    }

    // Normalize the values.
    for (std::size_t i { 0 }; i < data_size; ++i) {
        float value { static_cast<float>(data[i]) };
        std::size_t component_idx { this->m_components - (i % this->m_components) - 1 };
        glm::vec2& component { this->m_component_ranges[component_idx] };
        float min = component.x;
        float max = component.y;

        float normalized = (value - min) / (max - min);
        this->m_data[i] = normalized;
    }
}

PVMVolume::PVMVolume(const PVMVolume& volume)
    : m_component_ranges { new glm::vec2[volume.m_components] }
    , m_data { new float[volume.m_size_x * volume.m_size_y * volume.m_size_z * volume.m_components] }
    , m_name { volume.m_name }
    , m_size_x { volume.m_size_x }
    , m_size_y { volume.m_size_y }
    , m_size_z { volume.m_size_z }
    , m_components { volume.m_components }
    , m_scale_x { volume.m_scale_x }
    , m_scale_y { volume.m_scale_y }
    , m_scale_z { volume.m_scale_z }
{
    size_t data_size { volume.m_size_x * volume.m_size_y * volume.m_size_z * volume.m_components };
    std::copy_n(volume.m_component_ranges.get(), volume.m_components, this->m_component_ranges.get());
    std::copy_n(volume.m_data.get(), data_size, this->m_data.get());
}

PVMVolume& PVMVolume::operator=(const PVMVolume& volume)
{
    if (this != &volume) {
        if (this->m_components != volume.m_components) {
            this->m_component_ranges.reset(new glm::vec2[volume.m_components]);
        }
        std::copy_n(volume.m_component_ranges.get(), volume.m_components, this->m_component_ranges.get());

        size_t data_size { volume.m_size_x * volume.m_size_y * volume.m_size_z * volume.m_components };
        size_t current_size { this->m_size_x * this->m_size_y * this->m_size_z * this->m_components };
        if (current_size != data_size) {
            this->m_data.reset(new float[data_size]);
        }
        std::copy_n(volume.m_data.get(), data_size, this->m_data.get());

        this->m_name = volume.m_name;
        this->m_size_x = volume.m_size_x;
        this->m_size_y = volume.m_size_y;
        this->m_size_z = volume.m_size_z;
        this->m_components = volume.m_components;
        this->m_scale_x = volume.m_scale_x;
        this->m_scale_y = volume.m_scale_y;
        this->m_scale_z = volume.m_scale_z;
    }
    return *this;
}

bool PVMVolume::is_scalar_field() const
{
    return this->m_components == 1;
}

bool PVMVolume::is_vector_field() const
{
    return this->m_components > 1;
}

std::size_t PVMVolume::components() const
{
    return this->m_components;
}

std::size_t PVMVolume::size_x() const
{
    return this->m_size_x;
}

std::size_t PVMVolume::size_y() const
{
    return this->m_size_y;
}

std::size_t PVMVolume::size_z() const
{
    return this->m_size_z;
}

glm::vec<3, std::size_t> PVMVolume::extents() const
{
    return glm::vec<3, std::size_t> {
        this->m_size_x,
        this->m_size_y,
        this->m_size_z
    };
}

float PVMVolume::scale_x() const
{
    return this->m_scale_x;
}

float PVMVolume::scale_y() const
{
    return this->m_scale_y;
}

float PVMVolume::scale_z() const
{
    return this->m_scale_z;
}

glm::vec3 PVMVolume::scale() const
{
    return glm::vec3 { this->m_scale_x, this->m_scale_y, this->m_scale_z };
}

glm::vec3 PVMVolume::voxel_position_start(std::size_t x, std::size_t y, std::size_t z) const
{
    glm::vec3 idx_f { static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) };
    return idx_f * this->scale();
}

glm::vec3 PVMVolume::voxel_position_end(std::size_t x, std::size_t y, std::size_t z) const
{
    return this->voxel_position_start(x, y, z) + this->scale();
}

glm::vec3 PVMVolume::voxel_position_center(std::size_t x, std::size_t y, std::size_t z) const
{
    return this->voxel_position_start(x, y, z) + (this->scale() * 0.5f);
}

float PVMVolume::voxel(std::size_t x, std::size_t y, std::size_t z) const
{
    return this->voxel(x, y, z, 0);
}

float PVMVolume::voxel(std::size_t x, std::size_t y, std::size_t z, std::size_t component) const
{
    float value { this->voxel_normalized(x, y, z, component) };
    glm::vec2 range { this->m_component_ranges[component] };
    float start { range.x };
    float end { range.y };

    return (start * (1.0f - value)) + (end * value);
}

float PVMVolume::voxel_normalized(std::size_t x, std::size_t y, std::size_t z) const
{
    return this->voxel_normalized(x, y, z, 0);
}

float PVMVolume::voxel_normalized(std::size_t x, std::size_t y, std::size_t z, std::size_t component) const
{
    if (x >= this->m_size_x) {
        throw std::out_of_range("x coordinate out of range");
    }
    if (y >= this->m_size_y) {
        throw std::out_of_range("y coordinate out of range");
    }
    if (z >= this->m_size_z) {
        throw std::out_of_range("z coordinate out of range");
    }
    if (component >= this->m_components) {
        throw std::out_of_range("component index out of range");
    }

    std::size_t voxel_index { (x + y * this->m_size_x + z * this->m_size_x * this->m_size_y) * this->m_components };
    std::size_t component_index { voxel_index + (this->m_components - 1 - component) };
    return this->m_data[component_index];
}
