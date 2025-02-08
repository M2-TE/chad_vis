#pragma once
#include <GLFW/glfw3.h>

namespace Input {
	struct MouseVec2 { int32_t x, y; };
	// data storage for internal use only
	struct Data {
		auto static get() noexcept -> Data& { 
            static Data instance;
            return instance;
        }
		std::set<int> keys_pressed, keys_held, keys_released;
		std::set<int> buttons_pressed, buttons_held, buttons_released;
		MouseVec2 mouse_position;
		MouseVec2 mouse_delta;
		bool mouse_captured;
	};

	struct Keys {
		// check if key was pressed in this frame
		bool static inline pressed(int code) noexcept { return Data::get().keys_pressed.contains(code); }
		bool static inline pressed(char code) noexcept { return Data::get().keys_pressed.contains((int)std::toupper(code)); }

		// check if key is being held
		bool static inline held(int code) noexcept { return Data::get().keys_held.contains(code); }
		bool static inline held(char code) noexcept { return Data::get().keys_held.contains((int)std::toupper(code)); }

		// check if key was released in this frame
		bool static inline released(int code) noexcept { return Data::get().keys_released.contains(code); }
		bool static inline released(char code) noexcept { return Data::get().keys_released.contains((int)std::toupper(code)); }
    };
	struct Mouse {
		// set capture mode of the mouse
		void static set_mode(GLFWwindow* window_p, bool captured) {
			glfwSetInputMode(window_p, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
			Data::get().mouse_captured = captured;
		}
		// check if mouse button was pressed in this frame
		bool static inline pressed(int button) noexcept { return Data::get().buttons_pressed.contains(button); }
		// check if mouse button is being held held
		bool static inline held(int button) noexcept { return Data::get().buttons_held.contains(button); }
		// check if mouse button was released in this frame
		bool static inline released(int button) noexcept { return Data::get().buttons_released.contains(button); }

		// get the current mouse position in screen coordinates
		auto static inline position() noexcept -> MouseVec2 { return Data::get().mouse_position; };
		// get the change in mouse position since the last frame
		auto static inline delta() noexcept -> MouseVec2 { return Data::get().mouse_delta; };
		// check if mouse is captured by the window
		bool static inline captured() noexcept { return Data::get().mouse_captured; }
	};
	
	// clear single-frame events
    void inline flush() noexcept {
		Data::get().keys_pressed.clear();
		Data::get().keys_released.clear();
		Data::get().buttons_pressed.clear();
		Data::get().buttons_released.clear();
		Data::get().mouse_delta = {0, 0};
	}
	// clear all events
	void inline clear() noexcept {
		flush();
		Data::get().keys_held.clear();
		Data::get().buttons_held.clear();
	}

	// callback for key events
	[[maybe_unused]]
	void static key_callback(GLFWwindow* window_p, int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS) {
			Data::get().keys_pressed.insert(key);
			Data::get().keys_held.insert(key);
		}
		else if (action == GLFW_RELEASE) {
			Data::get().keys_released.insert(key);
			Data::get().keys_held.erase(key);
		}
		(void)window_p;
		(void)scancode;
		(void)mods;
	}

	// callback for mouse button events
	[[maybe_unused]]
	void static mouse_button_callback(GLFWwindow* window_p, int button, int action, int mods) {
		if (action == GLFW_PRESS) {
			Data::get().buttons_pressed.insert(button);
			Data::get().buttons_held.insert(button);
		}
		else if (action == GLFW_RELEASE) {
			Data::get().buttons_released.insert(button);
			Data::get().buttons_held.erase(button);
		}
		(void)window_p;
		(void)mods;
	}

	// callback for mouse movement
	[[maybe_unused]]
	void static mouse_position_callback(GLFWwindow* window_p, double xpos, double ypos) {
		Data::get().mouse_delta.x += (int32_t)xpos - Data::get().mouse_position.x;
		Data::get().mouse_delta.y += (int32_t)ypos - Data::get().mouse_position.y;
		Data::get().mouse_position = { (int32_t)xpos, (int32_t)ypos };
		(void)window_p;
	}
}
typedef Input::Keys Keys;
typedef Input::Mouse Mouse;