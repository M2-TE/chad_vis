#pragma once
#include <SFML/Window/Event.hpp>
// conditional includes in case imgui is available
#if __has_include(<imgui.h>)
#   include <imgui.h>
#   define IMGUI_CAPTURE_KBD ImGui::GetIO().WantCaptureKeyboard
#   define IMGUI_CAPTURE_MOUSE ImGui::GetIO().WantCaptureMouse
#else
#   define IMGUI_CAPTURE_KBD 0
#   define IMGUI_CAPTURE_MOUSE 0
#endif

namespace Input {
	// data storage for internal use only
	struct Data {
		auto static get() noexcept -> Data& { 
            static Data instance;
            return instance;
        }
		std::set<sf::Keyboard::Scan> keys_pressed, keys_held, keys_released;
		std::set<sf::Mouse::Button> buttons_pressed, buttons_held, buttons_released;
		sf::Vector2i mouse_position;
		sf::Vector2i mouse_delta;
		bool mouse_captured;
	};

	struct Keys {
		// check if (physical) key was pressed in this frame
		bool static inline pressed(sf::Keyboard::Scan code) noexcept { return Data::get().keys_pressed.contains(code); }
		// check if (logical) key was pressed in this frame
		bool static inline pressed(sf::Keyboard::Key code) noexcept { return pressed(sf::Keyboard::delocalize(code)); }
		// check if (logical) key was pressed in this frame
		bool static inline pressed(char character) noexcept { return pressed(sf::Keyboard::delocalize(static_cast<sf::Keyboard::Key>(std::tolower(character)))); }

		// check if (physical) key is being held
		bool static inline held(sf::Keyboard::Scan code) noexcept { return Data::get().keys_held.contains(code); }
		// check if (logical) key is being held
		bool static inline held(sf::Keyboard::Key code) noexcept { return held(sf::Keyboard::delocalize(code)); }
		// check if (logical) key is being held
		bool static inline held(char character) noexcept { return held(sf::Keyboard::delocalize(static_cast<sf::Keyboard::Key>(std::tolower(character)))); }
		
		// check if (physical) key was released in this frame
		bool static inline released(sf::Keyboard::Scan code) noexcept { return Data::get().keys_released.contains(code); }
		// check if (logical) key was released in this frame
		bool static inline released(sf::Keyboard::Key code) noexcept { return released(sf::Keyboard::delocalize(code)); }
		// check if (logical) key was released in this frame
		bool static inline released(char character) noexcept { return released(sf::Keyboard::delocalize(static_cast<sf::Keyboard::Key>(std::tolower(character)))); }
    };
	struct Mouse {
		// check if mouse button was pressed in this frame
		bool static inline pressed(sf::Mouse::Button button) noexcept { return Data::get().buttons_pressed.contains(button); }
		// check if mouse button is being held held
		bool static inline held(sf::Mouse::Button button) noexcept { return Data::get().buttons_held.contains(button); }
		// check if mouse button was released in this frame
		bool static inline released(sf::Mouse::Button button) noexcept { return Data::get().buttons_released.contains(button); }

		// get the current mouse position in screen coordinates
		auto static inline position() noexcept -> sf::Vector2i { return Data::get().mouse_position; };
		// get the change in mouse position since the last frame
		auto static inline delta() noexcept -> sf::Vector2i { return Data::get().mouse_delta; };
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
	// pass an SDL event to the input system
	void inline handle_event(const std::optional<sf::Event> event) noexcept {
		if (event->is<sf::Event::FocusLost>()) {
			// throw away all keyboard states when focus is lost
			flush();
			Data::get().keys_held.clear();
			Data::get().buttons_held.clear();
		}
		else if (const auto* keys_pressed = event->getIf<sf::Event::KeyPressed>()) {
			if (IMGUI_CAPTURE_KBD) return;
			Data::get().keys_pressed.insert(keys_pressed->scancode);
			Data::get().keys_held.insert(keys_pressed->scancode);
		}
		else if (const auto* keys_released = event->getIf<sf::Event::KeyReleased>()) {
			if (IMGUI_CAPTURE_KBD) return;
			Data::get().keys_released.insert(keys_released->scancode);
			Data::get().keys_held.erase(keys_released->scancode);
		}
		else if (const auto* button_pressed = event->getIf<sf::Event::MouseButtonPressed>()) {
			if (IMGUI_CAPTURE_MOUSE) return;
			Data::get().buttons_pressed.insert(button_pressed->button);
			Data::get().buttons_held.insert(button_pressed->button);
		}
		else if (const auto* button_released = event->getIf<sf::Event::MouseButtonReleased>()) {
			if (IMGUI_CAPTURE_MOUSE) return;
			Data::get().buttons_released.insert(button_released->button);
			Data::get().buttons_held.erase(button_released->button);
		}
		#ifdef INPUT_DISABLE_RAW_MOUSE
		else if (const auto* mouse_moved = event->getIf<sf::Event::MouseMoved>()) {
			if (IMGUI_CAPTURE_MOUSE) return;
			Data::get().mouse_delta = Data::get().mouse_position - mouse_moved->position;
			Data::get().mouse_position += mouse_moved->position;
		}
		#else
		else if (const auto* mouse_moved = event->getIf<sf::Event::MouseMovedRaw>()) {
			if (IMGUI_CAPTURE_MOUSE) return;
			Data::get().mouse_position += mouse_moved->delta;
			Data::get().mouse_delta += mouse_moved->delta;
		}
		#endif
	}
	// update the current capture state of the mouse
	void inline register_capture(bool captured) noexcept {
		Data::get().mouse_captured = captured;
	}
}
#undef IMGUI_CAPTURE_EVAL
typedef Input::Keys Keys;
typedef Input::Mouse Mouse;