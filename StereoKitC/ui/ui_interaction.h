#include "../stereokit_ui.h"
#include "../_stereokit_ui.h"
#include "../libraries/array.h"

///////////////////////////////////////////

namespace sk {

enum interactor_type_ {
	interactor_type_point,
	interactor_type_ray,
};

enum interactor_event_ {
	interactor_event_poke  = 1 << 1,
	interactor_event_grip  = 1 << 2,
	interactor_event_pinch = 1 << 3
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

struct interactor_t {
	// What type of interactions does this provide
	interactor_type_  type;
	interactor_event_ events;
	
	// What is the current state of the interaction
	button_state_        state;
	button_state_        tracked;
	float                activation;
	
	float                focused_priority;
	id_hash_t            focused_prev;
	id_hash_t            focused;
	id_hash_t            active_prev;
	id_hash_t            active;
	
	// Information about the location and shape of the interactor
	float                radius;
	float                rot_smoothing;
	float                pos_smoothing;
	bool32_t             show_ray;
	float                ray_minimum_dist;
	float                head_minimum_dist;

	float                _ray_visibility;
	
	pose_t               motion_pose_world;
	pose_t               _motion_pose_local;
	pose_t               _motion_pose_world_action;
	
	pose_t               hit_test_world;
	pose_t               _hit_test_world_prev;
	pose_t               _hit_test_local;
	pose_t               _hit_test_local_prev;
	pose_t               _hit_test_world_action;
	vec3                 _hit_test_local_dir;
	vec3                 _hit_test_world_dir;
	
	vec3                 _hit_at_world;
	vec3                 _hit_at_action_local;
};

typedef int32_t interactor_id_t;


bool32_t        interactor_check_box     (const interactor_t* actor, bounds_t box, vec3* out_at, float* out_priority);

void            interactor_update_local  (matrix to_local);

void            interactor_volume_1h     (id_hash_t id, interactor_event_ event_mask, vec3 box_unfocused_start, vec3 box_unfocused_size, vec3 box_focused_start, vec3 box_focused_size, button_state_* out_focus_state, int32_t* out_interactor, vec3* out_interaction_at_local);
void            interactor_volume_2h     (id_hash_t id, interactor_event_ event_mask, bounds_t bounds, ui_2h_state_* out_focus_state, int32_t* out_interactor1, int32_t* out_interactor2);

bool32_t        interactor_is_preoccupied(interactor_id_t interactor, id_hash_t for_el_id, bool32_t include_focused);
button_state_   interactor_set_focus     (interactor_id_t interactor, id_hash_t for_el_id, bool32_t focused, float priority);
button_state_   interactor_set_active    (interactor_id_t interactor, id_hash_t for_el_id, bool32_t active, vec3 at);
interactor_id_t interactor_last_active   (id_hash_t for_el_id);
interactor_id_t interactor_last_focused  (id_hash_t for_el_id);

extern array_t<interactor_t> skui_interactors;

}