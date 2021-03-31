#ifndef _OWN_UTILS_H_
#define _OWN_UTILS_H_

/* Main */
#ifdef __cplusplus
extern "C" {
#endif

#include <wups.h>
#include <stddef.h>

bool HasHookCallHook(wups_loader_hook_type_t hook_type);
void CallHook(wups_loader_hook_type_t hook_type);

void CallHookEx(wups_loader_hook_type_t hook_type, int32_t plugin_index_needed);

bool isInMiiMakerHBL();

#ifdef __cplusplus
}
#endif

#endif
