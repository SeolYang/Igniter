#pragma once
#include <Igniter.h>
#include <Gameplay/ComponentRegistry.h>

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

        [[nodiscard]] Vector3 GetForwardDirection() const { return Vector3::Transform(Vector3::Forward, Rotation); }

        [[nodiscard]] Vector3 GetBackwardDirection() const { return Vector3::Transform(Vector3::Backward, Rotation); }

        [[nodiscard]] Vector3 GetRightDirection() const { return Vector3::Transform(Vector3::Right, Rotation); }

        [[nodiscard]] Vector3 GetLeftDirection() const { return Vector3::Transform(Vector3::Left, Rotation); }

        [[nodiscard]] Vector3 GetDownDirection() const { return Vector3::Transform(Vector3::Down, Rotation); }

        [[nodiscard]] Vector3 GetUpDirection() const { return Vector3::Transform(Vector3::Up, Rotation); }

    public:
        Vector3 Position{};
        Vector3 Scale{1.f, 1.f, 1.f};
        Quaternion Rotation;
    };

    IG_DECLARE_COMPONENT(TransformComponent);
}    // namespace ig
