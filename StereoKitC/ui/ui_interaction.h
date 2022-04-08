#include "../stereokit_ui.h"
#include "../_stereokit_ui.h"
#include "../libraries/array.h"

///////////////////////////////////////////

namespace sk {

enum ui_interactor_type_ {
	ui_interactor_type_point,
	ui_interactor_type_ray,
};

enum ui_interactor_event_ {
	ui_interactor_event_poke  = 1 << 1,
	ui_interactor_event_grip  = 1 << 2,
	ui_interactor_event_pinch = 1 << 3
};

enum ui_2h_state_ {
	ui_2h_state_none             = 0,
	ui_2h_state_1h_active        = 1 << 1,
	ui_2h_state_1h_just_active   = 2 << 1,
	ui_2h_state_1h_just_inactive = 3 << 1,
	ui_2h_state_2h_active        = 4 << 1,
	ui_2h_state_2h_just_active   = 5 << 1,
	ui_2h_state_2h_just_inactive = 6 << 1,
};

struct ui_interactor_pos_t {
	union {
		struct { ray_t ray; };
		struct { vec3  at, at_prev; };
	};
};

struct ui_interactor_t {
	ui_interactor_type_  type;
	ui_interactor_event_ events;
	button_state_        state;
	button_state_        tracked;
	float                activation;
	float                radius;
	pose_t               pose_world;
	pose_t               pose_local;
	pose_t               action_pose_world;
	ui_interactor_pos_t  pos_world;
	ui_interactor_pos_t  pos_local;
	ui_interactor_pos_t  action_pos_world;

	float                focus_priority;
	uint64_t             focused_prev;
	uint64_t             focused;
	uint64_t             active_prev;
	uint64_t             active;
};

bool32_t      ui_in_box                   (vec3 pt1, vec3 pt2, float radius, bounds_t box);
bool32_t      ui_intersect_box            (ray_t ray, bounds_t box, float *out_distance);
bool32_t      ui_interact_box             (const ui_interactor_t *interactor, bounds_t box, float *out_focus);

void          ui_interaction_1h           (uint64_t id, ui_interactor_event_ event_mask, vec3 unfocused_start, vec3 unfocused_size, vec3 focused_start, vec3 focused_size, button_state_ *out_focus_state, int32_t *out_interactor);
void          ui_interaction_2h           (uint64_t id, ui_interactor_event_ event_mask, bounds_t bounds, button_state_ *out_focus_state, int32_t *out_interactor);

bool32_t      ui_interactor_is_preoccupied(int32_t interactor, uint64_t for_el_id, bool32_t include_focused);
button_state_ ui_interactor_set_focus     (int32_t interactor, uint64_t for_el_id, bool32_t focused, float priority);
button_state_ ui_interactor_set_active    (int32_t interactor, uint64_t for_el_id, bool32_t active);
int32_t       ui_interactor_last_active   (uint64_t for_el_id);
int32_t       ui_interactor_last_focused  (uint64_t for_el_id);

inline bounds_t ui_size_box(vec3 top_left, vec3 dimensions) {
	return { top_left - dimensions/2, dimensions };
}

extern array_t<ui_interactor_t> skui_interactors;

}