#pragma once
#include <Math/Common.h>

namespace fe
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

	public:
		Vector3 Position{};
		Vector3 Scale{ 1.f, 1.f, 1.f };
		Quaternion Rotation;
	};
} // namespace fe