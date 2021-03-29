#include <utils/logger.h>
#include <utils/function_patcher.h>
#include <dynamic_libs/vpad_functions.h>
#include "common/retain_vars.h"
#include "hooks_patcher.h"
#include "myutils/overlay_helper.h"
#include <malloc.h>
#include "main.h"
#include "utils.h"
#include "mymemory/memory_mapping.h"
#include "myutils/mem_utils.h"
#include "myutils/texture_utils.h"
#include <gx2/clear.h>

DECL(void, __PPCExit, void) {
    // Only continue if we are in the "right" application.
    //if(OSGetTitleID() == gGameTitleID) {
    //DEBUG_FUNCTION_LINE("__PPCExit\n");
    //CallHook(WUPS_LOADER_HOOK_ENDING_APPLICATION);
    //DeInit();
    //}

    real___PPCExit();
}

DECL_FUNCTION(uint32_t, __OSPhysicalToEffectiveCached, uint32_t phyiscalAddress) {
    uint32_t result = real___OSPhysicalToEffectiveCached(phyiscalAddress);
    if(result == 0) {
        result = MemoryMapping::PhysicalToEffective(phyiscalAddress);
        //DEBUG_FUNCTION_LINE("__OSPhysicalToEffectiveCached in %08X out %08X\n",phyiscalAddress,result);
    }
    return result;
}

DECL_FUNCTION(uint32_t, __OSPhysicalToEffectiveUncached, uint32_t phyiscalAddress) {
    uint32_t result = real___OSPhysicalToEffectiveUncached(phyiscalAddress);
    if(result == 0) {
        result = MemoryMapping::PhysicalToEffective(phyiscalAddress);
        //DEBUG_FUNCTION_LINE("__OSPhysicalToEffectiveUncached in %08X out %08X\n",phyiscalAddress,result);
        return result;
    }
    return result;
}


DECL_FUNCTION(uint32_t, OSEffectiveToPhysical, uint32_t virtualAddress) {
    uint32_t result = real_OSEffectiveToPhysical(virtualAddress);
    if(result == 0) {
        result = MemoryMapping::EffectiveToPhysical(virtualAddress);
        //DEBUG_FUNCTION_LINE("OSEffectiveToPhysical in %08X out %08X\n",virtualAddress,result);
        return result;
    }
    return result;
}

DECL_FUNCTION(int32_t, OSIsAddressValid, uint32_t virtualAddress) {
    int32_t result = real_OSIsAddressValid(virtualAddress);
    if(result == 0) {
        result = (MemoryMapping::EffectiveToPhysical(virtualAddress) > 0);
        //DEBUG_FUNCTION_LINE("OSIsAddressValid in %08X out %d\n",virtualAddress,result);
        return result;
    }
    return result;
}

DECL(void, GX2SetTVBuffer, void *buffer, uint32_t buffer_size, int32_t tv_render_mode, int32_t format, int32_t buffering_mode) {
    tv_store.buffer = buffer;
    tv_store.buffer_size = buffer_size;
    tv_store.mode = tv_render_mode;
    tv_store.surface_format = format;
    tv_store.buffering_mode = buffering_mode;

    return real_GX2SetTVBuffer(buffer,buffer_size,tv_render_mode,format,buffering_mode);
}

DECL(void, GX2SetDRCBuffer, void *buffer, uint32_t buffer_size, int32_t drc_mode, int32_t surface_format, int32_t buffering_mode) {
    drc_store.buffer = buffer;
    drc_store.buffer_size = buffer_size;
    drc_store.mode = drc_mode;
    drc_store.surface_format = surface_format;
    drc_store.buffering_mode = buffering_mode;

    return real_GX2SetDRCBuffer(buffer,buffer_size,drc_mode,surface_format,buffering_mode);
}

DECL(void, GX2WaitForVsync, void) {
    CallHook(WUPS_LOADER_HOOK_VSYNC);
    real_GX2WaitForVsync();
}

uint8_t vpadPressCooldown = 0xFF;

/*uint8_t angleX_counter = 0;
float angleX_delta = 0.0f;
float angleX_last = 0.0f;
uint8_t angleX_frameCounter = 0;*/

DECL(int32_t, VPADRead, int32_t chan, VPADData *buffer, uint32_t buffer_size, int32_t *error) {
    int32_t result = real_VPADRead(chan, buffer, buffer_size, error);

    if(result > 0 && (buffer[0].btns_h == (VPAD_BUTTON_PLUS | VPAD_BUTTON_R | VPAD_BUTTON_L)) && vpadPressCooldown == 0 && OSIsHomeButtonMenuEnabled()) {
        if(MemoryMapping::isMemoryMapped()) {
            MemoryMapping::readTestValuesFromMemory();
        } else {
            DEBUG_FUNCTION_LINE("Memory was not mapped. To test the memory please exit the plugin loader by pressing MINUS\n");
        }
        vpadPressCooldown = 0x3C;
    }

    if(vpadPressCooldown > 0) {
        vpadPressCooldown--;
    }
    return result;
}

