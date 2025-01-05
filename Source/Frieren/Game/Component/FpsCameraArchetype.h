#pragma once
#include "Igniter/Component/NameComponent.h"
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Component/TransformComponent.h"
#include "Frieren/Game/Component/FpsCameraController.h"

namespace fe
{
    struct FpsCmaeraArchetype
    {
    public:
        static ig::Entity Create(ig::Registry& registry, const bool bIsMainCamera)
        {
            using namespace ig::literals;
            const ig::Entity newEntity = registry.create();
            auto& cameraComponent = registry.emplace<ig::CameraComponent>(newEntity);
            cameraComponent.bIsMainCamera = bIsMainCamera;
            registry.emplace<ig::TransformComponent>(newEntity);
            registry.emplace<FpsCameraController>(newEntity);
            registry.emplace<ig::NameComponent>(newEntity, "FpsCamera"_fs);
            return newEntity;
        }
    };
} // namespace fe
