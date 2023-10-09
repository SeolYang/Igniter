#pragma once
#include <Core/String.h>
#include <Core/Name.h>
#include <queue>

namespace fe
{
	template <typename T>
	class OwnedHandle
	{
	public:
		bool IsValid() const;

	private:
		uint64_t index;
	};

	template <typename T>
	class WeakHandle
	{
	public:
		bool IsValid() const;

	private:
		uint64_t index;
	};

} // namespace fe