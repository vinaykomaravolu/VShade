#include "visual/RenderFixture.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <physics/PhysicsTypes.hpp>
#include <physics/physics3d/PhysicsShape3D.hpp>
#include <physics/physics3d/PhysicsSystem3D.hpp>
#include <physics/physics3d/PhysicsWorld3D.hpp>
#include <renderer/Buffer.hpp>
#include <renderer/Camera.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Material.hpp>
#include <renderer/Mesh.hpp>
#include <renderer/Renderer3D.hpp>
#include <renderer/VertexArray.hpp>
#include <scene/Components.hpp>
#include <scene/Entity.hpp>
#include <scene/Scene.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

namespace {

constexpr float fixedTimeStep = 1.0F / 60.0F;
constexpr float pi = 3.14159265358979323846F;

[[nodiscard]] vshade::renderer::Mesh createMesh(
    const std::vector<vshade::renderer::MeshVertex>& vertices,
    const std::vector<std::uint32_t>& indices
) {
    auto vertexBuffer = std::make_shared<vshade::renderer::VertexBuffer>(
        vertices.data(),
        vertices.size() * sizeof(vshade::renderer::MeshVertex)
    );
    vertexBuffer->setLayout({
        {"position", vshade::renderer::ShaderDataType::Float3},
        {"normal", vshade::renderer::ShaderDataType::Float3},
        {"textureCoordinate", vshade::renderer::ShaderDataType::Float2},
        {"tangent", vshade::renderer::ShaderDataType::Float4},
    });
    auto indexBuffer = std::make_shared<vshade::renderer::IndexBuffer>(
        indices.data(),
        static_cast<std::uint32_t>(indices.size())
    );
    auto vertexArray = std::make_shared<vshade::renderer::VertexArray>();
    vertexArray->addVertexBuffer(std::move(vertexBuffer));
    vertexArray->setIndexBuffer(std::move(indexBuffer));
    return vshade::renderer::Mesh(std::move(vertexArray));
}

[[nodiscard]] vshade::renderer::Mesh createBoxMesh() {
    using vshade::renderer::MeshVertex;
    const std::vector<MeshVertex> vertices{
        {{-0.5F, -0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
        {{0.5F, -0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
        {{0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
        {{-0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
        {{0.5F, -0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}, {0.0F, 0.0F}},
        {{-0.5F, -0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}, {1.0F, 0.0F}},
        {{-0.5F, 0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}, {1.0F, 1.0F}},
        {{0.5F, 0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F}},
        {{-0.5F, -0.5F, -0.5F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
        {{-0.5F, -0.5F, 0.5F}, {-1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
        {{-0.5F, 0.5F, 0.5F}, {-1.0F, 0.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.5F, 0.5F, -0.5F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
        {{0.5F, -0.5F, 0.5F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.5F, -0.5F, -0.5F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.5F, 0.5F, -0.5F}, {1.0F, 0.0F, 0.0F}, {1.0F, 1.0F}},
        {{0.5F, 0.5F, 0.5F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
        {{-0.5F, 0.5F, 0.5F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.5F, 0.5F, 0.5F}, {0.0F, 1.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.5F, 0.5F, -0.5F}, {0.0F, 1.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.5F, 0.5F, -0.5F}, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F}},
        {{-0.5F, -0.5F, -0.5F}, {0.0F, -1.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.5F, -0.5F, -0.5F}, {0.0F, -1.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.5F, -0.5F, 0.5F}, {0.0F, -1.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.5F, -0.5F, 0.5F}, {0.0F, -1.0F, 0.0F}, {0.0F, 1.0F}},
    };
    const std::vector<std::uint32_t> indices{
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20,
    };
    return createMesh(vertices, indices);
}

[[nodiscard]] vshade::renderer::Mesh createSphereMesh() {
    constexpr std::uint32_t latitudeSegments = 16;
    constexpr std::uint32_t longitudeSegments = 24;
    constexpr float radius = 0.5F;
    std::vector<vshade::renderer::MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
    vertices.reserve((latitudeSegments + 1) * (longitudeSegments + 1));
    indices.reserve(latitudeSegments * longitudeSegments * 6);

    for (std::uint32_t latitude = 0; latitude <= latitudeSegments; ++latitude) {
        const float v = static_cast<float>(latitude) / latitudeSegments;
        const float theta = v * pi;
        const float sinTheta = std::sin(theta);
        const float cosTheta = std::cos(theta);
        for (std::uint32_t longitude = 0; longitude <= longitudeSegments; ++longitude) {
            const float u = static_cast<float>(longitude) / longitudeSegments;
            const float phi = u * 2.0F * pi;
            const vshade::math::Vec3 normal{
                sinTheta * std::cos(phi),
                cosTheta,
                sinTheta * std::sin(phi)
            };
            vertices.push_back({normal * radius, normal, {u, v}});
        }
    }

    constexpr std::uint32_t rowSize = longitudeSegments + 1;
    for (std::uint32_t latitude = 0; latitude < latitudeSegments; ++latitude) {
        for (std::uint32_t longitude = 0; longitude < longitudeSegments; ++longitude) {
            const std::uint32_t current = latitude * rowSize + longitude;
            const std::uint32_t next = current + rowSize;
            indices.insert(indices.end(), {
                current, current + 1, next + 1,
                current, next + 1, next,
            });
        }
    }
    return createMesh(vertices, indices);
}

void checkBallAndFloorGolden(
    const vshade::math::Vec3& ballPosition,
    const vshade::math::Vec3& floorPosition,
    const std::string_view imageName
) {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.025F, 0.035F, 0.06F, 1.0F}
    );

    vshade::renderer::Camera camera;
    camera.setPerspective(0.72F, 1.0F, 0.1F, 100.0F);
    camera.lookAt({4.2F, 2.5F, 6.0F}, {0.0F, -0.7F, 0.0F}, {0.0F, 1.0F, 0.0F});

    const vshade::renderer::Mesh sphere = createSphereMesh();
    const vshade::renderer::Mesh floor = createBoxMesh();
    vshade::renderer::Material ballMaterial(vshade::renderer::MaterialShading::Lit);
    ballMaterial.setAlbedoColor({0.20F, 0.58F, 0.95F, 1.0F});
    ballMaterial.setRoughness(0.38F);
    vshade::renderer::Material floorMaterial(vshade::renderer::MaterialShading::Lit);
    floorMaterial.setAlbedoColor({0.38F, 0.43F, 0.52F, 1.0F});
    floorMaterial.setRoughness(0.82F);

    vshade::renderer::Renderer3D::setDirectionalLight({
        .direction = {-0.55F, -1.0F, -0.35F},
        .color = {1.0F, 0.95F, 0.84F},
        .intensity = 1.1F,
    });
    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawMesh(
        vshade::math::Transform(
            floorPosition,
            vshade::math::Quat{1.0F, 0.0F, 0.0F, 0.0F},
            {6.0F, 0.5F, 6.0F}
        ),
        floor,
        floorMaterial
    );
    vshade::renderer::Renderer3D::drawMesh(
        vshade::math::Transform(ballPosition),
        sphere,
        ballMaterial
    );
    vshade::renderer::Renderer3D::endScene();

    const vshade::tests::visual::Image actual =
        vshade::tests::visual::captureFramebuffer(framebuffer);
    CHECK(vshade::renderer::Renderer3D::stats().meshCount == 2);
    vshade::tests::visual::checkGoldenImage(
        actual,
        imageName,
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "physics3d",
        VSHADE_PHYSICS3D_OUTPUT_DIR
    );
}

} // namespace

TEST_CASE(
    "PhysicsWorld3D ball and floor match their golden image",
    "[physics][physics3d][world][visual][opengl]"
) {
    vshade::physics::PhysicsWorld3D world({.maxBodies = 1'024});
    const auto floor = world.createBody(
        {
            .type = vshade::physics::BodyType::Static,
            .position = {0.0F, -1.3F, 0.0F},
        },
        vshade::physics::BoxShape3D{{3.0F, 0.25F, 3.0F}}
    );
    const auto ball = world.createBody(
        {
            .type = vshade::physics::BodyType::Dynamic,
            .position = {0.0F, 2.5F, 0.0F},
        },
        vshade::physics::SphereShape3D{0.5F}
    );
    for (int step = 0; step < 180; ++step) {
        world.step(fixedTimeStep);
    }

    const vshade::math::Vec3 ballPosition = world.position(ball);
    CHECK(ballPosition.y == Catch::Approx(-0.55F).margin(0.04F));
    checkBallAndFloorGolden(
        ballPosition,
        world.position(floor),
        "physics_world3d_ball_and_floor"
    );
}

TEST_CASE(
    "PhysicsSystem3D ball and floor match their golden image",
    "[physics][physics3d][system][scene][visual][opengl]"
) {
    vshade::scene::Scene scene("PhysicsSystem3D visual scene");

    vshade::scene::Entity floor = scene.createEntity("Floor");
    floor.component<vshade::scene::TransformComponent>().transform.setPosition({
        0.0F,
        -1.3F,
        0.0F
    });
    floor.addComponent<vshade::scene::RigidBody3DComponent>().settings.type =
        vshade::physics::BodyType::Static;
    floor.addComponent<vshade::scene::Collider3DComponent>(
        vshade::scene::Collider3DComponent{
            .shape = vshade::physics::BoxShape3D{{3.0F, 0.25F, 3.0F}},
        }
    );

    vshade::scene::Entity ball = scene.createEntity("Ball");
    ball.component<vshade::scene::TransformComponent>().transform.setPosition({
        0.0F,
        2.5F,
        0.0F
    });
    ball.addComponent<vshade::scene::RigidBody3DComponent>().settings.type =
        vshade::physics::BodyType::Dynamic;
    ball.addComponent<vshade::scene::Collider3DComponent>(
        vshade::scene::Collider3DComponent{
            .shape = vshade::physics::SphereShape3D{0.5F},
        }
    );

    vshade::physics::PhysicsSystem3D physics({.maxBodies = 1'024});
    physics.rebuild(scene);
    for (int step = 0; step < 180; ++step) {
        physics.update(scene, fixedTimeStep);
    }

    const vshade::math::Vec3 ballPosition =
        ball.component<vshade::scene::TransformComponent>().transform.position();
    CHECK(ballPosition.y == Catch::Approx(-0.55F).margin(0.04F));
    checkBallAndFloorGolden(
        ballPosition,
        floor.component<vshade::scene::TransformComponent>().transform.position(),
        "physics_system3d_ball_and_floor"
    );
}
