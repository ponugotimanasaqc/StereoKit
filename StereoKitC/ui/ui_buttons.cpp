#include "../stereokit.h"
#include "../stereokit_ui.h"
#include "../_stereokit_ui.h"
#include "../sk_math.h"
#include "../systems/interaction.h"

namespace sk {

///////////////////////////////////////////
// Button core behavior!                 //
///////////////////////////////////////////

void ui_button_behavior(vec3 window_relative_pos, vec2 size, id_hash_t id, float &finger_offset, button_state_ &button_state, button_state_ &focus_state) {
	button_state = button_state_inactive;
	focus_state  = button_state_inactive;
	int32_t interactor = -1;
	vec3    interaction_at;

	// Button interaction focus is detected out in front of the button to 
	// prevent 'reverse' or 'side' presses where the finger comes from the
	// back or side.
	//
	// Once focused is gained, interaction is tracked within a volume that
	// extends from the detection plane, to a good distance through the
	// button's back. This is to help when the user's finger inevitably goes
	// completely through the button. Width and height is added to this 
	// volume to account for vertical or horizontal movement during a press,
	// such as the downward motion often accompanying a 'poke' motion.
	float interact_radius  = input_hand(handed_right)->fingers[1][4].radius; // TODO: this might be something to revise for interactors?
	float activation_plane = skui_settings.depth + interact_radius*2;
	vec3  activation_start = window_relative_pos + vec3{ 0, 0, -activation_plane };
	vec3  activation_size  = vec3{ size.x, size.y, 0.0001f };

	vec3  box_size  = vec3{ size.x + 2*interact_radius, size.y + 2*interact_radius, activation_plane + 6*interact_radius  };
	vec3  box_start = window_relative_pos + vec3{ interact_radius, interact_radius, -activation_plane + box_size.z };
	interactor_volume_1h(id, interactor_event_poke,
		activation_start, activation_size,
		box_start,        box_size,
		&focus_state, &interactor, &interaction_at);

	// If a hand is interacting, adjust the button surface accordingly
	finger_offset = skui_settings.depth;
	if (focus_state & button_state_active) {
		interactor_t* actor = &skui_interactors[interactor];

		finger_offset = -(interaction_at.z+actor->radius) - window_relative_pos.z;
		bool pressed  = finger_offset < skui_settings.depth / 2;
		if ((actor->active_prev == id && (actor->state & button_state_active)) || (actor->state & button_state_just_active)) {
			pressed = true;
			finger_offset = 0;
		}
		button_state  = interactor_set_active(interactor, id, pressed, interaction_at);
		finger_offset = fminf(fmaxf(2*mm2m, finger_offset), skui_settings.depth);
	} else if (focus_state & button_state_just_inactive) {
		button_state = interactor_set_active(interactor, id, false, interaction_at);
	}
	
	if      (button_state & button_state_just_active)   sound_play(skui_snd_interact,   skui_interactors[interactor]._hit_at_world, 1);
	else if (button_state & button_state_just_inactive) sound_play(skui_snd_uninteract, skui_interactors[interactor]._hit_at_world, 1);
}

///////////////////////////////////////////
// Image buttons                         //
///////////////////////////////////////////

template<typename C>
bool32_t ui_button_img_at_g(const C* text, sprite_t image, ui_btn_layout_ image_layout, vec3 window_relative_pos, vec2 size) {
	id_hash_t     id = ui_stack_hash(text);
	float         finger_offset;
	button_state_ state, focus;
	ui_button_behavior(window_relative_pos, size, id, finger_offset, state, focus);

	if (state & button_state_just_active)
		ui_anim_start(id);
	float color_blend = focus & button_state_active ? 2.f : 1;
	if (ui_anim_has(id, .2f)) {
		float t     = ui_anim_elapsed    (id, .2f);
		color_blend = math_ease_overshoot(1, 2.f, 40, t);
	}
	color_blend = fmaxf(color_blend, 1 + 1 - (finger_offset / skui_settings.depth));

	ui_draw_el(ui_vis_button, window_relative_pos, vec3{ size.x,size.y,finger_offset }, ui_color_common, color_blend);
	
	float pad2       = skui_settings.padding * 2;
	float pad2gutter = pad2 + skui_settings.gutter;
	float depth      = finger_offset + 2 * mm2m;
	vec3  image_at;
	float image_size;
	text_align_ image_align;
	vec3  text_at;
	vec2  text_size;
	text_align_ text_align;
	float aspect = sprite_get_aspect(image);
	switch (image_layout) {
	default:
	case ui_btn_layout_left:
		image_align = text_align_center_left;
		image_size  = fminf(size.y - pad2, ((size.x - pad2gutter)*0.5f) / aspect);
		image_at    = window_relative_pos - vec3{ skui_settings.padding, size.y/2, depth };
			
		text_align = text_align_center_right;
		text_at    = window_relative_pos - vec3{ size.x-skui_settings.padding, size.y/2, depth };
		text_size  = { size.x - (image_size * aspect + pad2gutter), size.y - pad2 };
		break;
	case ui_btn_layout_right:
		image_align = text_align_center_right;
		image_at    = window_relative_pos - vec3{ size.x-skui_settings.padding, size.y / 2, depth };
		image_size  = fminf(size.y - pad2, ((size.x - pad2gutter) * 0.5f) / aspect);
			
		text_align = text_align_center_left;
		text_at    = window_relative_pos - vec3{ skui_settings.padding, size.y / 2, depth };
		text_size  = { size.x - (image_size * aspect + pad2gutter), size.y - pad2 };
		break;
	case ui_btn_layout_center_no_text:
	case ui_btn_layout_center:
		image_align = text_align_center;
		image_size  = fminf(size.y - pad2, (size.x - pad2) / aspect);
		image_at    = window_relative_pos - vec3{ size.x/2, size.y / 2, depth }; 
			
		text_align = text_align_top_center;
		float y = size.y / 2 + image_size / 2;
		text_at    = window_relative_pos - vec3{size.x/2, y, depth};
		text_size  = { size.x-pad2, (size.y-skui_settings.padding*0.25f)-y };
		break;
	}

	if (image_size>0) {
		color128 final_color = skui_tint;
		if (!skui_enabled_stack.last()) final_color = final_color * color128{ .5f, .5f, .5f, 1 };
	
		sprite_draw_at(image, matrix_ts(image_at, { image_size, image_size, image_size }), image_align, color_to_32( final_color ));
		if (image_layout != ui_btn_layout_center_no_text)
			ui_text_in(text_at, text_size, text, text_align, text_align_center);
	}
	return state & button_state_just_active;
}
bool32_t ui_button_img_at   (const char     *text, sprite_t image, ui_btn_layout_ image_layout, vec3 window_relative_pos, vec2 size) { return ui_button_img_at_g<char>(text, image, image_layout, window_relative_pos, size); }
bool32_t ui_button_img_at   (const char16_t *text, sprite_t image, ui_btn_layout_ image_layout, vec3 window_relative_pos, vec2 size) { return ui_button_img_at_g<char16_t>(text, image, image_layout, window_relative_pos, size); }
bool32_t ui_button_img_at_16(const char16_t *text, sprite_t image, ui_btn_layout_ image_layout, vec3 window_relative_pos, vec2 size) { return ui_button_img_at_g<char16_t>(text, image, image_layout, window_relative_pos, size); }

///////////////////////////////////////////

template<typename C>
bool32_t ui_button_img_sz_g(const C *text, sprite_t image, ui_btn_layout_ image_layout, vec2 size) {
	vec3 final_pos;
	vec2 final_size;

	ui_layout_reserve_sz(size, false, &final_pos, &final_size);
	return ui_button_img_at(text, image, image_layout, final_pos, final_size);
}
bool32_t ui_button_img_sz   (const char     *text, sprite_t image, ui_btn_layout_ image_layout, vec2 size) { return ui_button_img_sz_g<char    >(text, image, image_layout, size); }
bool32_t ui_button_img_sz_16(const char16_t *text, sprite_t image, ui_btn_layout_ image_layout, vec2 size) { return ui_button_img_sz_g<char16_t>(text, image, image_layout, size); }

///////////////////////////////////////////

template<typename C, vec2(*text_size_t)(const C *text, text_style_t style)>
bool32_t ui_button_img_g(const C *text, sprite_t image, ui_btn_layout_ image_layout) {
	vec3 final_pos;
	vec2 final_size;

	vec2 size = {};
	if (image_layout == ui_btn_layout_center || image_layout == ui_btn_layout_center_no_text) {
		size = { skui_fontsize, skui_fontsize };
	} else {
		vec2  txt_size   = text_size_t(text, skui_font_stack.last());
		float aspect     = sprite_get_aspect(image);
		float image_size = skui_fontsize * aspect;
		size = vec2{ txt_size.x + image_size + skui_settings.gutter, skui_fontsize };
	}

	ui_layout_reserve_sz(size, true, &final_pos, &final_size);
	return ui_button_img_at(text, image, image_layout, final_pos, final_size);
}
bool32_t ui_button_img   (const char     *text, sprite_t image, ui_btn_layout_ image_layout) { return ui_button_img_g<char,     text_size   >(text, image, image_layout); }
bool32_t ui_button_img_16(const char16_t *text, sprite_t image, ui_btn_layout_ image_layout) { return ui_button_img_g<char16_t, text_size_16>(text, image, image_layout); }

///////////////////////////////////////////
// Labeled buttons                       //
///////////////////////////////////////////

template<typename C>
bool32_t ui_button_at_g(const C *text, vec3 window_relative_pos, vec2 size) {
	id_hash_t     id = ui_stack_hash(text);
	float         finger_offset;
	button_state_ state, focus;
	ui_button_behavior(window_relative_pos, size, id, finger_offset, state, focus);

	if (state & button_state_just_active)
		ui_anim_start(id);
	float color_blend = focus & button_state_active ? 2.f : 1;
	if (ui_anim_has(id, .2f)) {
		float t     = ui_anim_elapsed    (id, .2f);
		color_blend = math_ease_overshoot(1, 2.f, 40, t);
	}
	color_blend = fmaxf(color_blend, 1 + 1 - (finger_offset / skui_settings.depth));

	ui_draw_el(ui_vis_button, window_relative_pos,  vec3{ size.x,   size.y,   finger_offset }, ui_color_common, color_blend);
	ui_text_in(               window_relative_pos - vec3{ size.x/2, size.y/2, finger_offset + 2*mm2m }, vec2{size.x-skui_settings.padding*2, size.y-skui_settings.padding*2}, text, text_align_center, text_align_center);

	return state & button_state_just_active;
}
bool32_t ui_button_at   (const char     *text, vec3 window_relative_pos, vec2 size) { return ui_button_at_g<char    >(text, window_relative_pos, size); }
bool32_t ui_button_at   (const char16_t *text, vec3 window_relative_pos, vec2 size) { return ui_button_at_g<char16_t>(text, window_relative_pos, size); }
bool32_t ui_button_at_16(const char16_t *text, vec3 window_relative_pos, vec2 size) { return ui_button_at_g<char16_t>(text, window_relative_pos, size); }

///////////////////////////////////////////

template<typename C>
bool32_t ui_button_sz_g(const C *text, vec2 size) {
	vec3 final_pos;
	vec2 final_size;
	ui_layout_reserve_sz(size, false, &final_pos, &final_size);

	return ui_button_at(text, final_pos, final_size);
}
bool32_t ui_button_sz   (const char     *text, vec2 size) { return ui_button_sz_g<char    >(text, size); }
bool32_t ui_button_sz_16(const char16_t *text, vec2 size) { return ui_button_sz_g<char16_t>(text, size); }

///////////////////////////////////////////

template<typename C, vec2 (*text_size_t)(const C *text, text_style_t style)>
bool32_t ui_button_g(const C *text) {
	vec3 final_pos;
	vec2 final_size;
	ui_layout_reserve_sz(text_size_t(text, skui_font_stack.last()), true, &final_pos, &final_size);

	return ui_button_at(text, final_pos, final_size);
}
bool32_t ui_button   (const char     *text) { return ui_button_g<char,     text_size   >(text); }
bool32_t ui_button_16(const char16_t *text) { return ui_button_g<char16_t, text_size_16>(text); }

///////////////////////////////////////////
// Toggle buttons                        //
///////////////////////////////////////////

template<typename C>
bool32_t ui_toggle_at_g(const C *text, bool32_t &pressed, vec3 window_relative_pos, vec2 size) {
	id_hash_t     id = ui_stack_hash(text);
	float         finger_offset;
	button_state_ state, focus;
	ui_button_behavior(window_relative_pos, size, id, finger_offset, state, focus);

	if (state & button_state_just_active)
		ui_anim_start(id);
	float color_blend = pressed || focus & button_state_active ? 2.f : 1;
	if (ui_anim_has(id, .2f)) {
		float t     = ui_anim_elapsed    (id, .2f);
		color_blend = math_ease_overshoot(1, 2.f, 40, t);
	}

	if (state & button_state_just_active) {
		pressed = !pressed;
	}
	finger_offset = pressed ? fminf(skui_settings.backplate_depth*skui_settings.depth + mm2m, finger_offset) : finger_offset;

	ui_draw_el(ui_vis_toggle, window_relative_pos,  vec3{ size.x,    size.y,   finger_offset }, ui_color_common, color_blend);
	ui_text_in(               window_relative_pos - vec3{ size.x/2,  size.y/2, finger_offset + 2*mm2m }, vec2{size.x-skui_settings.padding*2, size.y-skui_settings.padding*2}, text, text_align_center, text_align_center);

	return state & button_state_just_active;
}
bool32_t ui_toggle_at   (const char     *text, bool32_t &pressed, vec3 window_relative_pos, vec2 size) { return ui_toggle_at_g<char    >(text, pressed, window_relative_pos, size); }
bool32_t ui_toggle_at   (const char16_t *text, bool32_t &pressed, vec3 window_relative_pos, vec2 size) { return ui_toggle_at_g<char16_t>(text, pressed, window_relative_pos, size); }
bool32_t ui_toggle_at_16(const char16_t *text, bool32_t &pressed, vec3 window_relative_pos, vec2 size) { return ui_toggle_at_g<char16_t>(text, pressed, window_relative_pos, size); }

///////////////////////////////////////////

template<typename C>
bool32_t ui_toggle_sz_g(const C *text, bool32_t &pressed, vec2 size) {
	vec3 final_pos;
	vec2 final_size;
	ui_layout_reserve_sz(size, false, &final_pos, &final_size);

	return ui_toggle_at(text, pressed, final_pos, final_size);
}
bool32_t ui_toggle_sz   (const char     *text, bool32_t &pressed, vec2 size) { return ui_toggle_sz_g<char    >(text, pressed, size); }
bool32_t ui_toggle_sz_16(const char16_t *text, bool32_t &pressed, vec2 size) { return ui_toggle_sz_g<char16_t>(text, pressed, size); }

///////////////////////////////////////////

template<typename C, vec2 (*text_size_t)(const C *text, text_style_t style)>
bool32_t ui_toggle_g(const C *text, bool32_t &pressed) {
	vec3 final_pos;
	vec2 final_size;
	ui_layout_reserve_sz(text_size_t(text, skui_font_stack.last()), true, &final_pos, &final_size);

	return ui_toggle_at(text, pressed, final_pos, final_size);
}
bool32_t ui_toggle   (const char     *text, bool32_t &pressed) { return ui_toggle_g<char,     text_size   >(text, pressed); }
bool32_t ui_toggle_16(const char16_t *text, bool32_t &pressed) { return ui_toggle_g<char16_t, text_size_16>(text, pressed); }

///////////////////////////////////////////
// Round buttons                         //
///////////////////////////////////////////

template<typename C>
bool32_t ui_button_round_at_g(const C *text, sprite_t image, vec3 window_relative_pos, float diameter) {
	id_hash_t     id = ui_stack_hash(text);
	float         finger_offset;
	button_state_ state, focus;
	ui_button_behavior(window_relative_pos, { diameter,diameter }, id, finger_offset, state, focus);

	if (state & button_state_just_active)
		ui_anim_start(id);
	float color_blend = state & button_state_active ? 2.f : 1;
	float back_size   = skui_settings.backplate_border;
	if (ui_anim_has(id, .2f)) {
		float t     = ui_anim_elapsed    (id, .2f);
		color_blend = math_ease_overshoot(1, 2.f, 40, t);
	}

	ui_cylinder(window_relative_pos, diameter, finger_offset, skui_mat, skui_palette[2] * color_blend);
	ui_cylinder(window_relative_pos + vec3{back_size, back_size, mm2m}, diameter+back_size*2, skui_settings.backplate_depth*skui_settings.depth+mm2m, skui_mat, skui_color_border * color_blend);

	float sprite_scale = fmaxf(1, sprite_get_aspect(image));
	float sprite_size  = (diameter * 0.8f) / sprite_scale;
	sprite_draw_at(image, matrix_ts(window_relative_pos + vec3{ -diameter/2, -diameter/2, -(finger_offset + 2*mm2m) }, vec3{ sprite_size, sprite_size, 1 }), text_align_center);

	return state & button_state_just_active;
}
bool32_t ui_button_round_at   (const char     *text, sprite_t image, vec3 window_relative_pos, float diameter) { return ui_button_round_at_g<char    >(text, image, window_relative_pos, diameter); }
bool32_t ui_button_round_at   (const char16_t *text, sprite_t image, vec3 window_relative_pos, float diameter) { return ui_button_round_at_g<char16_t>(text, image, window_relative_pos, diameter); }
bool32_t ui_button_round_at_16(const char16_t *text, sprite_t image, vec3 window_relative_pos, float diameter) { return ui_button_round_at_g<char16_t>(text, image, window_relative_pos, diameter); }

///////////////////////////////////////////

template<typename C>
bool32_t ui_button_round_g(const C *id, sprite_t image, float diameter) {
	if (diameter == 0)
		diameter = ui_line_height();
	vec2 size = vec2{diameter, diameter};
	size = vec2_one * fmaxf(size.x, size.y);

	vec3 final_pos;
	vec2 final_size;
	ui_layout_reserve_sz(size, false, &final_pos, &final_size);

	return ui_button_round_at(id, image, final_pos, final_size.x);
}
bool32_t ui_button_round   (const char     *id, sprite_t image, float diameter) { return ui_button_round_g<char    >(id, image, diameter); }
bool32_t ui_button_round_16(const char16_t *id, sprite_t image, float diameter) { return ui_button_round_g<char16_t>(id, image, diameter); }
}