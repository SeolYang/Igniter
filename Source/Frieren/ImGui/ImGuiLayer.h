#pragma once

namespace fe
{
    class ImGuiLayer
    {
	public:
		virtual ~ImGuiLayer() = default;
		virtual void Render() = 0;
    };
}