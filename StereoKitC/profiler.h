#pragma once

#if defined(TRACY_ENABLE)

#pragma comment(lib,"TracyClient.lib")
#include <tracy/tracyC.h>

#define PROFILE_START() TracyCZone(_tracy_ctx, true)
#define PROFILE_START_NAME(name) TracyCZoneN(_tracy_ctx, name, true)
#define PROFILE_START_NAME_COLOR(name, color) TracyCZoneNC(_tracy_ctx, name, color, true)
#define PROFILE_END() TracyCZoneEnd(_tracy_ctx)
#define PROFILE_FRAME() TracyCFrameMark
#define PROFILE_THREAD_NAME(name) TracyCSetThreadName(name)
#define PROFILE_ALLOC(ptr, size) TracyCAllocS(ptr, size, 12)
#define PROFILE_FREE(ptr) TracyCFreeS(ptr, 12)

#else

#define PROFILE_START()
#define PROFILE_START_NAME(name)
#define PROFILE_START_NAME_COLOR(name, color)
#define PROFILE_END()
#define PROFILE_FRAME()
#define PROFILE_THREAD_NAME(name)
#define PROFILE_ALLOC(ptr, size)
#define PROFILE_FREE(ptr)

#endif