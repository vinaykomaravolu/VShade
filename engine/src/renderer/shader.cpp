#include "renderer/shader.hpp"

#include "core/filesystem.hpp"
#include "renderer/renderer.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace vshade::renderer {
namespace {

GLuint currentProgram = 0;

void requireRenderer() {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Shader operation requires an initialized renderer");
    }
}

GLuint compileStage(const GLenum type, const std::string_view source, const std::string& shaderName) {
    if (source.size() > static_cast<std::size_t>(std::numeric_limits<GLint>::max())) {
        throw std::overflow_error("Shader source is too large: " + shaderName);
    }

    const GLuint shader = glCreateShader(type);
    const char* sourceData = source.data();
    const auto sourceLength = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &sourceData, &sourceLength);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> log(static_cast<std::size_t>(std::max(logLength, 1)));
    glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    glDeleteShader(shader);
    throw std::runtime_error("Failed to compile shader '" + shaderName + "': " + log.data());
}

void deleteProgram(std::uint32_t& rendererId) noexcept {
    if (rendererId != 0 && Renderer::isInitialized()) {
        if (currentProgram == rendererId) {
            glUseProgram(0);
            currentProgram = 0;
        }
        glDeleteProgram(rendererId);
    }
    rendererId = 0;
}

} // namespace

Shader::Shader(
    std::string name,
    const std::string_view vertexSource,
    const std::string_view fragmentSource
) : m_name(std::move(name)) {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Renderer must be initialized before creating a shader");
    }

    const ShaderStage vertexShader{
        compileStage(GL_VERTEX_SHADER, vertexSource, m_name)
    };
    const ShaderStage fragmentShader{
        compileStage(GL_FRAGMENT_SHADER, fragmentSource, m_name)
    };

    m_rendererId = glCreateProgram();
    glAttachShader(m_rendererId, vertexShader.rendererId());
    glAttachShader(m_rendererId, fragmentShader.rendererId());
    glLinkProgram(m_rendererId);

    GLint linked = GL_FALSE;
    glGetProgramiv(m_rendererId, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return;
    }

    GLint logLength = 0;
    glGetProgramiv(m_rendererId, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> log(static_cast<std::size_t>(std::max(logLength, 1)));
    glGetProgramInfoLog(m_rendererId, logLength, nullptr, log.data());
    const std::string message = "Failed to link shader '" + m_name + "': " + log.data();
    glDeleteProgram(m_rendererId);
    m_rendererId = 0;
    throw std::runtime_error(message);
}

Shader::ShaderStage::ShaderStage(const std::uint32_t id) noexcept
    : m_id(id) {}

Shader::ShaderStage::~ShaderStage() {
    if (m_id != 0) {
        glDeleteShader(m_id);
    }
}

std::uint32_t Shader::ShaderStage::rendererId() const noexcept {
    return m_id;
}

Shader::~Shader() {
    deleteProgram(m_rendererId);
}

Shader::Shader(Shader&& other) noexcept
    : m_name(std::move(other.m_name)),
      m_rendererId(std::exchange(other.m_rendererId, 0)),
      m_uniformLocations(std::move(other.m_uniformLocations)) {}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        deleteProgram(m_rendererId);
        m_name = std::move(other.m_name);
        m_rendererId = std::exchange(other.m_rendererId, 0);
        m_uniformLocations = std::move(other.m_uniformLocations);
    }
    return *this;
}

Shader Shader::fromFiles(
    std::string name,
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath
) {
    const std::string vertexSource = core::filesystem::readTextFile(vertexPath);
    const std::string fragmentSource = core::filesystem::readTextFile(fragmentPath);
    return Shader(std::move(name), vertexSource, fragmentSource);
}

void Shader::bind() const {
    requireRenderer();
    if (currentProgram != m_rendererId) {
        glUseProgram(m_rendererId);
        currentProgram = m_rendererId;
    }
}

void Shader::unbind() {
    requireRenderer();
    if (currentProgram != 0) {
        glUseProgram(0);
        currentProgram = 0;
    }
}

void Shader::setInt(const std::string_view name, const int value) {
    bind();
    glUniform1i(uniformLocation(name), value);
}

void Shader::setFloat(const std::string_view name, const float value) {
    bind();
    glUniform1f(uniformLocation(name), value);
}

void Shader::setVec2(const std::string_view name, const math::Vec2& value) {
    bind();
    glUniform2fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string_view name, const math::Vec3& value) {
    bind();
    glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string_view name, const math::Vec4& value) {
    bind();
    glUniform4fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string_view name, const math::Mat4& value) {
    bind();
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

const std::string& Shader::name() const noexcept {
    return m_name;
}

std::uint32_t Shader::rendererId() const noexcept {
    return m_rendererId;
}

int Shader::uniformLocation(const std::string_view name) {
    requireRenderer();
    const std::string key(name);
    if (const auto found = m_uniformLocations.find(key); found != m_uniformLocations.end()) {
        return found->second;
    }

    const int location = glGetUniformLocation(m_rendererId, key.c_str());
    m_uniformLocations.emplace(key, location);
    return location;
}

bool Shader::hasUniform(const std::string_view name) {
    return uniformLocation(name) >= 0;
}

void Shader::resetBindingCache() noexcept {
    if (Renderer::isInitialized() && currentProgram != 0) {
        glUseProgram(0);
    }
    currentProgram = 0;
}

} // namespace vshade::renderer
