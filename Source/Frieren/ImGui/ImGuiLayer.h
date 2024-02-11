#pragma once
#include <Core/String.h>

namespace fe
{
	class ImGuiLayer
	{
	public:
		ImGuiLayer(const String newName) : name(newName) {}
		virtual ~ImGuiLayer() = default;
		virtual void Render() = 0;

		String GetName() const { return name; }

	private:
		const String name;
	};
} // namespace fe