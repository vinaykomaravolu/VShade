#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <math/Math.hpp>

#include <stdexcept>
#include <type_traits>

namespace {

void checkVec2(const vshade::math::Vec2& actual, const vshade::math::Vec2& expected) {
    CHECK(actual.x == Catch::Approx(expected.x).margin(0.0001F));
    CHECK(actual.y == Catch::Approx(expected.y).margin(0.0001F));
}

void checkVec3(const vshade::math::Vec3& actual, const vshade::math::Vec3& expected) {
    CHECK(actual.x == Catch::Approx(expected.x).margin(0.0001F));
    CHECK(actual.y == Catch::Approx(expected.y).margin(0.0001F));
    CHECK(actual.z == Catch::Approx(expected.z).margin(0.0001F));
}

void checkVec4(const vshade::math::Vec4& actual, const vshade::math::Vec4& expected) {
    CHECK(actual.x == Catch::Approx(expected.x).margin(0.0001F));
    CHECK(actual.y == Catch::Approx(expected.y).margin(0.0001F));
    CHECK(actual.z == Catch::Approx(expected.z).margin(0.0001F));
    CHECK(actual.w == Catch::Approx(expected.w).margin(0.0001F));
}

} // namespace

TEST_CASE("Math aliases expose GLM types", "[math]") {
    STATIC_REQUIRE((std::is_same_v<vshade::math::Vec3, glm::vec3>));
    STATIC_REQUIRE((std::is_same_v<vshade::math::Mat4, glm::mat4>));
    STATIC_REQUIRE((std::is_same_v<vshade::math::Quat, glm::quat>));
}

TEST_CASE("Vectors retain GLM arithmetic", "[math]") {
    const vshade::math::Vec3 left{1.0F, 2.0F, 3.0F};
    const vshade::math::Vec3 right{4.0F, 5.0F, 6.0F};

    checkVec3(left + right, {5.0F, 7.0F, 9.0F});
}

TEST_CASE("Vector length normalization and products use the engine API", "[math]") {
    CHECK(vshade::math::length(vshade::math::Vec2{3.0F, 4.0F}) == 5.0F);
    CHECK(vshade::math::length(vshade::math::Vec3{0.0F, 0.0F, 3.0F}) == 3.0F);
    CHECK(vshade::math::length(vshade::math::Vec4{0.0F, 0.0F, 0.0F, 2.0F}) == 2.0F);

    checkVec2(vshade::math::normalize(vshade::math::Vec2{3.0F, 4.0F}), {0.6F, 0.8F});
    checkVec3(
        vshade::math::normalize(vshade::math::Vec3{0.0F, 0.0F, -4.0F}),
        {0.0F, 0.0F, -1.0F}
    );
    checkVec4(
        vshade::math::normalize(vshade::math::Vec4{0.0F, 0.0F, 0.0F, 2.0F}),
        {0.0F, 0.0F, 0.0F, 1.0F}
    );

    CHECK(vshade::math::dot(
        vshade::math::Vec2{1.0F, 2.0F},
        vshade::math::Vec2{3.0F, 4.0F}
    ) == 11.0F);
    CHECK(vshade::math::dot(
        vshade::math::Vec3{1.0F, 2.0F, 3.0F},
        vshade::math::Vec3{4.0F, 5.0F, 6.0F}
    ) == 32.0F);
    CHECK(vshade::math::dot(
        vshade::math::Vec4{1.0F},
        vshade::math::Vec4{2.0F}
    ) == 8.0F);
    checkVec3(
        vshade::math::cross(
            vshade::math::Vec3{1.0F, 0.0F, 0.0F},
            vshade::math::Vec3{0.0F, 1.0F, 0.0F}
        ),
        {0.0F, 0.0F, 1.0F}
    );
}

TEST_CASE("Vector distance reflection and interpolation support gameplay math", "[math]") {
    CHECK(vshade::math::distance(
        vshade::math::Vec2{0.0F, 0.0F},
        vshade::math::Vec2{3.0F, 4.0F}
    ) == 5.0F);
    CHECK(vshade::math::distance(
        vshade::math::Vec3{1.0F, 2.0F, 3.0F},
        vshade::math::Vec3{1.0F, 2.0F, 8.0F}
    ) == 5.0F);

    checkVec3(
        vshade::math::reflect(
            vshade::math::Vec3{1.0F, -1.0F, 0.0F},
            vshade::math::Vec3{0.0F, 1.0F, 0.0F}
        ),
        {1.0F, 1.0F, 0.0F}
    );
    checkVec2(
        vshade::math::lerp(
            vshade::math::Vec2{0.0F, 2.0F},
            vshade::math::Vec2{10.0F, 6.0F},
            0.25F
        ),
        {2.5F, 3.0F}
    );
    checkVec3(
        vshade::math::lerp(
            vshade::math::Vec3{0.0F},
            vshade::math::Vec3{2.0F, 4.0F, 6.0F},
            1.5F
        ),
        {3.0F, 6.0F, 9.0F}
    );
}

