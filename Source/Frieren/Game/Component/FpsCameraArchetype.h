#pragma once
#include "Igniter/Component/NameComponent.h"
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Component/TransformComponent.h"
#include "Frieren/Game/Component/FpsCameraController.h"

namespace fe
{
    struct FpsCameraArchetype
    {
      public:
        static ig::Entity Create(ig::Registry* registry)
        {
            const ig::Entity newEntity = registry->create();
            auto& cameraComponent = registry->emplace<ig::CameraComponent>(newEntity);
            cameraComponent.bIsMainCamera = false;
            registry->emplace<ig::TransformComponent>(newEntity);
            registry->emplace<FpsCameraController>(newEntity);
            registry->emplace<ig::NameComponent>(newEntity, "FpsCamera");
            return newEntity;
        }
    };

    IG_DECLARE_META(FpsCameraArchetype);
} // namespace fe
