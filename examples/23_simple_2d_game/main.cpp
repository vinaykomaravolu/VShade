#include <common/ExampleSupport.hpp>
#include <core/Log.hpp>

class Simple2DGame final : public vshade::Application {
public:
    Simple2DGame() : Application(vshade::examples::config("23 - Block Runner")) {}

protected:
    void onStart() override {
        createCamera();
        createWorld();
        createPlayer();
        createObstacles();

        playScene(m_scene);
        runtime().physics2D().setContactListener(
            [this](const vshade::physics::SceneContactEvent2D& event) {
                handleContact(event);
            }
        );
        GAME_INFO("Block Runner: Space/Up jumps, R restarts, Escape exits");
    }

    void onUpdate(float) override {
        using vshade::input::Input;
        using vshade::input::KeyCode;

        if (Input::isKeyPressed(KeyCode::R)) resetRun();

        const bool jumpPressed = Input::isKeyPressed(KeyCode::Space) ||
            Input::isKeyPressed(KeyCode::Up);
        const auto body = runtime().physics2D().body(m_player);
        if (body) {
            auto& world = runtime().physics2D().world();
            vshade::math::Vec2 velocity = world.linearVelocity(*body);
            velocity.x = m_gameOver ? 0.0F : runSpeed;
            if (!m_gameOver && m_grounded && jumpPressed) {
                velocity.y = jumpSpeed;
                m_grounded = false;
                audio().playOneShot(vshade::examples::asset("sample-3.wav"));
            }
            world.setLinearVelocity(*body, velocity);
        }

        followPlayer();
        recycleObstacles();
        if (m_player.transform().position().y < -5.0F) resetRun();
        vshade::examples::closeOnEscape(*this);
    }

private:
    static constexpr float runSpeed = 5.5F;
    static constexpr float jumpSpeed = 7.5F;
    static constexpr vshade::math::Vec4 normalBackground{
        0.42F, 0.75F, 0.96F, 1.0F
    };
    static constexpr vshade::math::Vec4 crashedBackground{
        0.45F, 0.08F, 0.07F, 1.0F
    };

    void createCamera() {
        m_camera = m_scene.create("Following camera");
        m_camera.transform().setPosition({3.0F, 3.0F, 5.0F});
        auto& settings = m_camera.add<vshade::scene::CameraComponent>();
        settings.projection = vshade::scene::CameraProjection::Orthographic;
        settings.orthographicHeight = 7.0F;
        settings.clearColor = normalBackground;
    }

