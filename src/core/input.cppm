module;
#include <set>
#include <cctype>
export module input;

export namespace Input {
	struct MouseVec2 { double x, y; };
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
		enum: int {
			eSpacebar = 32,
			eEscape = 256,
			eF11 = 300,
			eLeftShift = 340,
			eLeftCtrl = 341,
			eLeftAlt = 342,
		};
		// check if key was pressed in this frame
		bool static inline pressed(char character) noexcept { return Data::get().keys_pressed.contains((int)std::toupper(character)); }
		bool static inline pressed(int code) noexcept { return Data::get().keys_pressed.contains(code); }

		// check if key is being held
		bool static inline held(char character) noexcept { return Data::get().keys_held.contains((int)std::toupper(character)); }
		bool static inline held(int code) noexcept { return Data::get().keys_held.contains(code); }

		// check if key was released in this frame
		bool static inline released(char character) noexcept { return Data::get().keys_released.contains((int)std::toupper(character)); }
		bool static inline released(int code) noexcept { return Data::get().keys_released.contains(code); }
    };
	struct Mouse {
		enum: int {
			eLeft = 0,
			eRight = 1,
			eMiddle = 2,
		};
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

	void register_key_press(int key) {
		Data::get().keys_pressed.insert(key);
		Data::get().keys_held.insert(key);
	}
	void register_key_release(int key) {
		Data::get().keys_released.insert(key);
		Data::get().keys_held.erase(key);
	}
	void register_mouse_press(int button) {
		Data::get().buttons_pressed.insert(button);
		Data::get().buttons_held.insert(button);
	}
	void register_mouse_release(int button) {
		Data::get().buttons_released.insert(button);
		Data::get().buttons_held.erase(button);
	}
	void register_mouse_capture(bool captured) {
		Data::get().mouse_captured = captured;
	}
	void register_mouse_move(double xpos, double ypos) {
		Data::get().mouse_delta.x += xpos - Data::get().mouse_position.x;
		Data::get().mouse_delta.y += ypos - Data::get().mouse_position.y;
		Data::get().mouse_position = { xpos, ypos };
	}
}
export using Keys  = Input::Keys;
export using Mouse = Input::Mouse;