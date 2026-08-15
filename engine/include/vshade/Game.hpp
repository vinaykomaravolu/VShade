#pragma once

// Curated game-author surface. Include subsystem headers directly for advanced use.
#include "asset/AssetRef.hpp"
#include "asset/AssetManager.hpp"
#include "asset/AssetReference.hpp"
#include "audio/AudioService.hpp"
#include "core/Application.hpp"
#include "core/EntryPoint.hpp"
#include "core/TypeRegistry.hpp"
#include "math/Transform.hpp"
#include "math/Vector.hpp"
#include "physics/physics2d/PhysicsSystem2D.hpp"
#include "physics/physics3d/PhysicsSystem3D.hpp"
#include "renderer/Model.hpp"
#include "scene/Components.hpp"
#include "scene/Entity.hpp"
#include "scene/Prefab.hpp"
#include "scene/Scene.hpp"
#include "scene/SceneRuntime.hpp"
#include "script/NativeScript.hpp"
#include "script/NativeScriptRegistry.hpp"

namespace vshade {

using Application = core::Application;
using ApplicationConfig = core::ApplicationConfig;
using Entity = scene::Entity;
using Scene = scene::Scene;
using SceneRuntime = scene::SceneRuntime;
using Prefab = scene::Prefab;
using NativeScript = script::NativeScript;

} // namespace vshade

/** @brief Defines the entry point for a default-constructible VShade game. */
#define VSHADE_GAME(application_type) SHADE_ENGINE_MAIN(application_type)
