#include <vector>

// ------------------------------------------------------------------
// STD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <tuple>
#include <cstdlib>

using namespace std;
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// WII
#include <gccore.h>
#include <ogcsys.h>

#include <ogc/tpl.h>
#include <asndlib.h>
#include "library/oggplayer.h"
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// OTHER
#include "library/FastNoiseLite.h"
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// INCLUDE DATA FILES HERE
#include "textures_tpl.h"
#include "textures.h"
/* #include "town_ogg.h" */
#include "fairy_path_ogg.h"
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// CONSTANTS / IMPORTANT VARIABLES FOR BOILERPLATE
#include "image_info.h"
#include "buttons.h"
#include "names.h"

#define DEFAULT_FIFO_SIZE	(256*1024)
#define USE_CONSOLE false
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

static void *xfb = NULL;
static void *frameBuffer[2] = { NULL, NULL};
static GXRModeObj *rmode;
GXTexObj texObj;
// ------------------------------------------------------------------

// ------------------------------------------------------------------
// USER DEFINED HEADERS/LOGIC HERE
#include "classes.h"

// classes we want available in logic.h
Camera camera;

#include "logic.h"
// ------------------------------------------------------------------

int main( int argc, char **argv ) {
    // ------------------------------------------------------------------
    // IN CASE WE NEED DEBUGGING
    // console() has a while loop, so if this function is called,
    // everything below this line won't run.
    if(USE_CONSOLE) console();
    // ------------------------------------------------------------------

    u32	fb; 	// initial framebuffer index
    u32 first_frame;
    f32 yscale;
    u32 xfbHeight;
    Mtx44 perspective;
    Mtx GXmodelView2D;
    void *gp_fifo = NULL;

    GXColor background = {0, 0, 0, 0xff};

    VIDEO_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    fb = 0;
    first_frame = 1;
    // allocate 2 framebuffers for double buffering
    frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer[fb]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    fb ^= 1;

    // setup the fifo and then init the flipper
    gp_fifo = memalign(32,DEFAULT_FIFO_SIZE);
    memset(gp_fifo,0,DEFAULT_FIFO_SIZE);

    GX_Init(gp_fifo,DEFAULT_FIFO_SIZE);

    // clears the bg to color and clears the z buffer
    GX_SetCopyClear(background, 0x00ffffff);

    // other gx setup
    GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
    yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
    xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
    GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
    GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

    if (rmode->aa)
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    else
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);


    GX_SetCullMode(GX_CULL_NONE);
    GX_CopyDisp(frameBuffer[fb],GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    // setup the vertex descriptor
    // tells the flipper to expect direct data
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);


    GX_SetNumChans(1);
    GX_SetNumTexGens(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

    GX_InvalidateTexAll();

    TPLFile spriteTPL;
    TPL_OpenTPLFromMemory(&spriteTPL, (void *)textures_tpl,textures_tpl_size);
    TPL_GetTexture(&spriteTPL,spritesheet,&texObj);
    GX_LoadTexObj(&texObj, GX_TEXMAP0);

    guOrtho(perspective,0,SCREEN_HEIGHT,0,SCREEN_WIDTH,0,320);
    GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

    PAD_Init();

    srand(time(NULL));

    // Initialise the audio subsystem
    ASND_Init();
    /* PlayOgg(town_ogg, town_ogg_size, 0, OGG_INFINITE_TIME); */
    PlayOgg(fairy_path_ogg, fairy_path_ogg_size, 1, OGG_INFINITE_TIME);


	setup();

    while(true) {

        // ------------------------------------------------------------------
        // Camera
        guOrtho(perspective,camera.y,camera.y + SCREEN_HEIGHT, camera.x,camera.x + SCREEN_WIDTH, 0, 320);
        GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);
        camera.follow_smooth(player.getX(), player.getY());
        // ------------------------------------------------------------------

        GX_InvVtxCache();
        GX_InvalidateTexAll();

        GX_ClearVtxDesc();
        GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
        GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

        guMtxIdentity(GXmodelView2D);
        guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -5.0F);
        GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);

        // ------------------------------------------------------------------
        // GAME LOGIC AND DRAW LOOP
        game_loop();
        draw_loop();
        // ------------------------------------------------------------------
                
        int x_pos = player.getX() - (SCREEN_WIDTH - 64) / 2;
        int y_pos = player.getY() - (SCREEN_HEIGHT - 64) / 2;
        guOrtho(perspective,y_pos,SCREEN_HEIGHT + y_pos, x_pos,SCREEN_WIDTH + x_pos,0,320);
        GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

        GX_DrawDone();

        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
        GX_SetAlphaUpdate(GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(frameBuffer[fb],GX_TRUE);

        VIDEO_SetNextFramebuffer(frameBuffer[fb]);
        if(first_frame) {
            VIDEO_SetBlack(FALSE);
            first_frame = 0;
        }
        VIDEO_Flush();
        VIDEO_WaitVSync();
        fb ^= 1;		// flip framebuffer
    }

	/* Stop music currently playing */
    StopOgg();

	/* Deallocate objects */
	delete(&entities);
	delete(&player);
	delete(&area);

    return 0;
}
