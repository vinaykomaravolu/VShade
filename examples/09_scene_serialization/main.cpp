#include <common/ExampleSupport.hpp>
#include <core/Log.hpp>
#include <scene/SceneSerializer.hpp>

class SceneSerialization final : public vshade::Application {
public:
    SceneSerialization()
        : Application(vshade::examples::config("09 - Scene Serialization")) {}
protected:
    void onStart() override {
        std::filesystem::create_directories(VSHADE_EXAMPLE_OUTPUT_DIR);
        const auto path = std::filesystem::path(VSHADE_EXAMPLE_OUTPUT_DIR) / "scene.json";
        auto entity = m_source.create("Persistent entity");
        const auto uuid = entity.uuid();
        entity.transform().setPosition({2.0F, 1.0F, 0.0F});

        vshade::scene::SceneSerializer writer(m_source);
        const auto saved = writer.serializeResult(path);
        if (!saved) throw std::runtime_error(saved.error().message);

        vshade::scene::SceneSerializer reader(m_loaded);
        const auto loaded = reader.deserializeResult(path);
        if (!loaded) throw std::runtime_error(loaded.error().message);
        GAME_INFO("Saved and restored entity UUID {}", uuid);
        if (!m_loaded.findEntity(uuid)) throw std::runtime_error("UUID was not restored");
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override { vshade::examples::clear(); }
private:
    vshade::Scene m_source{"Serializable scene"};
    vshade::Scene m_loaded{"Loaded scene"};
};
VSHADE_GAME(SceneSerialization)
