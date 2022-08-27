#pragma once

#include "stereokit_ui.h"
#include "libraries/array.h"

namespace sk {

bool  ui_init        ();
void  ui_update      ();
void  ui_update_late ();
void  ui_shutdown    ();

id_hash_t ui_stack_hash(const char16_t* string);

// Animation
void  ui_anim_start  (id_hash_t id);
bool  ui_anim_has    (id_hash_t id, float duration);
float ui_anim_elapsed(id_hash_t id, float duration = 1, float max = 1);

// Theming
mesh_t     ui_get_mesh    (ui_vis_ element_visual);
material_t ui_get_material(ui_vis_ element_visual);
void       ui_draw_el     (ui_vis_ element_visual, vec3 start, vec3 size, ui_color_ color, float focus);

// Layout
void  ui_layout_reserve_sz(vec2 size, bool32_t add_padding, vec3* out_position, vec2* out_size);

// Base render types
void  ui_cube    (vec3 start, vec3 size, material_t material, color128 color);
void  ui_cylinder(vec3 start, float radius, float depth, material_t material, color128 color);
void  ui_text_in (vec3 start, vec2 size, const char*     text, text_align_ position, text_align_ align);
void  ui_text_in (vec3 start, vec2 size, const char16_t* text, text_align_ position, text_align_ align);

extern bool32_t      skui_show_volumes;
extern bool32_t      skui_interact_enabled;
extern material_t    skui_mat;
extern mesh_t        skui_box_dbg;
extern material_t    skui_mat_dbg;
extern ui_settings_t skui_settings;
extern sound_t       skui_snd_interact;
extern sound_t       skui_snd_uninteract;
extern color128      skui_tint;
extern float         skui_fontsize;
extern color128      skui_palette[ui_color_max];
extern color128      skui_color_border;

extern array_t<bool>      skui_preserve_keyboard_stack;
extern array_t<uint64_t> *skui_preserve_keyboard_ids_read;
extern array_t<uint64_t> *skui_preserve_keyboard_ids_write;

extern array_t<bool32_t>     skui_enabled_stack;
extern array_t<text_style_t> skui_font_stack;
}