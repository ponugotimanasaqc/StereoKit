#include "../stereokit.h"
#include "../stereokit_ui.h"
#include "../_stereokit_ui.h"
#include "../sk_math.h"
#include "../libraries/unicode.h"
#include "ui_interaction.h"

namespace sk {

id_hash_t skui_input_target = 0;
int32_t   skui_input_carat = 0;
int32_t   skui_input_carat_end = 0;
float     skui_input_blink = 0;

///////////////////////////////////////////

inline bool    utf_is_start     (char     ch) { return utf8_is_start(ch); }
inline bool    utf_is_start     (char16_t ch) { return utf16_is_start(ch); }
inline int32_t utf_encode_append(char     *buffer, size_t size, char32_t ch) { return utf8_encode_append (buffer, size, ch); }
inline int32_t utf_encode_append(char16_t *buffer, size_t size, char32_t ch) { return utf16_encode_append(buffer, size, ch); }

inline vec2 text_char_at_o(const char*     text, text_style_t style, int32_t char_index, vec2* opt_size, text_fit_ fit, text_align_ position, text_align_ align) { return text_char_at   (text, style, char_index, opt_size, fit, position, align); }
inline vec2 text_char_at_o(const char16_t* text, text_style_t style, int32_t char_index, vec2* opt_size, text_fit_ fit, text_align_ position, text_align_ align) { return text_char_at_16(text, style, char_index, opt_size, fit, position, align); }

template<typename C, vec2 (*text_size_t)(const C *text, text_style_t style)>
bool32_t ui_input_g(const C *id, C *buffer, int32_t buffer_size, vec2 size, text_context_ type) {
	vec3 final_pos;
	vec2 final_size;
	ui_layout_reserve_sz(size, false, &final_pos, &final_size);

	id_hash_t id_hash  = ui_stack_hash(id);
	bool      result   = false;
	vec3      box_size = vec3{ final_size.x, final_size.y, skui_settings.depth };

	// Find out if the user is trying to focus this UI element
	button_state_ focus;
	int32_t       interactor;
	vec3          interaction_at;
	interactor_volume_1h(id_hash, interactor_event_poke, final_pos, box_size, final_pos, box_size, &focus, &interactor, &interaction_at);

	bool32_t is_active = interactor != -1 && (
		(skui_interactors[interactor].type == interactor_type_point && (focus & button_state_active)) ||
		(skui_interactors[interactor].type == interactor_type_ray   && (skui_interactors[interactor].state & button_state_just_active)));
	button_state_ state = interactor_set_active(interactor, id_hash, is_active, interaction_at);

	if (state & button_state_just_active) {
		platform_keyboard_show(true,type);
		skui_input_blink  = time_getf();
		skui_input_target = id_hash;
		skui_input_carat  = skui_input_carat_end = (int32_t)utf_charlen(buffer);
		sound_play(skui_snd_interact, skui_interactors[interactor]._hit_at_world, 1);
	}

	if (state & button_state_just_active)
		ui_anim_start(id_hash);
	float color_blend = (skui_input_target == id_hash) || (focus & button_state_active) ? 2.f : 1;
	if (ui_anim_has(id_hash, .2f)) {
		float t     = ui_anim_elapsed    (id_hash, .2f);
		color_blend = math_ease_overshoot(1, 2.f, 40, t);
	}

	// Unfocus this if the user starts interacting with something else
	if (skui_input_target == id_hash) {
		for (int32_t i = 0; i < skui_interactors.count; i++) {
			if (interactor_is_preoccupied(i, id_hash, false)) {
				const interactor_t *actor = &skui_interactors[i];
				if (actor->focused && skui_preserve_keyboard_ids_read->index_of(actor->focused) < 0) {
					skui_input_target = 0;
					platform_keyboard_show(false, type);
				}
			}
		}
	}

	// If focused, acquire any input in the keyboard's queue
	if (skui_input_target == id_hash) {
		uint32_t curr = input_text_consume();
		while (curr != 0) {
			uint32_t add = '\0';

			if (curr == key_backspace) {
				if (skui_input_carat != skui_input_carat_end) {
					int32_t start = mini(skui_input_carat, skui_input_carat_end);
					int32_t count = maxi(skui_input_carat, skui_input_carat_end) - start;
					utf_remove_chars(utf_advance_chars(buffer, start), count);
					skui_input_carat_end = skui_input_carat = start;
					result = true;
				} else if (skui_input_carat > 0) {
					skui_input_carat_end = skui_input_carat = skui_input_carat - 1;
					utf_remove_chars(utf_advance_chars(buffer, skui_input_carat), 1);
					result = true;
				}
			} else if (curr == 0x7f) {
				if (skui_input_carat != skui_input_carat_end) {
					int32_t start = mini(skui_input_carat, skui_input_carat_end);
					int32_t count = maxi(skui_input_carat, skui_input_carat_end) - start;
					utf_remove_chars(utf_advance_chars(buffer, start), count);
					skui_input_carat_end = skui_input_carat = start;
					result = true;
				} else if (skui_input_carat >= 0) {
					utf_remove_chars(utf_advance_chars(buffer, skui_input_carat), 1);
					result = true;
				}
			} else if (curr == 0x0D) { // Enter, carriage return
				skui_input_target = 0;
				platform_keyboard_show(false, type);
				result = true;
			} else if (curr == 0x0A) { // Shift+Enter, linefeed
				add = '\n';
			} else if (curr == 0x1B) { // Escape
				skui_input_target = 0;
				platform_keyboard_show(false, type);
			} else {
				add = curr;
			}

			if (add != '\0') {
				// Remove any selected
				if (skui_input_carat != skui_input_carat_end) {
					int32_t start = mini(skui_input_carat, skui_input_carat_end);
					int32_t count = maxi(skui_input_carat, skui_input_carat_end) - start;
					utf_remove_chars(utf_advance_chars(buffer, start), count);
					skui_input_carat_end = skui_input_carat = start;
				}
				utf_insert_char(buffer, buffer_size, utf_advance_chars(buffer, skui_input_carat), add);
				skui_input_carat += 1;
				skui_input_carat_end = skui_input_carat;
				result = true;
			}

			curr = input_text_consume();
		}
		if      (input_key(key_shift) & button_state_active && input_key(key_left ) & button_state_just_active) { skui_input_blink = time_getf(); skui_input_carat = maxi(0, skui_input_carat - 1); }
		else if (input_key(key_left ) & button_state_just_active)                                               { skui_input_blink = time_getf(); if (skui_input_carat_end == skui_input_carat) skui_input_carat = maxi(0, skui_input_carat - 1); skui_input_carat_end = skui_input_carat; }
		if      (input_key(key_shift) & button_state_active && input_key(key_right) & button_state_just_active) { skui_input_blink = time_getf(); skui_input_carat = mini((int32_t)utf_charlen(buffer), skui_input_carat + 1); }
		else if (input_key(key_right) & button_state_just_active)                                               { skui_input_blink = time_getf(); if (skui_input_carat_end == skui_input_carat) skui_input_carat = mini((int32_t)utf_charlen(buffer), skui_input_carat + 1); skui_input_carat_end = skui_input_carat; }
	}

	// Render the input UI
	vec2 text_bounds = { final_size.x - skui_settings.padding * 2,final_size.y - skui_settings.padding * 2 };
	ui_draw_el(ui_vis_input, final_pos, vec3{ final_size.x, final_size.y, skui_settings.depth/2 }, ui_color_common, color_blend);
	ui_text_in(              final_pos - vec3{ skui_settings.padding, skui_settings.padding, skui_settings.depth/2 + 2*mm2m }, text_bounds, buffer, text_align_top_left, text_align_center_left);
	
	float line      = ui_line_height() * 0.5f;
	vec2  carat_pos = text_char_at_o(buffer, skui_font_stack.last(), skui_input_carat, &text_bounds, text_fit_squeeze, text_align_top_left, text_align_center_left);
	if (skui_input_carat != skui_input_carat_end) {
		vec2  carat_end = text_char_at_o(buffer, skui_font_stack.last(), skui_input_carat_end, &text_bounds, text_fit_squeeze, text_align_top_left, text_align_center_left);
		float left      = fmaxf(carat_pos.x, carat_end.x);
		float right     = fminf(carat_pos.x, carat_end.x);
		ui_cube(final_pos - vec3{ skui_settings.padding - left, skui_settings.padding - carat_pos.y, skui_settings.depth/2 + 1*mm2m }, vec3{ -(right-left), line, line * 0.01f }, skui_mat, skui_palette[3]);
	}
	// Show a blinking text carat
	if (skui_input_target == id_hash && (int)((time_getf()-skui_input_blink)*2)%2==0) {
		
		ui_cube(final_pos - vec3{ skui_settings.padding - carat_pos.x, skui_settings.padding - carat_pos.y, skui_settings.depth/2 }, vec3{ line * 0.1f, line, line * 0.1f }, skui_mat, skui_palette[4]);
	}

	return result;
}

bool32_t ui_input(const char *id, char *buffer, int32_t buffer_size, vec2 size, text_context_ type) {
	return ui_input_g<char, text_size>(id, buffer, buffer_size, size, type);
}
bool32_t ui_input_16(const char16_t *id, char16_t *buffer, int32_t buffer_size, vec2 size, text_context_ type) {
	return ui_input_g<char16_t, text_size_16>(id, buffer, buffer_size, size, type);
}

///////////////////////////////////////////

bool32_t ui_has_keyboard_focus() {
	return skui_input_target != 0;
}

}