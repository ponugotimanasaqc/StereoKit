#pragma once

#include "stereokit_ui.h"
#include "libraries/array.h"

namespace sk {

bool ui_init       ();
void ui_update     ();
void ui_update_late();
void ui_shutdown   ();

extern bool32_t   skui_show_volumes;
extern bool32_t   skui_interact_enabled;
extern mesh_t     skui_box_dbg;
extern material_t skui_mat_dbg;

extern array_t<bool>     skui_preserve_keyboard_stack;
extern array_t<uint64_t> skui_preserve_keyboard_ids;

}