TEST_CASE("Vector helpers safely normalize and measure vectors", "[math]") {
    CHECK(vshade::math::lengthSquared(vshade::math::Vec2{3.0F, 4.0F}) == 25.0F);
    CHECK(vshade::math::distanceSquared(
        vshade::math::Vec3{1.0F, 2.0F, 3.0F},
        vshade::math::Vec3{4.0F, 6.0F, 3.0F}
    ) == 25.0F);

    checkVec3(
        vshade::math::normalizedOrZero(vshade::math::Vec3{0.0F, 0.0F, 0.0F}),
        {0.0F, 0.0F, 0.0F}
    );
    checkVec3(
        vshade::math::normalizedOrZero(vshade::math::Vec3{0.0F, 0.0F, -5.0F}),
        {0.0F, 0.0F, -1.0F}
    );
    checkVec3(
        vshade::math::direction(
            vshade::math::Vec3{1.0F, 2.0F, 3.0F},
            vshade::math::Vec3{1.0F, 2.0F, -2.0F}
        ),
        {0.0F, 0.0F, -1.0F}
    );
}

TEST_CASE("Vector helpers move toward targets and measure angles", "[math]") {
    checkVec2(
        vshade::math::moveTowards(
            vshade::math::Vec2{0.0F, 0.0F},
            vshade::math::Vec2{3.0F, 4.0F},
            2.0F
        ),
        {1.2F, 1.6F}
    );
    checkVec2(
        vshade::math::moveTowards(
            vshade::math::Vec2{0.0F, 0.0F},
            vshade::math::Vec2{1.0F, 0.0F},
            10.0F
        ),
        {1.0F, 0.0F}
    );

    constexpr float quarter_turn = 1.57079632679F;
    CHECK(vshade::math::angleBetween(
        vshade::math::Vec3{1.0F, 0.0F, 0.0F},
        vshade::math::Vec3{0.0F, 1.0F, 0.0F}
    ) == Catch::Approx(quarter_turn).margin(0.0001F));
}

TEST_CASE("Quaternions rotate vectors", "[math]") {
    constexpr float quarter_turn = 1.57079632679F;
    const vshade::math::Quat rotation = vshade::math::fromAxisAngle(
        {0.0F, 0.0F, 5.0F},
        quarter_turn
    );

    checkVec3(
        vshade::math::rotate(rotation, {1.0F, 0.0F, 0.0F}),
        {0.0F, 1.0F, 0.0F}
    );
}

TEST_CASE("Quaternion identity exposes engine basis directions", "[math]") {
    const vshade::math::Quat identity = vshade::math::identity();

    checkVec3(vshade::math::forward(identity), {0.0F, 0.0F, -1.0F});
    checkVec3(vshade::math::right(identity), {1.0F, 0.0F, 0.0F});
    checkVec3(vshade::math::up(identity), {0.0F, 1.0F, 0.0F});
}

TEST_CASE("Quaternion Euler angles round trip", "[math]") {
    const vshade::math::Vec3 angles{0.2F, -0.4F, 0.6F};
    const vshade::math::Quat rotation = vshade::math::fromEuler(angles);

    checkVec3(vshade::math::toEuler(rotation), angles);
}

TEST_CASE("Quaternion interpolation blends and clamps rotations", "[math]") {
    constexpr float quarter_turn = 1.57079632679F;
    constexpr float diagonal = 0.70710678118F;
    const vshade::math::Quat start = vshade::math::identity();
    const vshade::math::Quat end = vshade::math::fromAxisAngle(
        {0.0F, 1.0F, 0.0F},
        quarter_turn
    );

    checkVec3(
        vshade::math::forward(vshade::math::slerp(start, end, 0.5F)),
        {-diagonal, 0.0F, -diagonal}
    );
    checkVec3(
        vshade::math::forward(vshade::math::slerp(start, end, 2.0F)),
        {-1.0F, 0.0F, 0.0F}
    );
}