void setupContextState() {
    g_vid_ownContextState = (GX2ContextState*)memalign(
                                GX2_CONTEXT_STATE_ALIGNMENT,
                                sizeof(GX2ContextState)
                            );
    if(g_vid_ownContextState == NULL) {
        OSFatal("VideoSquoosher: Failed to alloc g_vid_ownContextState\n");
    }
    GX2SetupContextStateEx(g_vid_ownContextState, 1);

    GX2SetContextState(g_vid_ownContextState);
    GX2SetColorBuffer(&g_vid_main_cbuf, GX2_RENDER_TARGET_0);
    //GX2SetDepthBuffer(&tvDepthBuffer);
    GX2SetContextState(g_vid_originalContextSave);
    DEBUG_FUNCTION_LINE("Setup contest state done\n");
}

void initTextures() {
    GX2InitColorBuffer(&g_vid_main_cbuf,
                       GX2_SURFACE_DIM_TEXTURE_2D,
                       1280, 720, 1,
                       GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8,
                       GX2_AA_MODE1X
                      );

    if (g_vid_main_cbuf.surface.imageSize) {
        g_vid_main_cbuf.surface.image = MemoryUtils::alloc(
                g_vid_main_cbuf.surface.imageSize,
                g_vid_main_cbuf.surface.alignment
                                             );
        if(g_vid_main_cbuf.surface.image == NULL) {
            OSFatal("Failed to alloc g_vid_main_cbuf\n");
        }
        DEBUG_FUNCTION_LINE("Allocated %dx%d g_vid_main_cbuf %08X\n",
                            g_vid_main_cbuf.surface.width,
                            g_vid_main_cbuf.surface.height,
                            g_vid_main_cbuf.surface.image);
    } else {
        DEBUG_FUNCTION_LINE("GX2InitTexture failed for g_vid_main_cbuf!\n");
    }

    GX2InitTexture(&g_vid_drcTex,
                   854, 480, 1, 0,
                   GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8,
                   GX2_SURFACE_DIM_TEXTURE_2D,
                   GX2_TILE_MODE_LINEAR_ALIGNED
                  );
    g_vid_drcTex.surface.use = (GX2SurfaceUse)(GX2_SURFACE_USE_COLOR_BUFFER | GX2_SURFACE_USE_TEXTURE);

    if (g_vid_drcTex.surface.imageSize) {

        g_vid_drcTex.surface.image = MemoryUtils::alloc(
                                              g_vid_drcTex.surface.imageSize,
                                              g_vid_drcTex.surface.alignment);

        if(g_vid_drcTex.surface.image == NULL) {
            OSFatal("VideoSquoosher: Failed to alloc g_vid_drcTex\n");
        }
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU, g_vid_drcTex.surface.image, g_vid_drcTex.surface.imageSize);
        DEBUG_FUNCTION_LINE("VideoSquoosher: allocated %dx%d g_vid_drcTex %08X\n",
                            g_vid_drcTex.surface.width,
                            g_vid_drcTex.surface.height,
                            g_vid_drcTex.surface.image);

    } else {
        DEBUG_FUNCTION_LINE("VideoSquoosher: GX2InitTexture failed for g_vid_drcTex!\n");
    }

    GX2InitTexture(&g_vid_tvTex,
                   1280, 720, 1, 0,
                   GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8,
                   GX2_SURFACE_DIM_TEXTURE_2D,
                   GX2_TILE_MODE_LINEAR_ALIGNED
                  );
    g_vid_tvTex.surface.use =
        (GX2SurfaceUse)(GX2_SURFACE_USE_COLOR_BUFFER | GX2_SURFACE_USE_TEXTURE);

    DCFlushRange(&g_vid_tvTex, sizeof(GX2Texture));

    if (g_vid_tvTex.surface.imageSize) {
        g_vid_tvTex.surface.image = MemoryUtils::alloc(
                                             g_vid_tvTex.surface.imageSize,
                                             g_vid_tvTex.surface.alignment
                                         );
        if(g_vid_tvTex.surface.image == NULL) {
            OSFatal("VideoSquoosher: Failed to alloc g_vid_tvTex\n");
        }
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU, g_vid_tvTex.surface.image, g_vid_tvTex.surface.imageSize);
        DEBUG_FUNCTION_LINE("VideoSquoosher: allocated %dx%d g_vid_tvTex %08X\n",
                            g_vid_tvTex.surface.width,
                            g_vid_tvTex.surface.height,
                            g_vid_tvTex.surface.image);
    } else {
        DEBUG_FUNCTION_LINE("VideoSquoosher: GX2InitTexture failed for g_vid_tvTex!\n");
    }

    GX2InitSampler(&g_vid_sampler,
                   GX2_TEX_CLAMP_MODE_CLAMP,
                   GX2_TEX_XY_FILTER_MODE_LINEAR
                  );
}

