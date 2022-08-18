#include "../stereokit_ui.h"

namespace sk {

typedef struct ui_el_visual_t {
	ui_vis_    fallback;
	mesh_t     mesh;
	vec2       min_size;
	material_t material;
	color128   color_linear;
	bool32_t   has_color;
} ui_el_visual_t;

typedef struct _ui_theme_t {
	int            refcount;
	ui_el_visual_t visuals[ui_vis_max];
} _ui_theme_t;

}