TEST_CASE("Quaternion normalization and inverse produce valid rotations", "[math]") {
    const vshade::math::Quat normalized = vshade::math::normalize(
        vshade::math::Quat{2.0F, 0.0F, 0.0F, 0.0F}
    );
    CHECK(normalized.w == Catch::Approx(1.0F).margin(0.0001F));
    CHECK(normalized.x == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(normalized.y == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(normalized.z == Catch::Approx(0.0F).margin(0.0001F));

    constexpr float quarter_turn = 1.57079632679F;
    const vshade::math::Quat rotation = vshade::math::fromAxisAngle(
        {0.0F, 1.0F, 0.0F},
        quarter_turn
    );
    const vshade::math::Vec3 original{0.0F, 0.0F, -1.0F};
    const vshade::math::Vec3 rotated = vshade::math::rotate(rotation, original);

    checkVec3(vshade::math::rotate(vshade::math::inverse(rotation), rotated), original);
}

TEST_CASE("Quaternion normalization rejects zero length", "[math]") {
    CHECK_THROWS_AS(
        vshade::math::normalize(vshade::math::Quat{0.0F, 0.0F, 0.0F, 0.0F}),
        std::invalid_argument
    );
}

TEST_CASE("Quaternion axis angle rejects a zero axis", "[math]") {
    CHECK_THROWS_AS(
        vshade::math::fromAxisAngle({0.0F, 0.0F, 0.0F}, 1.0F),
        std::invalid_argument
    );
}

TEST_CASE("Matrix helpers create translation rotation and scale", "[math]") {
    constexpr float quarter_turn = 1.57079632679F;
    const vshade::math::Vec4 point{1.0F, 0.0F, 0.0F, 1.0F};

    checkVec4(
        vshade::math::translationMatrix({2.0F, 3.0F, 4.0F}) * point,
        {3.0F, 3.0F, 4.0F, 1.0F}
    );
    checkVec4(
        vshade::math::rotationMatrix(glm::angleAxis(
            quarter_turn,
            vshade::math::Vec3{0.0F, 0.0F, 1.0F}
        )) * point,
        {0.0F, 1.0F, 0.0F, 1.0F}
    );
    checkVec4(
        vshade::math::scaleMatrix({2.0F, 3.0F, 4.0F}) * point,
        {2.0F, 0.0F, 0.0F, 1.0F}
    );
}

TEST_CASE("Matrix identity leaves vectors unchanged", "[math]") {
    const vshade::math::Vec4 vector{1.0F, 2.0F, 3.0F, 1.0F};

    checkVec4(vshade::math::identityMatrix() * vector, vector);
}

TEST_CASE("Transforms compose translation rotation and scale", "[math]") {
    constexpr float quarter_turn = 1.57079632679F;
    const vshade::math::Transform transform(
        {2.0F, 3.0F, 4.0F},
        glm::angleAxis(
            quarter_turn,
            vshade::math::Vec3{0.0F, 0.0F, 1.0F}
        ),
        {2.0F, 2.0F, 2.0F}
    );

    const vshade::math::Vec4 transformed =
        transform.matrix() * vshade::math::Vec4{1.0F, 0.0F, 0.0F, 1.0F};

    CHECK(transformed.x == Catch::Approx(2.0F).margin(0.0001F));
    CHECK(transformed.y == Catch::Approx(5.0F).margin(0.0001F));
    CHECK(transformed.z == Catch::Approx(4.0F).margin(0.0001F));
    CHECK(transformed.w == Catch::Approx(1.0F).margin(0.0001F));

    checkVec3(transform.transformPoint({1.0F, 0.0F, 0.0F}), {2.0F, 5.0F, 4.0F});
    checkVec3(transform.transformVector({1.0F, 0.0F, 0.0F}), {0.0F, 2.0F, 0.0F});
    checkVec3(transform.transformDirection({1.0F, 0.0F, 0.0F}), {0.0F, 1.0F, 0.0F});
    checkVec3(transform.forward(), {0.0F, 0.0F, -1.0F});
    checkVec3(transform.right(), {0.0F, 1.0F, 0.0F});
    checkVec3(transform.up(), {-1.0F, 0.0F, 0.0F});
}

TEST_CASE("Transform accessors and mutators maintain transform state", "[math]") {
    constexpr float quarter_turn = 1.57079632679F;
    vshade::math::Transform transform;

    checkVec3(transform.position(), {0.0F, 0.0F, 0.0F});
    checkVec3(transform.scale(), {1.0F, 1.0F, 1.0F});
    checkVec3(transform.forward(), {0.0F, 0.0F, -1.0F});

    transform.setPosition({1.0F, 2.0F, 3.0F});
    transform.translate({4.0F, -2.0F, 1.0F});
    transform.setScale({2.0F, 3.0F, 4.0F});
    transform.setRotation(glm::angleAxis(
        quarter_turn,
        vshade::math::Vec3{0.0F, 1.0F, 0.0F}
    ));

    checkVec3(transform.position(), {5.0F, 0.0F, 4.0F});
    checkVec3(transform.scale(), {2.0F, 3.0F, 4.0F});
    CHECK(glm::length(transform.rotation()) == Catch::Approx(1.0F).margin(0.0001F));
    checkVec3(transform.forward(), {-1.0F, 0.0F, 0.0F});

    transform.rotate(glm::angleAxis(
        quarter_turn,
        vshade::math::Vec3{0.0F, 1.0F, 0.0F}
    ));
    checkVec3(transform.forward(), {0.0F, 0.0F, 1.0F});
}

TEST_CASE("Transform rejects zero quaternions", "[math]") {
    vshade::math::Transform transform;

    CHECK_THROWS_AS(
        transform.setRotation(vshade::math::Quat{0.0F, 0.0F, 0.0F, 0.0F}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        vshade::math::Transform(
            vshade::math::Vec3{0.0F},
            vshade::math::Quat{0.0F, 0.0F, 0.0F, 0.0F}
        ),
        std::invalid_argument
    );
}

TEST_CASE("Perspective maps near and far planes to OpenGL depth", "[math]") {
    constexpr float quarter_turn = 1.57079632679F;
    const vshade::math::Mat4 projection =
        vshade::math::perspective(quarter_turn, 1.0F, 1.0F, 10.0F);

    const vshade::math::Vec4 near_clip =
        projection * vshade::math::Vec4{0.0F, 0.0F, -1.0F, 1.0F};
    const vshade::math::Vec4 far_clip =
        projection * vshade::math::Vec4{0.0F, 0.0F, -10.0F, 1.0F};

    CHECK(near_clip.z / near_clip.w == Catch::Approx(-1.0F).margin(0.0001F));
    CHECK(far_clip.z / far_clip.w == Catch::Approx(1.0F).margin(0.0001F));
}

TEST_CASE("Orthographic maps view bounds to clip bounds", "[math]") {
    const vshade::math::Mat4 projection =
        vshade::math::orthographic(-2.0F, 2.0F, -1.0F, 1.0F, 0.1F, 10.0F);

    checkVec4(
        projection * vshade::math::Vec4{2.0F, 1.0F, -0.1F, 1.0F},
        {1.0F, 1.0F, -1.0F, 1.0F}
    );
}

TEST_CASE("Look at moves the camera eye to view-space origin", "[math]") {
    const vshade::math::Mat4 view = vshade::math::lookAt(
        {0.0F, 0.0F, 5.0F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );

    checkVec4(
        view * vshade::math::Vec4{0.0F, 0.0F, 5.0F, 1.0F},
        {0.0F, 0.0F, 0.0F, 1.0F}
    );
    checkVec4(
        view * vshade::math::Vec4{0.0F, 0.0F, 0.0F, 1.0F},
        {0.0F, 0.0F, -5.0F, 1.0F}
    );
}

TEST_CASE("Rays intersect planes in front of their origin", "[math][ray]") {
    const vshade::math::Ray ray{
        .origin = {0.0F, 2.0F, 0.0F},
        .direction = {0.0F, -1.0F, 0.0F},
    };
    const auto hit = vshade::math::intersectPlane(
        ray,
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );
    REQUIRE(hit.has_value());
    checkVec3(*hit, {0.0F, 0.0F, 0.0F});

    CHECK_FALSE(vshade::math::intersectPlane(
        ray,
        {0.0F, 0.0F, 0.0F},
        {1.0F, 0.0F, 0.0F}
    ));
    CHECK_FALSE(vshade::math::intersectPlane(
        {.origin = {0.0F, 2.0F, 0.0F}, .direction = {0.0F, 1.0F, 0.0F}},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    ));
}
