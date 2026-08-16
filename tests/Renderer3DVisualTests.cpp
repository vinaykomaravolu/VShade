#include "visual/RenderFixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <asset/AssetManager.hpp>
#include <math/Quaternion.hpp>
#include <math/Transform.hpp>
#include <renderer/Buffer.hpp>
#include <renderer/Camera.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Model.hpp>
#include <renderer/Renderer2D.hpp>
#include <renderer/Renderer3D.hpp>
#include <renderer/Texture.hpp>
#include <renderer/VertexArray.hpp>
#include <scene/Components.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneLightingSystem.hpp>
#include <scene/SceneRenderer.hpp>

#include <array>
#include <cstdint>
#include <memory>

namespace {

[[nodiscard]] vshade::renderer::Mesh createCubeMesh() {
    using vshade::renderer::MeshVertex;
    constexpr std::array<MeshVertex, 24> vertices{{
        // Front (+Z)
        {{-0.7F, -0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
        {{0.7F, -0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
        {{0.7F, 0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
        {{-0.7F, 0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
        // Back (-Z)
        {{0.7F, -0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {0.0F, 0.0F}},
        {{-0.7F, -0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {1.0F, 0.0F}},
        {{-0.7F, 0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {1.0F, 1.0F}},
        {{0.7F, 0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F}},
        // Left (-X)
        {{-0.7F, -0.7F, -0.7F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
        {{-0.7F, -0.7F, 0.7F}, {-1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
        {{-0.7F, 0.7F, 0.7F}, {-1.0F, 0.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.7F, 0.7F, -0.7F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
        // Right (+X)
        {{0.7F, -0.7F, 0.7F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.7F, -0.7F, -0.7F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.7F, 0.7F, -0.7F}, {1.0F, 0.0F, 0.0F}, {1.0F, 1.0F}},
        {{0.7F, 0.7F, 0.7F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
        // Top (+Y)
        {{-0.7F, 0.7F, 0.7F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.7F, 0.7F, 0.7F}, {0.0F, 1.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.7F, 0.7F, -0.7F}, {0.0F, 1.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.7F, 0.7F, -0.7F}, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F}},
        // Bottom (-Y)
        {{-0.7F, -0.7F, -0.7F}, {0.0F, -1.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.7F, -0.7F, -0.7F}, {0.0F, -1.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.7F, -0.7F, 0.7F}, {0.0F, -1.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.7F, -0.7F, 0.7F}, {0.0F, -1.0F, 0.0F}, {0.0F, 1.0F}},
    }};
    constexpr std::array<std::uint32_t, 36> indices{
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20,
    };

    auto vertexBuffer = std::make_shared<vshade::renderer::VertexBuffer>(
        vertices.data(),
        sizeof(vertices)
    );
    vertexBuffer->setLayout({
        {"position", vshade::renderer::ShaderDataType::Float3},
        {"normal", vshade::renderer::ShaderDataType::Float3},
        {"textureCoordinate", vshade::renderer::ShaderDataType::Float2},
        {"tangent", vshade::renderer::ShaderDataType::Float4},
    });
    auto indexBuffer = std::make_shared<vshade::renderer::IndexBuffer>(
        indices.data(),
        indices.size()
    );
    auto vertexArray = std::make_shared<vshade::renderer::VertexArray>();
    vertexArray->addVertexBuffer(std::move(vertexBuffer));
    vertexArray->setIndexBuffer(std::move(indexBuffer));
    return vshade::renderer::Mesh(std::move(vertexArray));
}

} // namespace

TEST_CASE("Renderer3D lit mesh matches its golden image", "[renderer3d][visual][opengl]") {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.025F, 0.035F, 0.06F, 1.0F}
    );

    const vshade::renderer::Mesh cube = createCubeMesh();
    constexpr std::array<std::uint8_t, 16> texturePixels{
        235, 225, 170, 255,
        55, 135, 105, 255,
        55, 135, 105, 255,
        235, 225, 170, 255,
    };
    auto texture = std::make_shared<vshade::renderer::Texture2D>(
        2,
        2,
        vshade::renderer::TextureFormat::RGBA8,
        texturePixels.data(),
        vshade::renderer::TextureFilter::Nearest,
        vshade::renderer::TextureWrap::ClampToEdge
    );

    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 100.0F);
    camera.lookAt(
        {3.2F, 2.4F, 4.2F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );

    vshade::renderer::Renderer3D::setDirectionalLight({
        .direction = {-0.55F, -1.0F, -0.35F},
        .color = {1.0F, 0.94F, 0.82F},
        .intensity = 0.95F,
    });
    vshade::renderer::Material material(vshade::renderer::MaterialShading::Lit);
    material.setAlbedoTexture(texture);
    material.setAlbedoColor({0.88F, 1.0F, 0.92F, 1.0F});
    // The neutral value preserves this test's established lighting baseline;
    // non-neutral material response is demonstrated by the sandbox.
    material.setRoughness(0.5F);
    material.setMetallic(0.0F);

    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawMesh(
        vshade::math::Transform(
            {0.0F, 0.0F, 0.0F},
            vshade::math::fromEuler({0.38F, 0.62F, 0.10F}),
            {1.0F, 1.0F, 1.0F}
        ),
        cube,
        material
    );
    vshade::renderer::Renderer3D::endScene();

    // Render an overlay between 3D frames. Its temporary index buffer must not
    // replace the cube VAO's index buffer, and its disabled depth writes must
    // not prevent the next frame's depth clear.
    vshade::renderer::Camera overlayCamera;
    overlayCamera.setOrthographic(-1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F);
    vshade::renderer::Renderer2D::beginScene(overlayCamera);
    vshade::renderer::Renderer2D::drawQuad(
        vshade::math::Transform{},
        {1.0F, 1.0F, 1.0F, 1.0F}
    );
    vshade::renderer::Renderer2D::endScene();

    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.025F, 0.035F, 0.06F, 1.0F}
    );
    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawMesh(
        vshade::math::Transform(
            {0.0F, 0.0F, 0.0F},
            vshade::math::fromEuler({0.38F, 0.62F, 0.10F}),
            {1.0F, 1.0F, 1.0F}
        ),
        cube,
        material
    );
    vshade::renderer::Renderer3D::endScene();

    const vshade::tests::visual::Image actual =
        vshade::tests::visual::captureFramebuffer(framebuffer);
    CHECK(vshade::renderer::Renderer3D::stats().meshCount == 1);
    CHECK(vshade::renderer::Renderer3D::stats().drawCalls == 1);
    vshade::tests::visual::checkGoldenImage(
        actual,
        "renderer3d_lit_cube",
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "render3d",
        VSHADE_RENDER3D_OUTPUT_DIR
    );
}

TEST_CASE("Renderer3D scene environment and lights match their golden image", "[renderer3d][scene][lighting][visual][opengl]") {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.018F, 0.022F, 0.04F, 1.0F}
    );

    const vshade::renderer::Mesh cube = createCubeMesh();
    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 100.0F);
    camera.lookAt(
        {3.2F, 2.4F, 4.2F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );

    vshade::scene::Scene scene("Renderer3D environment visual");
    scene.setEnvironment({
        .ambientColor = {0.2F, 0.25F, 0.45F},
        .ambientIntensity = 0.18F,
    });
    const auto addDirectional = [&scene](
        const char* name,
        const vshade::renderer::DirectionalLight& light
    ) {
        vshade::scene::Entity entity = scene.createEntity(name);
        entity.addComponent<vshade::scene::LightComponent>(light, true);
    };
    addDirectional("Warm sun", {
        .direction = {-0.7F, -1.0F, -0.3F},
        .color = {1.0F, 0.72F, 0.48F},
        .intensity = 0.55F,
    });
    addDirectional("Cool fill", {
        .direction = {0.8F, -0.35F, 0.2F},
        .color = {0.35F, 0.55F, 1.0F},
        .intensity = 0.35F,
    });
    const auto addPoint = [&scene](
        const char* name,
        const vshade::math::Vec3& position,
        const vshade::renderer::PointLight& light
    ) {
        vshade::scene::Entity entity = scene.createEntity(name);
        entity.component<vshade::scene::TransformComponent>()
            .transform.setPosition(position);
        entity.addComponent<vshade::scene::LightComponent>(light, true);
    };
    addPoint("Warm point", {-1.5F, 0.8F, 2.0F}, {
        .color = {1.0F, 0.2F, 0.12F},
        .intensity = 2.2F,
        .range = 5.0F,
    });
    addPoint("Cool point", {1.8F, -0.4F, 1.4F}, {
        .color = {0.12F, 0.45F, 1.0F},
        .intensity = 2.0F,
        .range = 5.0F,
    });
    vshade::renderer::Renderer3D::setLighting(
        vshade::scene::SceneLightingSystem::collect(scene)
    );

    vshade::renderer::Material material(vshade::renderer::MaterialShading::Lit);
    material.setAlbedoColor({0.72F, 0.76F, 0.82F, 1.0F});
    material.setRoughness(0.45F);

    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawMesh(
        vshade::math::Transform(
            {0.0F, 0.0F, 0.0F},
            vshade::math::fromEuler({0.38F, 0.62F, 0.10F}),
            {1.0F, 1.0F, 1.0F}
        ),
        cube,
        material
    );
    vshade::renderer::Renderer3D::endScene();

    const vshade::tests::visual::Image actual =
        vshade::tests::visual::captureFramebuffer(framebuffer);
    vshade::tests::visual::checkGoldenImage(
        actual,
        "renderer3d_multiple_lights",
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "render3d",
        VSHADE_RENDER3D_OUTPUT_DIR
    );
}

TEST_CASE("Renderer3D loaded model matches its golden image", "[renderer3d][visual][opengl][asset]") {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.025F, 0.035F, 0.06F, 1.0F}
    );

    vshade::asset::AssetManager assets;
    const auto handle = assets.load<vshade::renderer::Model>(
        std::filesystem::path(VSHADE_TEST_ASSET_DIR) / "visual_model.gltf"
    );
    const std::shared_ptr<vshade::renderer::Model> model = assets.get(handle);
    REQUIRE(model);

    vshade::renderer::Camera camera;
    camera.setOrthographic(-1.5F, 1.5F, -1.5F, 1.5F, -1.0F, 1.0F);

    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawModel(vshade::math::Transform{}, *model);
    vshade::renderer::Renderer3D::endScene();

    CHECK(vshade::renderer::Renderer3D::stats().meshCount == 2);
    CHECK(vshade::renderer::Renderer3D::stats().drawCalls == 2);
    const vshade::tests::visual::Image actual =
        vshade::tests::visual::captureFramebuffer(framebuffer);
    vshade::tests::visual::checkGoldenImage(
        actual,
        "renderer3d_gltf_model",
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "render3d",
        VSHADE_RENDER3D_OUTPUT_DIR
    );
}

TEST_CASE("Renderer3D eye materials match their golden image", "[renderer3d][visual][opengl][asset]") {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.025F, 0.035F, 0.06F, 1.0F}
    );

    vshade::asset::AssetManager assets;
    const auto handle = assets.load<vshade::renderer::Model>(
        std::filesystem::path(VSHADE_TEST_ASSET_DIR) / "eye.glb"
    );
    const std::shared_ptr<vshade::renderer::Model> model = assets.get(handle);
    REQUIRE(model);

    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 100.0F);
    camera.lookAt(
        {2.8F, 1.8F, 3.8F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );
    vshade::renderer::Renderer3D::setDirectionalLight({
        .direction = {-0.55F, -1.0F, -0.35F},
        .color = {1.0F, 0.94F, 0.82F},
        .intensity = 1.0F,
    });

    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawModel(
        vshade::math::Transform(
            {0.0F, 0.0F, 0.0F},
            vshade::math::Quat{1.0F, 0.0F, 0.0F, 0.0F},
            {0.01F, 0.01F, 0.01F}
        ),
        *model
    );
    vshade::renderer::Renderer3D::endScene();

    CHECK(vshade::renderer::Renderer3D::stats().meshCount == 2);
    CHECK(vshade::renderer::Renderer3D::stats().drawCalls == 2);
    const vshade::tests::visual::Image actual =
        vshade::tests::visual::captureFramebuffer(framebuffer);
    vshade::tests::visual::checkGoldenImage(
        actual,
        "renderer3d_eye_materials",
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "render3d",
        VSHADE_RENDER3D_OUTPUT_DIR
    );
}

TEST_CASE("SceneRenderer consumes camera model and light components", "[renderer3d][visual][opengl][scene]") {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    framebuffer.bind();
    vshade::renderer::Renderer::beginFrame();
    vshade::renderer::Renderer::setViewport(0, 0, framebuffer.width(), framebuffer.height());
    vshade::renderer::Renderer::setDithering(false);

    vshade::asset::AssetManager assets;
    vshade::scene::Scene scene("Rendered scene");
    auto camera = scene.create("Camera");
    const vshade::math::Vec3 cameraPosition{2.8F, 1.8F, 3.8F};
    camera.transform().setPosition(cameraPosition);
    camera.transform().setRotation(vshade::math::lookRotation(-cameraPosition));
    auto& cameraSettings = camera.add<vshade::scene::CameraComponent>();
    cameraSettings.verticalFieldOfViewRadians = 0.785398163F;

    auto eye = scene.create("Eye");
    eye.transform().setScale({0.01F, 0.01F, 0.01F});
    eye.add<vshade::scene::ModelRendererComponent>(
        assets.reference<vshade::renderer::Model>(
            std::filesystem::path(VSHADE_TEST_ASSET_DIR) / "eye.glb"
        ),
        true
    );
    scene.create("Sun").add<vshade::scene::LightComponent>(
        vshade::renderer::DirectionalLight{
            .direction = {-0.55F, -1.0F, -0.35F},
            .color = {1.0F, 0.94F, 0.82F},
            .intensity = 1.0F,
        },
        true
    );

    vshade::scene::SceneRenderer renderer(assets);
    REQUIRE(renderer.render(scene, framebuffer.width(), framebuffer.height()));
    CHECK(vshade::renderer::Renderer3D::stats().meshCount == 2);
    CHECK(renderer.frameStats().entityCount == 3);
    CHECK(renderer.frameStats().meshCount == 2);
    const auto actual = vshade::tests::visual::captureFramebuffer(framebuffer);
    vshade::tests::visual::checkGoldenImage(
        actual,
        "renderer3d_eye_materials",
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "render3d",
        VSHADE_RENDER3D_OUTPUT_DIR
    );
}
