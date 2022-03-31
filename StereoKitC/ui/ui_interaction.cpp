#include "ui_interaction.h"
#include "../sk_math.h"

namespace sk {

array_t<ui_interactor_t> skui_interactors = { };

///////////////////////////////////////////

bool32_t ui_in_box(vec3 pt, vec3 pt_prev, float radius, bounds_t box) {
	if (skui_show_volumes)
		render_add_mesh(skui_box_dbg, skui_mat_dbg, matrix_trs(box.center, quat_identity, box.dimensions));
	return bounds_capsule_contains(box, pt, pt_prev, radius);
}

///////////////////////////////////////////

bool32_t ui_intersect_box(ray_t ray, bounds_t box, float *out_distance) {
	if (skui_show_volumes)
		render_add_mesh(skui_box_dbg, skui_mat_dbg, matrix_trs(box.center, quat_identity, box.dimensions));
	vec3 at;
	return bounds_ray_intersect(box, ray, &at);
}

///////////////////////////////////////////

bool32_t ui_interact_box(const ui_interactor_t *actor, bounds_t box, float *out_priority) {
	*out_priority = 0;
	if (actor->tracked & button_state_active) {
		switch (actor->type) {
		case ui_interactor_type_point: return ui_in_box       (actor->pos_local.at, actor->pos_local.at_prev, actor->radius, box); break;
		case ui_interactor_type_ray:   return ui_intersect_box(actor->pos_local.ray, box, out_priority);                           break;
		}
	}
	return false;
}

///////////////////////////////////////////

void ui_interaction_1h(uint64_t id, ui_interactor_event_ event_mask, vec3 box_unfocused_start, vec3 box_unfocused_size, vec3 box_focused_start, vec3 box_focused_size, button_state_ *out_focus_state, int32_t *out_interactor) {
	*out_interactor  = -1;
	*out_focus_state = button_state_inactive;

	// If the element is disabled, leave it unfocused and ditch out
	if (!skui_interact_enabled) { return; }

	if (skui_preserve_keyboard_stack.last()) {
		skui_preserve_keyboard_ids.add(id);
	}

	for (int32_t i = 0; i < skui_interactors.count; i++) {
		const ui_interactor_t *actor = &skui_interactors[i];
		if (ui_interactor_is_preoccupied(i, id, false) ||
			(actor->events & event_mask) == 0)
			continue;

		bool was_focused = actor->focused_prev == id;
		vec3 box_start   = box_unfocused_start;
		vec3 box_size    = box_unfocused_size;
		if (was_focused) {
			box_start = box_focused_start;
			box_size  = box_focused_size;
		}

		float         priority = 0;
		bool          in_box   = ui_interact_box(actor, ui_size_box(box_start, box_size), &priority);
		button_state_ focus    = ui_interactor_set_focus(i, id, in_box, priority);
		if (focus != button_state_inactive) {
			*out_interactor  = i;
			*out_focus_state = focus;
		}
	}

	if (*out_interactor == -1)
		*out_interactor = ui_interactor_last_focused(id);
}

///////////////////////////////////////////

bool32_t ui_interactor_is_preoccupied(int32_t interactor, uint64_t for_el_id, bool32_t include_focused) {
	const ui_interactor_t *actor = &skui_interactors[interactor];
	return (include_focused &&  actor->focused_prev != 0 && actor->focused_prev != for_el_id)
	                        || (actor->active_prev  != 0 && actor->active_prev  != for_el_id);
}

///////////////////////////////////////////

button_state_ ui_interactor_set_focus(int32_t interactor, uint64_t for_el_id, bool32_t focused, float priority) {
	if (interactor == -1) return button_state_inactive;

	ui_interactor_t *actor = &skui_interactors[interactor];
	bool was_focused = actor->focused_prev == for_el_id;
	bool is_focused  = false;

	/*if (hand == -1) {
		if      (skui_hand[0].active_prev == for_el_id) hand = 0;
		else if (skui_hand[1].active_prev == for_el_id) hand = 1;
	}*/
	if (focused && priority <= actor->focus_priority) {
		is_focused = focused;
		actor->focused        = for_el_id;
		actor->focus_priority = priority;
	}

	button_state_ result = button_state_inactive;
	if ( is_focused                ) result  = button_state_active;
	if ( is_focused && !was_focused) result |= button_state_just_active;
	if (!is_focused &&  was_focused) result |= button_state_just_inactive;
	return result;
}

///////////////////////////////////////////

button_state_ ui_interactor_set_active(int32_t interactor, uint64_t for_el_id, bool32_t active) {
	if (interactor == -1) return button_state_inactive;

	ui_interactor_t *actor = &skui_interactors[interactor];
	bool was_active = actor->active_prev == for_el_id;
	bool is_active  = false;

	if (active && (was_active || actor->focused_prev == for_el_id || actor->focused == for_el_id)) {
		is_active = active;
		actor->active = for_el_id;
	}

	button_state_ result = button_state_inactive;
	if ( is_active               ) result  = button_state_active;
	if ( is_active && !was_active) result |= button_state_just_active; 
	if (!is_active &&  was_active) result |= button_state_just_inactive;
	return result;
}

///////////////////////////////////////////

int32_t ui_interactor_last_active(uint64_t for_el_id) {
	return (int32_t)skui_interactors.index_where(&ui_interactor_t::active_prev, for_el_id);
}

///////////////////////////////////////////

int32_t ui_interactor_last_focused(uint64_t for_el_id){
	return (int32_t)skui_interactors.index_where(&ui_interactor_t::focused_prev, for_el_id);
}

}