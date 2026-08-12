#pragma once

#include "math/matrix.hpp"
#include "math/vector.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace vshade::renderer {

/** @brief Owns a linked OpenGL vertex-and-fragment shader program. */
class Shader final {
public:
    /**
     * @brief Compiles and links GLSL source strings.
     * @param name Diagnostic name used in errors and debugging.
     * @param vertexSource GLSL source for the vertex stage.
     * @param fragmentSource GLSL source for the fragment stage.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::overflow_error If a source string is too large for OpenGL.
     * @throws std::runtime_error If a stage cannot compile or the program cannot link.
     */
    Shader(std::string name, std::string_view vertexSource, std::string_view fragmentSource);

    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    /**
     * @brief Loads, compiles, and links a vertex and fragment shader file.
     * @param name Diagnostic name used in errors and debugging.
     * @param vertexPath Path to the vertex-stage GLSL file.
     * @param fragmentPath Path to the fragment-stage GLSL file.
     * @return Linked shader program loaded from the supplied files.
     * @throws std::runtime_error If a file cannot be read, compilation fails, or linking fails.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::overflow_error If a source file is too large for OpenGL.
     */
    [[nodiscard]] static Shader fromFiles(
        std::string name,
        const std::filesystem::path& vertexPath,
        const std::filesystem::path& fragmentPath
    );

    /** @brief Binds this program for subsequent rendering. */
    void bind() const;

    /** @brief Unbinds the current shader program. */
    static void unbind();

    /**
     * @brief Sets a signed integer uniform.
     * @param name Uniform variable name.
     * @param value Value to upload.
     */
    void setInt(std::string_view name, int value);

    /**
     * @brief Sets a floating-point uniform.
     * @param name Uniform variable name.
     * @param value Value to upload.
     */
    void setFloat(std::string_view name, float value);

    /**
     * @brief Sets a two-component vector uniform.
     * @param name Uniform variable name.
     * @param value Value to upload.
     */
    void setVec2(std::string_view name, const math::Vec2& value);

    /**
     * @brief Sets a three-component vector uniform.
     * @param name Uniform variable name.
     * @param value Value to upload.
     */
    void setVec3(std::string_view name, const math::Vec3& value);

    /**
     * @brief Sets a four-component vector uniform.
     * @param name Uniform variable name.
     * @param value Value to upload.
     */
    void setVec4(std::string_view name, const math::Vec4& value);

    /**
     * @brief Sets a four-by-four matrix uniform.
     * @param name Uniform variable name.
     * @param value Value to upload.
     */
    void setMat4(std::string_view name, const math::Mat4& value);

    /**
     * @brief Returns the diagnostic name assigned to this shader.
     * @return Read-only reference to the diagnostic name.
     */
    [[nodiscard]] const std::string& name() const noexcept;

    /**
     * @brief Returns the native OpenGL program identifier.
     * @return OpenGL program object identifier.
     */
    [[nodiscard]] std::uint32_t rendererId() const noexcept;

private:
    /**
     * @brief Owns a temporary compiled OpenGL shader stage.
     *
     * Shader stages are deleted automatically after program linking or when an
     * exception interrupts shader construction.
     */
    class ShaderStage final {
    public:
        /**
         * @brief Takes ownership of a compiled shader-stage object.
         * @param id OpenGL shader-stage identifier to own.
         */
        explicit ShaderStage(std::uint32_t id) noexcept;
        ~ShaderStage();

        ShaderStage(const ShaderStage&) = delete;
        ShaderStage& operator=(const ShaderStage&) = delete;
        ShaderStage(ShaderStage&&) = delete;
        ShaderStage& operator=(ShaderStage&&) = delete;

        /**
         * @brief Returns the native OpenGL shader-stage identifier.
         * @return Owned OpenGL shader-stage identifier.
         */
        [[nodiscard]] std::uint32_t rendererId() const noexcept;

    private:
        std::uint32_t m_id = 0;
    };

    [[nodiscard]] int uniformLocation(std::string_view name);

    std::string m_name;
    std::uint32_t m_rendererId = 0;
    std::unordered_map<std::string, int> m_uniformLocations;
};

} // namespace vshade::renderer