    void createWorld() {
        m_floor = m_scene.create("Endless floor");
        m_floor.transform().setPosition({0.0F, -0.5F, 0.0F});
        m_floor.transform().setScale({20'000.0F, 1.0F, 1.0F});
        auto& sprite = m_floor.add<vshade::scene::SpriteRendererComponent>();
        sprite.color = {0.2F, 0.58F, 0.22F, 1.0F};
        sprite.sortingLayer = 0;
        m_floor.add<vshade::scene::RigidBody2DComponent>();
        m_floor.add<vshade::scene::Collider2DComponent>().shape =
            vshade::physics::BoxShape2D{{10'000.0F, 0.5F}};
    }

    void createPlayer() {
        m_player = m_scene.create("Running block");
        m_player.transform().setPosition({0.0F, 1.0F, 0.0F});
        m_player.transform().setScale({0.9F, 0.9F, 1.0F});
        auto& sprite = m_player.add<vshade::scene::SpriteRendererComponent>();
        sprite.color = {0.12F, 0.32F, 0.95F, 1.0F};
        sprite.sortingLayer = 2;

        auto& body = m_player.add<vshade::scene::RigidBody2DComponent>().settings;
        body.type = vshade::physics::BodyType::Dynamic;
        body.fixedRotation = true;
        body.continuousCollision = true;
        body.linearVelocity = {runSpeed, 0.0F};

        auto& collider = m_player.add<vshade::scene::Collider2DComponent>();
        collider.shape = vshade::physics::BoxShape2D{{0.45F, 0.45F}};
        collider.material.friction = 0.0F;
    }

    void createObstacles() {
        constexpr std::array<float, 7> positions{
            8.0F, 14.0F, 20.5F, 27.0F, 35.0F, 41.0F, 48.5F
        };
        constexpr std::array<vshade::math::Vec2, 7> halfExtents{{
            {0.4F, 0.8F},
            {0.55F, 1.15F},
            {0.35F, 0.65F},
            {0.7F, 0.9F},
            {0.4F, 1.25F},
            {0.65F, 0.7F},
            {0.45F, 1.0F},
        }};

        for (std::size_t index = 0; index < positions.size(); ++index) {
            const auto size = halfExtents[index];
            auto obstacle = m_scene.create("Obstacle");
            obstacle.transform().setPosition({positions[index], size.y, 0.0F});
            obstacle.transform().setScale({size.x * 2.0F, size.y * 2.0F, 1.0F});
            auto& sprite = obstacle.add<vshade::scene::SpriteRendererComponent>();
            sprite.color = {0.92F, 0.24F + 0.04F * index, 0.08F, 1.0F};
            sprite.sortingLayer = 1;
            obstacle.add<vshade::scene::RigidBody2DComponent>();
            obstacle.add<vshade::scene::Collider2DComponent>().shape =
                vshade::physics::BoxShape2D{size};
            m_obstacles.push_back(obstacle);
            m_obstacleStartPositions.push_back(positions[index]);
        }
        m_nextObstacleX = positions.back();
    }

    void handleContact(const vshade::physics::SceneContactEvent2D& event) {
        const bool playerFirst = event.first == m_player;
        const bool playerSecond = event.second == m_player;
        if (!playerFirst && !playerSecond) return;

        const vshade::Entity other = playerFirst ? event.second : event.first;
        if (other == m_floor) {
            if (event.phase == vshade::physics::ContactPhase::Ended) {
                m_grounded = false;
            } else {
                m_grounded = true;
            }
            return;
        }

        if (event.phase == vshade::physics::ContactPhase::Began &&
            std::find(m_obstacles.begin(), m_obstacles.end(), other) != m_obstacles.end()) {
            crash();
        }
    }

    void crash() {
        if (m_gameOver) return;
        m_gameOver = true;
        m_camera.get<vshade::scene::CameraComponent>().clearColor = crashedBackground;
        GAME_INFO(
            "Crashed after {:.1f} units. Press R to run again.",
            m_player.transform().position().x
        );
    }

    void followPlayer() {
        const float playerX = m_player.transform().position().x;
        m_camera.transform().setPosition({playerX + 3.0F, 3.0F, 5.0F});
    }

    void recycleObstacles() {
        const float playerX = m_player.transform().position().x;
        for (std::size_t index = 0; index < m_obstacles.size(); ++index) {
            auto& obstacle = m_obstacles[index];
            if (obstacle.transform().position().x >= playerX - 8.0F) continue;

            m_nextObstacleX += 5.5F + static_cast<float>(index % 3U);
            auto position = obstacle.transform().position();
            position.x = m_nextObstacleX;
            obstacle.transform().setPosition(position);
            if (const auto body = runtime().physics2D().body(obstacle)) {
                runtime().physics2D().world().setTransform(
                    *body,
                    {position.x, position.y},
                    0.0F
                );
            }
        }
    }

    void resetRun() {
        m_gameOver = false;
        m_grounded = false;
        m_camera.get<vshade::scene::CameraComponent>().clearColor = normalBackground;

        m_player.transform().setPosition({0.0F, 1.0F, 0.0F});
        if (const auto body = runtime().physics2D().body(m_player)) {
            auto& world = runtime().physics2D().world();
            world.setTransform(*body, {0.0F, 1.0F}, 0.0F);
            world.setLinearVelocity(*body, {runSpeed, 0.0F});
        }

        for (std::size_t index = 0; index < m_obstacles.size(); ++index) {
            auto position = m_obstacles[index].transform().position();
            position.x = m_obstacleStartPositions[index];
            m_obstacles[index].transform().setPosition(position);
            if (const auto body = runtime().physics2D().body(m_obstacles[index])) {
                runtime().physics2D().world().setTransform(
                    *body,
                    {position.x, position.y},
                    0.0F
                );
            }
        }
        m_nextObstacleX = m_obstacleStartPositions.back();
        followPlayer();
        GAME_INFO("Run restarted");
    }

    vshade::Scene m_scene{"Block Runner"};
    vshade::Entity m_camera;
    vshade::Entity m_floor;
    vshade::Entity m_player;
    std::vector<vshade::Entity> m_obstacles;
    std::vector<float> m_obstacleStartPositions;
    float m_nextObstacleX = 0.0F;
    bool m_grounded = false;
    bool m_gameOver = false;
};

VSHADE_GAME(Simple2DGame)
