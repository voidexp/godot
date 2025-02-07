/**************************************************************************/
/*  display_server_mock.h                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef DISPLAY_SERVER_MOCK_H
#define DISPLAY_SERVER_MOCK_H

#include "servers/display_server_headless.h"

#include "servers/rendering/dummy/rasterizer_dummy.h"

// Specialized DisplayServer for unittests based on DisplayServerHeadless, that
// additionally supports rudimentary InputEvent handling and mouse position.
class DisplayServerMock : public DisplayServerHeadless {
private:
	friend class DisplayServer;

	Point2i mouse_position = Point2i(-1, -1); // Outside of Window.
	bool window_over = false;
	Callable event_callback;
	Callable input_event_callback;

	static Vector<String> get_rendering_drivers_func() {
		Vector<String> drivers;
		drivers.push_back("dummy");
		return drivers;
	}

	static DisplayServer *create_func(const String &p_rendering_driver, DisplayServer::WindowMode p_mode, DisplayServer::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, Error &r_error) {
		r_error = OK;
		RasterizerDummy::make_current();
		return memnew(DisplayServerMock());
	}

	static void _dispatch_input_events(const Ref<InputEvent> &p_event) {
		static_cast<DisplayServerMock *>(get_singleton())->_dispatch_input_event(p_event);
	}

	void _dispatch_input_event(const Ref<InputEvent> &p_event) {
		Variant ev = p_event;
		Variant *evp = &ev;
		Variant ret;
		Callable::CallError ce;

		if (input_event_callback.is_valid()) {
			input_event_callback.callp((const Variant **)&evp, 1, ret, ce);
		}
	}

	void _set_mouse_position(const Point2i &p_position) {
		if (mouse_position == p_position) {
			return;
		}
		mouse_position = p_position;
		_set_window_over(Rect2i(Point2i(0, 0), window_get_size()).has_point(p_position));
	}

	void _set_window_over(bool p_over) {
		if (p_over == window_over) {
			return;
		}
		window_over = p_over;
		_send_window_event(p_over ? WINDOW_EVENT_MOUSE_ENTER : WINDOW_EVENT_MOUSE_EXIT);
	}

	void _send_window_event(WindowEvent p_event) {
		if (!event_callback.is_null()) {
			Variant event = int(p_event);
			Variant *eventp = &event;
			Variant ret;
			Callable::CallError ce;
			event_callback.callp((const Variant **)&eventp, 1, ret, ce);
		}
	}

public:
	bool has_feature(Feature p_feature) const override {
		switch (p_feature) {
			case FEATURE_MOUSE:
				return true;
			default: {
			}
		}
		return false;
	}

	String get_name() const override { return "mock"; }

	// You can simulate DisplayServer-events by calling this function.
	// The events will be deliverd to Godot's Input-system.
	// Mouse-events (Button & Motion) will additionally update the DisplayServer's mouse position.
	void simulate_event(Ref<InputEvent> p_event) {
		Ref<InputEventMouse> me = p_event;
		if (me.is_valid()) {
			_set_mouse_position(me->get_position());
		}
		Input::get_singleton()->parse_input_event(p_event);
	}

	virtual Point2i mouse_get_position() const override { return mouse_position; }

	virtual Size2i window_get_size(WindowID p_window = MAIN_WINDOW_ID) const override {
		return Size2i(1920, 1080);
	}

	virtual void window_set_window_event_callback(const Callable &p_callable, WindowID p_window = MAIN_WINDOW_ID) override {
		event_callback = p_callable;
	}

	virtual void window_set_input_event_callback(const Callable &p_callable, WindowID p_window = MAIN_WINDOW_ID) override {
		input_event_callback = p_callable;
	}

	static void register_mock_driver() {
		register_create_function("mock", create_func, get_rendering_drivers_func);
	}

	DisplayServerMock() {
		Input::get_singleton()->set_event_dispatch_function(_dispatch_input_events);
	}
	~DisplayServerMock() {}
};

#endif // DISPLAY_SERVER_MOCK_H
