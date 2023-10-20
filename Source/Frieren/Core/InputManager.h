#pragma once

namespace fe
{
    class InputManager
    {
	public:
		InputManager() = default;
		InputManager(const InputManager&) = delete;
		InputManager(InputManager&&) = delete;

		InputManager& operator=(const InputManager&) = delete;
		InputManager& operator=(InputManager&&) = delete;
    };
}