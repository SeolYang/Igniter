#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Serialization.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    struct TransformComponent
    {
    public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);
        static void OnInspector(Registry* registry, const Entity entity);

        Vector3 Position{};
        Vector3 Scale{1.f, 1.f, 1.f};
        Quaternion Rotation;
    };

    class TransformUtility
    {
    public:
        [[nodiscard]] static Matrix CreateTransformation(const TransformComponent& transform)
        {
            // TRS -> Column Vector ; SRT -> Row Vector
            return Matrix::CreateScale(transform.Scale) * Matrix::CreateFromQuaternion(transform.Rotation) * Matrix::CreateTranslation(transform.Position);
        }

        [[nodiscard]] static Matrix CreateView(const TransformComponent& transform)
        {
            const Vector3 lookDirection = Vector3::Transform(Vector3::Forward, transform.Rotation);
            return XMMatrixLookToLH(transform.Position, lookDirection, Vector3::Up);
        }
        
        [[nodiscard]] static Vector3 MakeForward(const TransformComponent& transform)
        {
            return Vector3::Transform(Vector3::Forward, transform.Rotation);
        }

        [[nodiscard]] static Vector3 MakeBackward(const TransformComponent& transform)
        {
            return Vector3::Transform(Vector3::Backward, transform.Rotation);
        }

        [[nodiscard]] static Vector3 MakeRight(const TransformComponent& transform)
        {
            return Vector3::Transform(Vector3::Right, transform.Rotation);
        }

        [[nodiscard]] static Vector3 MakeLeft(const TransformComponent& transform)
        {
            return Vector3::Transform(Vector3::Left, transform.Rotation);
        }

        [[nodiscard]] static Vector3 MakeUp(const TransformComponent& transform)
        {
            return Vector3::Transform(Vector3::Up, transform.Rotation);
        }

        [[nodiscard]] static Vector3 MakeDown(const TransformComponent& transform)
        {
            return Vector3::Transform(Vector3::Down, transform.Rotation);
        }
    };

    template <>
    Json& Serialize<Json, TransformComponent>(Json& archive, const TransformComponent& transform);

    template <>
    const Json& Deserialize<Json, TransformComponent>(const Json& archive, TransformComponent& transform);

    template <>
    void OnInspector<TransformComponent>(Registry* registry, const Entity entity);

    IG_DECLARE_META(TransformComponent);
} // namespace ig
