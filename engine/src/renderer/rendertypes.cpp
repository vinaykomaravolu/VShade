#include "renderer/rendertypes.hpp"

#include <stdexcept>

namespace vshade::renderer {

std::uint32_t shaderDataTypeSize(const ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float: return sizeof(float);
        case ShaderDataType::Float2: return sizeof(float) * 2;
        case ShaderDataType::Float3: return sizeof(float) * 3;
        case ShaderDataType::Float4: return sizeof(float) * 4;
        case ShaderDataType::Int: return sizeof(int);
        case ShaderDataType::Int2: return sizeof(int) * 2;
        case ShaderDataType::Int3: return sizeof(int) * 3;
        case ShaderDataType::Int4: return sizeof(int) * 4;
    }

    throw std::invalid_argument("Unknown shader data type");
}

std::uint32_t shaderDataTypeComponentCount(const ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:
        case ShaderDataType::Int:
            return 1;
        case ShaderDataType::Float2:
        case ShaderDataType::Int2:
            return 2;
        case ShaderDataType::Float3:
        case ShaderDataType::Int3:
            return 3;
        case ShaderDataType::Float4:
        case ShaderDataType::Int4:
            return 4;
    }

    throw std::invalid_argument("Unknown shader data type");
}

} // namespace vshade::renderer
