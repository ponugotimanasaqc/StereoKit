#pragma once
#include "platform_utils.h"

#if defined(SK_OS_WINDOWS)

namespace sk {

bool win32_init           ();
bool win32_start_pre_xr   ();
bool win32_start_post_xr  ();
bool win32_start_flat     ();
void win32_step_begin_xr  ();
void win32_step_begin_flat();
void win32_step_end_flat  ();
void win32_stop_flat      ();
void win32_shutdown       ();

void *win32_hwnd();
float win32_get_scroll();

} // namespace sk

#endif // defined(SK_OS_WINDOWS)