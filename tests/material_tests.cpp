#include "visual/renderfixture.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <renderer/material.hpp>
#include <renderer/shader.hpp>
#include <renderer/texture.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <variant>

TEST_CASE("Material exposes safe defaults and validated properties", "[material]") {
    vshade::renderer::Material material;

    CHECK_FALSE(material.hasShader());
    CHECK_FALSE(material.hasAlbedoTexture());
    CHECK(material.albedoColor().r == Catch::Approx(1.0F));
    CHECK(material.albedoColor().g == Catch::Approx(1.0F));
    CHECK(material.albedoColor().b == Catch::Approx(1.0F));
    CHECK(material.albedoColor().a == Catch::Approx(1.0F));
    CHECK(material.roughness() == Catch::Approx(0.5F));
    CHECK(material.metallic() == Catch::Approx(0.0F));
    CHECK(material.shading() == vshade::renderer::MaterialShading::Unlit);

    material.setAlbedoColor({0.2F, 0.4F, 0.6F, 0.8F});
    material.setRoughness(0.75F);
    material.setMetallic(0.25F);
    material.setShading(vshade::renderer::MaterialShading::Lit);

    CHECK(material.albedoColor().b == Catch::Approx(0.6F));
    CHECK(material.roughness() == Catch::Approx(0.75F));
    CHECK(material.metallic() == Catch::Approx(0.25F));
    CHECK(material.shading() == vshade::renderer::MaterialShading::Lit);

    CHECK_THROWS_AS(material.setRoughness(-0.01F), std::invalid_argument);
    CHECK_THROWS_AS(material.setRoughness(1.01F), std::invalid_argument);
    CHECK_THROWS_AS(
        material.setMetallic(std::numeric_limits<float>::quiet_NaN()),
        std::invalid_argument
    );
}

TEST_CASE("Material stores typed custom shader parameters", "[material]") {
    vshade::renderer::Material material;
    material.parameters().set("effectStrength", 0.75F);
    material.parameters().set("effectColor", vshade::math::Vec3{0.2F, 0.4F, 0.8F});

    CHECK(material.parameters().size() == 2);
    CHECK(material.parameters().contains("effectStrength"));
    const auto& strength = material.parameters().values().at("effectStrength");
    CHECK(std::get<float>(strength) == Catch::Approx(0.75F));

    material.parameters().set("effectStrength", 1.0F);
    CHECK(material.parameters().size() == 2);
    CHECK(std::get<float>(material.parameters().values().at("effectStrength")) ==
          Catch::Approx(1.0F));
    CHECK(material.parameters().erase("effectColor"));
    CHECK_FALSE(material.parameters().contains("effectColor"));
    CHECK_THROWS_AS(material.parameters().set("", 1.0F), std::invalid_argument);
}

TEST_CASE("Material retains shared texture resources", "[material][opengl]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    constexpr std::array<std::uint8_t, 4> pixel{120, 160, 220, 255};
    auto texture = std::make_shared<vshade::renderer::Texture2D>(
        1,
        1,
        vshade::renderer::TextureFormat::RGBA8,
        pixel.data()
    );
    const std::weak_ptr<vshade::renderer::Texture2D> observedTexture = texture;
    const std::filesystem::path shaderDirectory{VSHADE_SANDBOX_SHADER_DIR};
    auto shader = std::make_shared<vshade::renderer::Shader>(
        vshade::renderer::Shader::fromFiles(
            "material-test-sandbox-shader",
            shaderDirectory / "shader.vs",
            shaderDirectory / "shader.fs"
        )
    );
    const std::weak_ptr<vshade::renderer::Shader> observedShader = shader;

    vshade::renderer::Material material;
    material.setAlbedoTexture(texture);
    material.setShader(shader);
    texture.reset();
    shader.reset();

    CHECK(material.hasAlbedoTexture());
    CHECK(material.hasShader());
    CHECK_FALSE(observedTexture.expired());
    CHECK_FALSE(observedShader.expired());
    REQUIRE(material.albedoTexture());
    CHECK(material.albedoTexture()->width() == 1);
    CHECK(material.albedoTexture()->height() == 1);
}
