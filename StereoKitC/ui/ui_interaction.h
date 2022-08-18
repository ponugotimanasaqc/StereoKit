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
	// What type of interactions does this provide
	ui_interactor_type_  type;
	ui_interactor_event_ events;
	
	// What is the current state of the interaction
	button_state_        state;
	button_state_        tracked;
	float                activation;
	
	float                focused_priority;
	ui_hash_t            focused_prev;
	ui_hash_t            focused;
	ui_hash_t            active_prev;
	ui_hash_t            active;
	
	// Information about the location and shape of the interactor
	float                radius;
	float                rot_smoothing;
	float                pos_smoothing;
	
	pose_t               motion_pose_world;
	pose_t               motion_pose_local;
	pose_t               motion_pose_world_action;
	ui_interactor_pos_t  hit_test_world;
	ui_interactor_pos_t  hit_test_local;
	ui_interactor_pos_t  hit_test_world_action;
};

typedef int32_t ui_interactor_id_t;

bool32_t           ui_in_box                   (vec3 pt1, vec3 pt2, float radius, bounds_t box);
bool32_t           ui_intersect_box            (ray_t ray, bounds_t box, float *out_distance);
bool32_t           ui_interact_box             (const ui_interactor_t *interactor, bounds_t box, float *out_focus);

void               ui_interaction_1h           (ui_hash_t id, ui_interactor_event_ event_mask, vec3 unfocused_start, vec3 unfocused_size, vec3 focused_start, vec3 focused_size, button_state_ *out_focus_state, int32_t *out_interactor);
void               ui_interaction_2h           (ui_hash_t id, ui_interactor_event_ event_mask, bounds_t bounds, button_state_ *out_focus_state, int32_t *out_interactor);

bool32_t           ui_interactor_is_preoccupied(ui_interactor_id_t interactor, ui_hash_t for_el_id, bool32_t include_focused);
button_state_      ui_interactor_set_focus     (ui_interactor_id_t interactor, ui_hash_t for_el_id, bool32_t focused, float priority);
button_state_      ui_interactor_set_active    (ui_interactor_id_t interactor, ui_hash_t for_el_id, bool32_t active);
ui_interactor_id_t ui_interactor_last_active   (ui_hash_t for_el_id);
ui_interactor_id_t ui_interactor_last_focused  (ui_hash_t for_el_id);

inline bounds_t ui_size_box(vec3 top_left, vec3 dimensions) {
	return { top_left - dimensions/2, dimensions };
}

extern array_t<ui_interactor_t> skui_interactors;

}