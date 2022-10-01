﻿#include "virtual_keyboard.h"
#include "virtual_keyboard_layouts.h"
#include "../stereokit_ui.h"
#include "../systems/input_keyboard.h"
#include "../libraries/array.h"
#include "../libraries/unicode.h"

namespace sk {

bool32_t           keyboard_open         = false;
pose_t             keyboard_pose         = pose_identity;
array_t<key_>      keyboard_pressed_keys = {};
const keylayout_t* keyboard_active_layout;
text_context_      keyboard_text_context;

bool32_t           keyboard_fn    = false;
bool32_t           keyboard_altgr = false;
bool32_t           keyboard_shift = false;
bool32_t           keyboard_ctrl  = false;
bool32_t           keyboard_alt   = false;

///////////////////////////////////////////

const keylayout_t* virtualkeyboard_get_system_keyboard_layout() {
	return &virtualkeyboard_layout_en_us;
}

///////////////////////////////////////////

void virtualkeyboard_reset_modifiers() {
	if (keyboard_alt  ) { input_keyboard_inject_release(key_alt  ); keyboard_alt   = false; }
	if (keyboard_altgr) { input_keyboard_inject_release(key_alt  ); keyboard_altgr = false; }
	if (keyboard_ctrl ) { input_keyboard_inject_release(key_ctrl ); keyboard_ctrl  = false; }
	if (keyboard_shift) { input_keyboard_inject_release(key_shift); keyboard_shift = false; }
	if (keyboard_fn   ) {                                           keyboard_fn    = false; }
}

///////////////////////////////////////////

void virtualkeyboard_open(bool32_t open, text_context_ type) {
	if (open == keyboard_open && type == keyboard_text_context) return;

	// Position the keyboard in front of the user if this just opened
	if (open && !keyboard_open) {
		vec3 at;
		if (ui_last_element_active() & button_state_active) {
			log_info("Last element active");
			// If there was a UI element focused, we'll use that
			at = hierarchy_to_world_point( ui_layout_last().center );
		} else {
			bool active_left  = input_hand(handed_left )->tracked_state & button_state_active;
			bool active_right = input_hand(handed_right)->tracked_state & button_state_active;

			if (active_left && active_right) {
				// Both hands are active, pick the hand that's the most high
				// and outstretched.
				vec3  pl       = input_hand(handed_left)->fingers[1][4].position;
				vec3  pr       = input_hand(handed_right)->fingers[1][4].position;
				vec3  head     = input_head()->position;
				float dist_l   = vec3_distance(pl, head);
				float dist_r   = vec3_distance(pr, head);
				float height_l = pl.y - pr.y;
				float height_r = pr.y - pl.y;
				at = dist_l + height_l / 2.0f > dist_r + height_r / 2.0f ? pl : pr;
			} else if (active_left) {
				at = input_hand(handed_left)->fingers[1][4].position;
			} else if (active_right) {
				at = input_hand(handed_right)->fingers[1][4].position;
			} else {
				// Head based fallback!
				at = input_head()->position + input_head()->orientation * vec3_forward * 0.35f;
			}
		}

		vec3 dir = at - input_head()->position;
		at = input_head()->position + dir * 0.7f;

		matrix to_local = matrix_invert(render_get_cam_root());
		vec3   local_at = matrix_transform_pt(to_local, at);
		keyboard_pose.position    = local_at + vec3{ 0, -.1f, 0 };
		keyboard_pose.orientation = quat_lookat(vec3_zero, matrix_transform_dir(to_local, -dir));
	}

	// Reset the keyboard to its default state
	virtualkeyboard_reset_modifiers();

	keyboard_text_context = type;
	keyboard_open         = open;
}

///////////////////////////////////////////

bool virtualkeyboard_get_open() {
	return keyboard_open;
}

///////////////////////////////////////////

void virtualkeyboard_initialize() {
	keyboard_active_layout = virtualkeyboard_get_system_keyboard_layout();
}

///////////////////////////////////////////

void send_key_data(const char16_t* charkey,key_ key) {
	keyboard_pressed_keys.add(key);
	input_keyboard_inject_press(key);
	char32_t c = 0;
	while (utf16_decode_fast_b(charkey, &charkey, &c)) {
		input_text_inject_char(c);
	}
}

///////////////////////////////////////////

void remove_last_clicked_keys() {
	for (int32_t i = 0; i < keyboard_pressed_keys.count; i++) {
		input_keyboard_inject_release(keyboard_pressed_keys[0]);
		keyboard_pressed_keys.remove(0);
	}
}

///////////////////////////////////////////

void virtualkeyboard_keypress(keylayout_key_t key) {
	send_key_data(key.clicked_text, key.key_event_type);
	if (key.special_key == special_key_close_keyboard) {
		keyboard_open = false;
	}
	virtualkeyboard_reset_modifiers();
}

///////////////////////////////////////////

void virtualkeyboard_update() {
	if (!keyboard_open) return;
	if (keyboard_active_layout == nullptr) return;

	remove_last_clicked_keys();
	ui_push_preserve_keyboard(true);
	hierarchy_push(render_get_cam_root());
	ui_window_begin("SK/Keyboard", keyboard_pose, {0,0}, ui_win_body);

	// Check layer keys for switching between keyboard layout layers
	int layer_index = 0;
	if      (keyboard_shift && keyboard_altgr) layer_index = 3;
	else if (keyboard_fn)                      layer_index = 4;
	else if (keyboard_shift)                   layer_index = 1;
	else if (keyboard_altgr)                   layer_index = 2;

	// Until we have more, this is capped at 2
	assert(layer_index < 2);

	// Get the right layout for this text context
	const keylayout_key_t* layer = nullptr;
	switch (keyboard_text_context) {
	case text_context_text:  layer = keyboard_active_layout->text  [layer_index]; break;
	case text_context_number:layer = keyboard_active_layout->number[layer_index]; break;
	case text_context_uri:   layer = keyboard_active_layout->uri   [layer_index]; break;
	default:                 layer = keyboard_active_layout->text  [layer_index]; break;
	//case text_context_number_decimal:        layer = &keyboard_active_layout->number_decimal_layer       [layer_index]; break;
	//case text_context_number_signed:         layer = &keyboard_active_layout->number_signed_layer        [layer_index]; break;
	//case text_context_number_signed_decimal: layer = &keyboard_active_layout->number_signed_decimal_layer[layer_index]; break;
	}

	// Draw the keyboard
	float   button_size = ui_line_height();
	int32_t i           = 0;
	while (layer[i].special_key != special_key_end) {
		const keylayout_key_t *curr = &layer[i];
		i += 1;
		vec2 size = vec2{ button_size * curr->width, button_size };
		if (curr->special_key == special_key_nextline) ui_nextline();
		if (curr->width == 0) continue;

		ui_push_idi(i + 1000);
		switch(curr->special_key) {
		case special_key_spacer:   ui_layout_reserve(size); break;
		case special_key_nextline: break;
		case special_key_fn:       ui_toggle_sz_16(curr->display_text, keyboard_fn, size); break;
		case special_key_alt: {
			if (ui_toggle_sz_16(curr->display_text, keyboard_alt, size)) {
				if (keyboard_alt) input_keyboard_inject_press  (key_alt);
				else              input_keyboard_inject_release(key_alt);
			}
		} break;
		case special_key_ctrl: {
			if (ui_toggle_sz_16(curr->display_text, keyboard_ctrl, size)) {
				if (keyboard_ctrl) input_keyboard_inject_press  (key_ctrl);
				else               input_keyboard_inject_release(key_ctrl);
			}
		} break;
		case special_key_shift: {
			if (ui_toggle_sz_16(curr->display_text, keyboard_shift, size)) {
				if (keyboard_shift) input_keyboard_inject_press  (key_shift);
				else                input_keyboard_inject_release(key_shift);
			}
		} break;
		case special_key_alt_gr: {
			if (ui_toggle_sz_16(curr->display_text, keyboard_altgr, size)) {
				if (keyboard_altgr) input_keyboard_inject_press  (key_alt);
				else                input_keyboard_inject_release(key_alt);
			}
		} break;
		default: {
			if (ui_button_sz_16(curr->display_text, size))
				virtualkeyboard_keypress(*curr);
		} break;
		}
		ui_pop_id();
		ui_sameline();
	}
	ui_window_end();
	hierarchy_pop();
	ui_pop_preserve_keyboard();
}

}