DECL_FUNCTION(void, GX2SetContextState, GX2ContextState * curContext) {
    if(gAppStatus == WUPS_APP_STATUS_FOREGROUND) {
        g_vid_originalContextSave = curContext;
    }
    real_GX2SetContextState(curContext);
}

DECL_FUNCTION(void, GX2CopyColorBufferToScanBuffer, GX2ColorBuffer* cbuf, int32_t target) {
    bool hasDRCHook = HasHookCallHook(WUPS_LOADER_HOOK_VID_DRC_DRAW);
    bool hasTVHook = HasHookCallHook(WUPS_LOADER_HOOK_VID_TV_DRAW);
    if(gAppStatus != WUPS_APP_STATUS_FOREGROUND || !g_NotInLoader || (!hasDRCHook && !hasTVHook)) {
        return real_GX2CopyColorBufferToScanBuffer(cbuf,target);
    }

    if (!g_vid_drcTex.surface.image) {
        initTextures();
    }

    if(g_vid_ownContextState == NULL) {
        setupContextState();
    }

    if(target == 1) {
        TextureUtils::copyToTexture(cbuf,&g_vid_tvTex);
        if(!hasTVHook) {
            return real_GX2CopyColorBufferToScanBuffer(cbuf,target);
        }
    } else if(target == 4) {
        TextureUtils::copyToTexture(cbuf,&g_vid_drcTex);
        if(!hasDRCHook) {
            return real_GX2CopyColorBufferToScanBuffer(cbuf,target);
        }
    }

    GX2SetContextState(g_vid_ownContextState);
    GX2ClearColor(&g_vid_main_cbuf, 1.0f, 1.0f, 1.0f, 1.0f);
    GX2SetContextState(g_vid_ownContextState);

    GX2SetViewport(
        0.0f, 0.0f,
        g_vid_main_cbuf.surface.width, g_vid_main_cbuf.surface.height,
        0.0f, 1.0f
    );
    GX2SetScissor(
        0, 0,
        g_vid_main_cbuf.surface.width, g_vid_main_cbuf.surface.height
    );

    if(target == 1) {
        //drawTexture(&g_vid_tvTex, &g_vid_sampler, 0, 0, 1280, 720, 1.0f);
        CallHook(WUPS_LOADER_HOOK_VID_TV_DRAW);
    } else if(target == 4) {
        //drawTexture(&g_vid_drcTex, &g_vid_sampler, 0, 0, 1280, 720, 1.0f);
        CallHook(WUPS_LOADER_HOOK_VID_DRC_DRAW);
    }

    GX2SetContextState(g_vid_originalContextSave);

    return real_GX2CopyColorBufferToScanBuffer(&g_vid_main_cbuf,target);
}


hooks_magic_t method_hooks_hooks_static[] __attribute__((section(".data"))) = {
    MAKE_MAGIC(__PPCExit,                       LIB_CORE_INIT,  STATIC_FUNCTION),
    MAKE_MAGIC(GX2SetTVBuffer,                  LIB_GX2,        STATIC_FUNCTION),
    MAKE_MAGIC(GX2SetDRCBuffer,                 LIB_GX2,        STATIC_FUNCTION),
    MAKE_MAGIC(GX2WaitForVsync,                 LIB_GX2,        STATIC_FUNCTION),
    MAKE_MAGIC(GX2CopyColorBufferToScanBuffer,  LIB_GX2,        STATIC_FUNCTION),
    MAKE_MAGIC(GX2SetContextState,              LIB_GX2,        STATIC_FUNCTION),
    MAKE_MAGIC(VPADRead,                        LIB_VPAD,       STATIC_FUNCTION),
    MAKE_MAGIC(OSIsAddressValid,                LIB_CORE_INIT,  STATIC_FUNCTION),
    MAKE_MAGIC(__OSPhysicalToEffectiveUncached, LIB_CORE_INIT,  STATIC_FUNCTION),
    MAKE_MAGIC(__OSPhysicalToEffectiveCached,   LIB_CORE_INIT,  STATIC_FUNCTION),
    MAKE_MAGIC(OSEffectiveToPhysical,           LIB_CORE_INIT,  STATIC_FUNCTION),
};

uint32_t method_hooks_size_hooks_static __attribute__((section(".data"))) = sizeof(method_hooks_hooks_static) / sizeof(hooks_magic_t);

//! buffer to store our instructions needed for our replacements
volatile uint32_t method_calls_hooks_static[sizeof(method_hooks_hooks_static) / sizeof(hooks_magic_t) * FUNCTION_PATCHER_METHOD_STORE_SIZE] __attribute__((section(".data")));

