#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Meta.h"

namespace ig
{
    struct TransformComponent
    {
    public:
        [[nodiscard]] Matrix CreateTransformation() const
        {
            // TRS -> Column Vector ; SRT -> Row Vector
            return Matrix::CreateScale(Scale) * Matrix::CreateFromQuaternion(Rotation) * Matrix::CreateTranslation(Position);
        }

        [[nodiscard]] Matrix CreateView() const
        {
            const Vector3 lookDirection = Vector3::Transform(Vector3::Forward, Rotation);
            return XMMatrixLookToLH(Position, lookDirection, Vector3::Up);
        }

        [[nodiscard]] Vector3 GetForward() const { return Vector3::Transform(Vector3::Forward, Rotation); }

        [[nodiscard]] Vector3 GetBackward() const { return Vector3::Transform(Vector3::Backward, Rotation); }

        [[nodiscard]] Vector3 GetRight() const { return Vector3::Transform(Vector3::Right, Rotation); }

        [[nodiscard]] Vector3 GetLeft() const { return Vector3::Transform(Vector3::Left, Rotation); }

        [[nodiscard]] Vector3 GetDown() const { return Vector3::Transform(Vector3::Down, Rotation); }

        [[nodiscard]] Vector3 GetUp() const { return Vector3::Transform(Vector3::Up, Rotation); }

        Json&       Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);
        static void OnInspector(Registry* registry, const Entity entity);

    public:
        Vector3    Position{ };
        Vector3    Scale{1.f, 1.f, 1.f};
        Quaternion Rotation;
    };

    IG_DECLARE_TYPE_META(TransformComponent);
} // namespace ig
