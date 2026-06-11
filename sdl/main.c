/*
 * Castaway
 *  (C) 1994-2002 Martin Doering, Joachim Hoenig
 *  (C) 2001-2002 Petr Stehlik of ARAnyM Team
 *
 * $File$ - SDL GUI
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 * revision history
 *  19.06.2002  JH  Begin of revision history
 *  26.06.2002  JH  Fixed BIG_ENDIAN display (thx to cgraham@accessdevices.co.uk)
 *  09.07.2002  JH  Fixed display for funny video base addresses.
 *  22.08.2002  JH  Improved mips and cpu state display
 *  22.09.2002  JH  -i option: instructions per second
 */
static char     sccsid[] = "$Id: main.c,v 1.16 2002/09/22 22:08:37 jhoenig Exp $";
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

/*
 * SDL2 compatibility layer for Castaway (SDL1 -> SDL2)
 *
 * Strategy: keep the 8bpp indexed surface and all existing render logic
 * completely unchanged. At flip time, expand palette indices to ARGB32
 * in a fast C loop, then upload to a streaming ARGB texture.
 *
 * This avoids rewriting any of the inner render loops.
 */

#if SDL_MAJOR_VERSION >= 2

/* ---- type renames ---- */
typedef SDL_Keysym  SDL_keysym;
typedef SDL_Keycode SDLKey;
#define SDLK_KP0 SDLK_KP_0
#define SDLK_KP1 SDLK_KP_1
#define SDLK_KP2 SDLK_KP_2
#define SDLK_KP3 SDLK_KP_3
#define SDLK_KP4 SDLK_KP_4
#define SDLK_KP5 SDLK_KP_5
#define SDLK_KP6 SDLK_KP_6
#define SDLK_KP7 SDLK_KP_7
#define SDLK_KP8 SDLK_KP_8
#define SDLK_KP9 SDLK_KP_9

/* ---- SDL2 global state ---- */
static SDL_Window   *sdl2_win    = NULL;
static SDL_Renderer *sdl2_rend   = NULL;
static SDL_Texture  *sdl2_tex    = NULL;  /* ARGB8888, 640x400 */

/* ARGB palette: updated by SDL_SetPalette, used at flip time */
static Uint32 sdl2_argb_pal[256];  /* 256 entries, but only 0-15 used */

/* Expand-and-flip: read 8bpp indexed surface, write ARGB to texture */
static void sdl2_present(SDL_Surface *src)
{
    static Uint32 argb_buf[640 * 400];
    int x, y;
    Uint8  *sp;
    Uint32 *dp;
    int w = src->w, h = src->h;

    SDL_LockSurface(src);
    for (y = 0; y < h; y++) {
        sp = (Uint8 *)src->pixels + y * src->pitch;
        dp = argb_buf + y * w;
        for (x = 0; x < w; x++)
            dp[x] = sdl2_argb_pal[sp[x]];
    }
    SDL_UnlockSurface(src);

    SDL_UpdateTexture(sdl2_tex, NULL, argb_buf, w * sizeof(Uint32));
    SDL_RenderClear(sdl2_rend);
    SDL_RenderCopy(sdl2_rend, sdl2_tex, NULL, NULL);
    SDL_RenderPresent(sdl2_rend);
}

/* ---- SDL_SetVideoMode ---- */
static SDL_Surface *SDL1_SetVideoMode(int w, int h, int bpp, Uint32 flags)
{
    (void)bpp; (void)flags;
    sdl2_win = SDL_CreateWindow("Castaway",
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                w * 2, h * 2,
                                SDL_WINDOW_RESIZABLE);
    if (!sdl2_win) return NULL;
    sdl2_rend = SDL_CreateRenderer(sdl2_win, -1,
                                   SDL_RENDERER_ACCELERATED |
                                   SDL_RENDERER_PRESENTVSYNC);
    if (!sdl2_rend) return NULL;
    SDL_RenderSetLogicalSize(sdl2_rend, w, h);
    sdl2_tex = SDL_CreateTexture(sdl2_rend,
                                 SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING,
                                 w, h);
    if (!sdl2_tex) return NULL;

    /* Default palette: all black, index 0 = white, index 1 = black (mono) */
    {
        int pi;
        for (pi = 0; pi < 256; pi++) sdl2_argb_pal[pi] = 0xFF000000u;
        sdl2_argb_pal[0] = 0xFFFFFFFFu;  /* white */
    }

    /* 8bpp surface — same as SDL1 code expects */
    SDL_Surface *s = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
    return s;
}
#define SDL_SetVideoMode SDL1_SetVideoMode

/* ---- SDL_SetPalette: store ARGB palette for use at flip time ---- */
#define SDL_LOGPAL 1
#define SDL_PHYSPAL 2
static inline void SDL1_SetPalette(SDL_Surface *s, int flags,
                                   SDL_Color *colors, int first, int ncolors)
{
    int i;
    (void)s; (void)flags;
    for (i = 0; i < ncolors; i++) {
        SDL_Color *c = &colors[first + i];
        sdl2_argb_pal[first + i] = 0xFF000000u
            | ((Uint32)c->r << 16)
            | ((Uint32)c->g <<  8)
            | ((Uint32)c->b      );
    }
}
#define SDL_SetPalette(s,f,c,first,n) SDL1_SetPalette(s,f,c,first,n)

/* ---- SDL_Flip: expand 8bpp -> ARGB and present ---- */
static inline int SDL1_Flip(SDL_Surface *s)
{
    sdl2_present(s);
    return 0;
}
#define SDL_Flip(s) SDL1_Flip(s)

/* SDL_UpdateRect -> no-op; full present happens in SDL_Flip */
#define SDL_UpdateRect(s,x,y,w,h) ((void)0)

/* ---- SDL_WM_SetCaption ---- */
static inline void SDL1_WM_SetCaption(const char *title, const char *icon)
{
    (void)icon;
    if (sdl2_win) SDL_SetWindowTitle(sdl2_win, title);
}
#define SDL_WM_SetCaption SDL1_WM_SetCaption

/* ---- SDL_WM_GrabInput ---- */
typedef int SDL_GrabMode;
#define SDL_GRAB_QUERY  (-1)
#define SDL_GRAB_ON      1
#define SDL_GRAB_OFF     0
static inline SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode)
{
    if (!sdl2_win) return SDL_GRAB_OFF;
    if (mode == SDL_GRAB_QUERY)
        return SDL_GetWindowGrab(sdl2_win) ? SDL_GRAB_ON : SDL_GRAB_OFF;
    SDL_SetWindowGrab(sdl2_win, mode == SDL_GRAB_ON ? SDL_TRUE : SDL_FALSE);
    SDL_SetRelativeMouseMode(mode == SDL_GRAB_ON ? SDL_TRUE : SDL_FALSE);
    return mode;
}

#endif /* SDL_MAJOR_VERSION >= 2 */

#include "SDL_thread.h"
#include "SDL_main.h"

#include "68000.h"
#include "cycles68000.h"
#include "st.h"

#define min(A,B) (A)<(B)?(A):(B);
#define max(A,B) (A)>(B)?(A):(B);
#ifdef DEBUG
#include <assert.h>
#endif

SDL_Color   color[16];
SDL_Surface *screen;
SDL_TimerID timer;
SDL_Thread *thread;
#define INTERVAL            10      /* smallest system timer interval (ms) */
#define REFRESH_INTERVAL    20      /* window refresh interval (ms) */
#define TIMER_INTERVAL      5       /* timer c interval (ms) */
#define DISP_INTERVAL       100     /* caption update interval */
unsigned    ips = 0;

/*
 * 1 bit to 8 bit per pixel conversion arrays
 */
uint32 vm2bm1[256];
uint32 vm2bm2[256];
/*
 * pixel duplicator array
 */
unsigned short pixdup[256];
/*
 * clipping rectangles for video updates
 */
#define VIDMEMSIZE 32000
/* in most cases, depending on the video base address
 * the last rectangle won't be used. */
#define VIDRECTCOUNT ((VIDMEMSIZE / MEMBANKSIZE) + 2)
struct {
    int top, height;
    int modified;
    uint32 mem_begin;
    uint32 mem_end;
    uint32 vid_begin;
    uint32 vid_end;
}           update_rect[VIDRECTCOUNT];
unsigned    update_rect_count;

void VideoRamSet(uint32 address)
{
    unsigned index;
    for (index = 0; index < update_rect_count; index++) {
        if ((address & ~MEMBANKMASK) == update_rect[index].mem_begin) {
            update_rect[index].modified = 1;
            break; /* done */
        }
    }
    /* reinstall default RAM Set function. */
    MemTableSet(address & ~MEMBANKMASK, MEMBANKSIZE,
        RamSetB, RamSetW, RamSetL, RamGetB, RamGetW, RamGetL);
}

void VideoRamSetB(uint32 address, uint8 value)
{
    RamSetB(address, value);
    VideoRamSet(address);
}

void VideoRamSetW(uint32 address, uint16 value)
{
    RamSetW(address, value);
    VideoRamSet(address);
}

void VideoRamSetL(uint32 address, uint32 value)
{
    RamSetL(address, value);
    VideoRamSet(address);
}

void    Redraw(void)
{
    /*
     * get base address
     */
    uint32 VideoOffset;
    static uint32 OldOffset;
    unsigned char *VideoMemory, *ScreenMemory;
    unsigned i;
    VideoOffset = vid_baseh;
    VideoOffset <<= 8;
    VideoOffset |= vid_basem;
    VideoOffset <<= 8;
    VideoMemory = ((unsigned char *)membase) + VideoOffset;
    ScreenMemory = screen->pixels;

    if (vid_flag) {
        // Set colors if video settings have been changed.
        unsigned char fg_color = (vid_col[0] & 0x1) * 0xff;
        unsigned char bg_color = ~fg_color;
        vid_flag = 0;
        for (i = 0; i < 16; i++) {
            color[i].b = 36 * (vid_col[i] & 0x7);
            color[i].g = 36 * ((vid_col[i] >> 4) & 0x7);
            color[i].r = 36 * ((vid_col[i] >> 8) & 0x7);
        }
        if (vid_shiftmode == MONO) {
            color[0].b = fg_color;
            color[0].g = fg_color;
            color[0].r = fg_color;
            color[1].b = bg_color;
            color[1].g = bg_color;
            color[1].r = bg_color;
        }
        SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, color, 0, 16);
    }
    if (OldOffset != VideoOffset) {
        /* setup update rectangles */
        uint32 address;
        unsigned num_rects;
        OldOffset = VideoOffset;

        /*
         * clip rect top and height are set to work with color
         * modes, this setting may not be optimal for monochrome,
         * but it works. As raster lines are not aligned with
         * MEMBANKSIZE, some clip rects have to overlap by two lines.
         */
        for (address = VideoOffset & ~MEMBANKMASK, i = 0;
            address < VideoOffset + VIDMEMSIZE;
            address += MEMBANKSIZE, i++) {
#ifdef DEBUG
            assert(i < VIDRECTCOUNT);
#endif
            update_rect[i].mem_begin = address;
            update_rect[i].mem_end = address + MEMBANKSIZE;
            update_rect[i].vid_begin =
                max(update_rect[i].mem_begin, VideoOffset);
            update_rect[i].vid_end =
                min(update_rect[i].mem_end, VideoOffset + VIDMEMSIZE);
            update_rect[i].top =
                ((update_rect[i].vid_begin - VideoOffset) / 160) * 2;
            update_rect[i].height =
                ((update_rect[i].vid_end - VideoOffset) / 160) * 2
                - update_rect[i].top;
            if ((update_rect[i].vid_end - VideoOffset) % 160) {
                update_rect[i].height += 2;
            }
            if (update_rect[i].top + update_rect[i].height > 400) {
                update_rect[i].height = 400 - update_rect[i].top;
            }
            update_rect[i].modified = 1;
        }
        update_rect_count = i;
    }
    /* install Video Ram Set Handlers */
    MemTableSet(VideoOffset & ~MEMBANKMASK, MEMBANKSIZE * update_rect_count,
        VideoRamSetB, VideoRamSetW, VideoRamSetL, RamGetB, RamGetW, RamGetL);
    /* copy Video Ram to Screen */
    SDL_LockSurface(screen);
    for (i = 0; i < update_rect_count; i++) {
        register unsigned char *line_i;
        register uint32 *line_o, *line_o1, *line_o2;
        int row, col;
        if (update_rect[i].modified == 0) continue;
        switch (vid_shiftmode) {
        case MONO:
            line_i = update_rect[i].vid_begin + membase;
            line_o = (uint32 *)(ScreenMemory + 8 * (update_rect[i].vid_begin - VideoOffset));
            while (line_i < (unsigned char *)(update_rect[i].vid_end + membase)) {
                *line_o++ = vm2bm1[*line_i];
                *line_o++ = vm2bm2[*line_i++];
            }
            break;
        case COL2:
            line_i = VideoMemory + update_rect[i].top * 80;
            for (row = update_rect[i].top; row < update_rect[i].top + update_rect[i].height; row += 2) {
                line_o1 = (uint32 *)(ScreenMemory + screen->pitch * row);
                line_o2 = (uint32 *)(ScreenMemory + screen->pitch * (row + 1));
                for (col = 0; col < 80; col += 2) {
                    register uint32 val;
                    val = (vm2bm1[*line_i])
                        + (vm2bm1[*(line_i + 2)] << 1);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    val = (vm2bm2[*line_i])
                        + (vm2bm2[*(line_i + 2)] << 1);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    val = (vm2bm1[*(line_i + 1)])
                        + (vm2bm1[*(line_i + 3)] << 1);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    val = (vm2bm2[*(line_i + 1)])
                        + (vm2bm2[*(line_i + 3)] << 1);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    line_i += 4;
                }
            }
            break;
        case COL4:
            line_i = VideoMemory + update_rect[i].top * 80;
            for (row = update_rect[i].top; row < update_rect[i].top + update_rect[i].height; row += 2) {
                line_o1 = (uint32 *)(ScreenMemory + screen->pitch * row);
                line_o2 = (uint32 *)(ScreenMemory + screen->pitch * (row + 1));
                for (col = 0; col < 80; col += 4) {
                    register uint32 val;
                    unsigned short val0 = pixdup[*line_i++];
                    unsigned short val1 = pixdup[*line_i++];
                    unsigned short val2 = pixdup[*line_i++];
                    unsigned short val3 = pixdup[*line_i++];
                    unsigned short val4 = pixdup[*line_i++];
                    unsigned short val5 = pixdup[*line_i++];
                    unsigned short val6 = pixdup[*line_i++];
                    unsigned short val7 = pixdup[*line_i++];
                    val = (vm2bm1[val0 >> 8])
                        + (vm2bm1[val2 >> 8] << 1)
                        + (vm2bm1[val4 >> 8] << 2)
                        + (vm2bm1[val6 >> 8] << 3);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    val = (vm2bm2[val0 >> 8])
                        + (vm2bm2[val2 >> 8] << 1)
                        + (vm2bm2[val4 >> 8] << 2)
                        + (vm2bm2[val6 >> 8] << 3);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    val = (vm2bm1[val0 & 0xff])
                        + (vm2bm1[val2 & 0xff] << 1)
                        + (vm2bm1[val4 & 0xff] << 2)
                        + (vm2bm1[val6 & 0xff] << 3);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    val = (vm2bm2[val0 & 0xff])
                        + (vm2bm2[val2 & 0xff] << 1)
                        + (vm2bm2[val4 & 0xff] << 2)
                        + (vm2bm2[val6 & 0xff] << 3);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    val = (vm2bm1[val1 >> 8])
                        + (vm2bm1[val3 >> 8] << 1)
                        + (vm2bm1[val5 >> 8] << 2)
                        + (vm2bm1[val7 >> 8] << 3);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    val = (vm2bm2[val1 >> 8])
                        + (vm2bm2[val3 >> 8] << 1)
                        + (vm2bm2[val5 >> 8] << 2)
                        + (vm2bm2[val7 >> 8] << 3);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    val = (vm2bm1[val1 & 0xff])
                        + (vm2bm1[val3 & 0xff] << 1)
                        + (vm2bm1[val5 & 0xff] << 2)
                        + (vm2bm1[val7 & 0xff] << 3);
                    *line_o1++ = val;
                    *line_o2++ = val;
                    val = (vm2bm2[val1 & 0xff])
                        + (vm2bm2[val3 & 0xff] << 1)
                        + (vm2bm2[val5 & 0xff] << 2)
                        + (vm2bm2[val7 & 0xff] << 3);
                    *line_o1++ = val;
                    *line_o2++ = val;
                }
            }
            break;
        }
    }
    SDL_UnlockSurface(screen);
    /* update modified rectangles */
    {
        int any_modified = 0;
        for (i = 0; i < update_rect_count; i++) {
            if (update_rect[i].modified != 0) {
                update_rect[i].modified = 0;
                SDL_UpdateRect(screen, 0, update_rect[i].top, 640, update_rect[i].height);
                any_modified = 1;
            }
        }
#if SDL_MAJOR_VERSION >= 2
        if (any_modified)
            sdl2_present(screen);
#endif
    }
}

unsigned int    Timer(unsigned int interval, void *param)
{
    // 100 Hz timer interrupt
    CPUEvent();
    return interval;
}



/*********************************************************************
 * Keyboard handling
 *********************************************************************/

int     keysymToAtari(SDL_keysym keysym)
{
    static int offset = -1;     // uninitialized scancode offset

    switch(keysym.sym) {
        // Numeric Pad
    case SDLK_KP_DIVIDE:    return 0x65;    /* Numpad / */
    case SDLK_KP_MULTIPLY:  return 0x66;    /* NumPad * */
    case SDLK_KP7:      return 0x67;    /* NumPad 7 */
    case SDLK_KP8:      return 0x68;    /* NumPad 8 */
    case SDLK_KP9:      return 0x69;    /* NumPad 9 */
    case SDLK_KP_MINUS:     return 0x4a;    /* NumPad - */
    case SDLK_KP4:      return 0x6a;    /* NumPad 4 */
    case SDLK_KP5:      return 0x6b;    /* NumPad 5 */
    case SDLK_KP6:      return 0x6c;    /* NumPad 6 */
    case SDLK_KP_PLUS:      return 0x4e;    /* NumPad + */
    case SDLK_KP1:      return 0x6d;    /* NumPad 1 */
    case SDLK_KP2:      return 0x6e;    /* NumPad 2 */
    case SDLK_KP3:      return 0x6f;    /* NumPad 3 */
    case SDLK_KP0:      return 0x70;    /* NumPad 0 */
    case SDLK_KP_PERIOD:    return 0x71;    /* NumPad . */
    case SDLK_KP_ENTER:     return 0x72;    /* NumPad Enter */

    // Special Keys
    case SDLK_F11:      return 0x62;    /* F11 => Help */
    case SDLK_F12:      return 0x61;    /* F12 => Undo */
    case SDLK_HOME:     return 0x47;    /* Home */
    case SDLK_END:      return 0x60;    /* End => "<>" on German Atari kbd */
    case SDLK_UP:       return 0x48;    /* Arrow Up */
    case SDLK_LEFT:     return 0x4b;    /* Arrow Left */
    case SDLK_RIGHT:        return 0x4d;    /* Arrow Right */
    case SDLK_DOWN:     return 0x50;    /* Arrow Down */
    case SDLK_INSERT:       return 0x52;    /* Insert */
    case SDLK_DELETE:       return 0x53;    /* Delete */

    // Map Right Alt/Alt Gr/Control to the Atari keys
    case SDLK_RCTRL:        return 0x1d;    /* Control */
    case SDLK_MODE:
    case SDLK_RALT:     return 0x38;    /* Alternate */

    default:
        {
            // Process remaining keys: assume that it's PC101 keyboard
            // and that it is compatible with Atari ST keyboard (basically
            // same scancodes but on different platforms with different
            // base offset (framebuffer = 0, X11 = 8).
            // Try to detect the offset using a little bit of black magic.
            // If offset is known then simply pass the scancode.
            int scanPC = keysym.scancode;
            if (offset == -1) {
                // Heuristic analysis to find out the obscure scancode offset
                switch(keysym.sym) {
                case SDLK_ESCAPE:   offset = scanPC - 0x01; break;
                case SDLK_1:        offset = scanPC - 0x02; break;
                case SDLK_2:        offset = scanPC - 0x03; break;
                case SDLK_3:        offset = scanPC - 0x04; break;
                case SDLK_4:        offset = scanPC - 0x05; break;
                case SDLK_5:        offset = scanPC - 0x06; break;
                case SDLK_6:        offset = scanPC - 0x07; break;
                case SDLK_7:        offset = scanPC - 0x08; break;
                case SDLK_8:        offset = scanPC - 0x09; break;
                case SDLK_9:        offset = scanPC - 0x0a; break;
                case SDLK_0:        offset = scanPC - 0x0b; break;
                case SDLK_BACKSPACE:    offset = scanPC - 0x0e; break;
                case SDLK_TAB:      offset = scanPC - 0x0f; break;
                case SDLK_RETURN:   offset = scanPC - 0x1c; break;
                case SDLK_SPACE:    offset = scanPC - 0x39; break;
                case SDLK_q:        offset = scanPC - 0x10; break;
                case SDLK_w:        offset = scanPC - 0x11; break;
                case SDLK_e:        offset = scanPC - 0x12; break;
                case SDLK_r:        offset = scanPC - 0x13; break;
                case SDLK_t:        offset = scanPC - 0x14; break;
                case SDLK_y:        offset = scanPC - 0x15; break;
                case SDLK_u:        offset = scanPC - 0x16; break;
                case SDLK_i:        offset = scanPC - 0x17; break;
                case SDLK_o:        offset = scanPC - 0x18; break;
                case SDLK_p:        offset = scanPC - 0x19; break;
                case SDLK_a:        offset = scanPC - 0x1e; break;
                case SDLK_s:        offset = scanPC - 0x1f; break;
                case SDLK_d:        offset = scanPC - 0x20; break;
                case SDLK_f:        offset = scanPC - 0x21; break;
                case SDLK_g:        offset = scanPC - 0x22; break;
                case SDLK_h:        offset = scanPC - 0x23; break;
                case SDLK_j:        offset = scanPC - 0x24; break;
                case SDLK_k:        offset = scanPC - 0x25; break;
                case SDLK_l:        offset = scanPC - 0x26; break;
                case SDLK_z:        offset = scanPC - 0x2c; break;
                case SDLK_x:        offset = scanPC - 0x2d; break;
                case SDLK_c:        offset = scanPC - 0x2e; break;
                case SDLK_v:        offset = scanPC - 0x2f; break;
                case SDLK_b:        offset = scanPC - 0x30; break;
                case SDLK_n:        offset = scanPC - 0x31; break;
                case SDLK_m:        offset = scanPC - 0x32; break;
                case SDLK_CAPSLOCK: offset = scanPC - 0x3a; break;
                case SDLK_LSHIFT:   offset = scanPC - 0x2a; break;
                case SDLK_LCTRL:    offset = scanPC - 0x1d; break;
                case SDLK_LALT:     offset = scanPC - 0x38; break;
                case SDLK_F1:       offset = scanPC - 0x3b; break;
                case SDLK_F2:       offset = scanPC - 0x3c; break;
                case SDLK_F3:       offset = scanPC - 0x3d; break;
                case SDLK_F4:       offset = scanPC - 0x3e; break;
                case SDLK_F5:       offset = scanPC - 0x3f; break;
                case SDLK_F6:       offset = scanPC - 0x40; break;
                case SDLK_F7:       offset = scanPC - 0x41; break;
                case SDLK_F8:       offset = scanPC - 0x42; break;
                case SDLK_F9:       offset = scanPC - 0x43; break;
                case SDLK_F10:      offset = scanPC - 0x44; break;
                default:        break;
                }
                if (offset != -1) {
                    printf("Detected scancode offset = %d (key: '%s' with scancode $%02x)\n", offset, SDL_GetKeyName(keysym.sym), scanPC);
                }
            }

            if (offset >= 0) {
                // offset is defined so pass the scancode directly
                return scanPC - offset;
            }
            else {
                fprintf(stderr, "Unknown key: scancode = %d ($%02x), keycode = '%s' ($%02x)\n", scanPC, scanPC, SDL_GetKeyName(keysym.sym), keysym.sym);
                return 0;   // unknown scancode
            }
        }
    }
}


void    Main_EventHandler(void)
{
    SDL_Event event;
    static uint8 joystick1_state = 0;

    /* Just process events, if there */
    if( SDL_PollEvent(&event) ) {

        SDL_keysym keysym = event.key.keysym;
        SDLKey sym = keysym.sym;
        int state = keysym.mod; // SDL_GetModState();

        int shifted = state & KMOD_SHIFT;
        int controlled = state & KMOD_CTRL;
        int alternated = state & KMOD_ALT;

        switch(event.type) {
        case SDL_KEYDOWN:
            /* Eventually leave emulator */
            if( sym==SDLK_PAUSE && shifted ) {
                exit(0);
            }
            if (sym==SDLK_UP)    { joystick1_state|=0x01; IkbdJoystickChange(0,joystick1_state); IkbdJoystickChange(1,joystick1_state); break; }
            if (sym==SDLK_DOWN)  { joystick1_state|=0x02; IkbdJoystickChange(0,joystick1_state); IkbdJoystickChange(1,joystick1_state); break; }
            if (sym==SDLK_LEFT)  { joystick1_state|=0x04; IkbdJoystickChange(0,joystick1_state); IkbdJoystickChange(1,joystick1_state); break; }
            if (sym==SDLK_RIGHT) { joystick1_state|=0x08; IkbdJoystickChange(0,joystick1_state); IkbdJoystickChange(1,joystick1_state); break; }
            if (sym==SDLK_LCTRL) { joystick1_state|=0x80; IkbdJoystickChange(0,joystick1_state); IkbdJoystickChange(1,joystick1_state); break; }
            IkbdKeyPress(keysymToAtari(keysym));
            break;
        case SDL_KEYUP:
            if (sym==SDLK_UP)    { joystick1_state&=~0x01; IkbdJoystickChange(0,joystick1_state); IkbdJoystickChange(1,joystick1_state); break; }
            if (sym==SDLK_DOWN)  { joystick1_state&=~0x02; IkbdJoystickChange(0,joystick1_state); IkbdJoystickChange(1,joystick1_state); break; }
            if (sym==SDLK_LEFT)  { joystick1_state&=~0x04; IkbdJoystickChange(0,joystick1_state); IkbdJoystickChange(1,joystick1_state); break; }
            if (sym==SDLK_RIGHT) { joystick1_state&=~0x08; IkbdJoystickChange(0,joystick1_state); IkbdJoystickChange(1,joystick1_state); break; }
            if (sym==SDLK_LCTRL) { joystick1_state&=~0x80; IkbdJoystickChange(0,joystick1_state); IkbdJoystickChange(1,joystick1_state); break; }
            IkbdKeyRelease(keysymToAtari(keysym)|0x80);
            break;
        case SDL_MOUSEMOTION:
            IkbdMouseMotion(event.motion.x, event.motion.y);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
              IkbdMousePress(2);
            }
            if (event.button.button == SDL_BUTTON_RIGHT) {
                IkbdMousePress(1);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                IkbdMouseRelease(2);
            }
            if (event.button.button == SDL_BUTTON_RIGHT) {
                IkbdMouseRelease(1);
            }
            if (event.button.button == SDL_BUTTON_MIDDLE) {
                int grab_state = SDL_WM_GrabInput(SDL_GRAB_QUERY);
                if (grab_state == SDL_GRAB_ON) {
                    SDL_WM_GrabInput(SDL_GRAB_OFF);
                } else {
                    SDL_WM_GrabInput(SDL_GRAB_ON);
                }
            }
            break;
        case SDL_QUIT:
            exit(0);
        }
    }
    CPUEvent();
}

int     main(int argc, char *argv[])
{
    int i;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);
    Init(argc, argv);
    screen = SDL_SetVideoMode(640, 400, 8, 0);
    SDL_ShowCursor(SDL_DISABLE);
    if (screen == NULL) {
        fprintf(stderr, "SDL_SetVideoMode(): %s\n", SDL_GetError());
        exit(1);
    }
    fprintf(stderr, "Got SDL screen flags %08lx\n", (unsigned long)screen->flags);
    // initialize 1bpp to 8bpp conversion arrays
    for (i = 0; i < 256; i++) {
#ifdef LITTLE_ENDIAN
        vm2bm1[i] =
            ((i & 0x10) ? 0x01000000L : 0)
          | ((i & 0x20) ? 0x00010000L : 0)
          | ((i & 0x40) ? 0x00000100L : 0)
          | ((i & 0x80) ? 0x00000001L : 0);
        vm2bm2[i] =
            ((i & 0x01) ? 0x01000000L : 0)
          | ((i & 0x02) ? 0x00010000L : 0)
          | ((i & 0x04) ? 0x00000100L : 0)
          | ((i & 0x08) ? 0x00000001L : 0);
#else
        vm2bm1[i] =
            ((i & 0x80) ? 0x01000000L : 0)
          | ((i & 0x40) ? 0x00010000L : 0)
          | ((i & 0x20) ? 0x00000100L : 0)
          | ((i & 0x10) ? 0x00000001L : 0);
        vm2bm2[i] =
            ((i & 0x08) ? 0x01000000L : 0)
          | ((i & 0x04) ? 0x00010000L : 0)
          | ((i & 0x02) ? 0x00000100L : 0)
          | ((i & 0x01) ? 0x00000001L : 0);
#endif
    }
    // initialize pixel duplicator array
    for (i = 0; i < 256; i++) {
        pixdup[i] =
            ((i & 0x80) ? 0xc000 : 0)
          | ((i & 0x40) ? 0x3000 : 0)
          | ((i & 0x20) ? 0x0c00 : 0)
          | ((i & 0x10) ? 0x0300 : 0)
          | ((i & 0x08) ? 0x00c0 : 0)
          | ((i & 0x04) ? 0x0030 : 0)
          | ((i & 0x02) ? 0x000c : 0)
          | ((i & 0x01) ? 0x0003 : 0);
    }
    timer = SDL_AddTimer(INTERVAL, Timer, NULL);
    //thread = SDL_CreateThread(Thread, NULL);
    /* MAD: Switch on processing of different event types */
    SDL_EventState(SDL_MOUSEMOTION,     SDL_ENABLE);
    SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
    SDL_EventState(SDL_MOUSEBUTTONUP,   SDL_ENABLE);
    SDL_EventState(SDL_KEYDOWN,         SDL_ENABLE);
    SDL_EventState(SDL_KEYUP,           SDL_ENABLE);
    SDL_EventState(SDL_QUIT,            SDL_ENABLE);

    {
        /*
         * Cycle-accurate timing for the Atari ST.
         * The 68000 runs at 8 MHz = 8,000,000 cycles/sec.
         * VBlank at 50 Hz     = every 160,000 cycles
         * IOTimer at 200 Hz   = every  40,000 cycles
         * Run ~8000 cycles per burst (1ms of real ST time)
         */
#define ST_CLOCK_HZ     8000000UL
#define CYCLES_PER_VBLANK   (ST_CLOCK_HZ / 50)     /* 160000 */
#define CYCLES_PER_IOTIMER  (ST_CLOCK_HZ / 200)    /*  40000 */
#define CYCLES_PER_BURST    8000                    /* ~1ms burst */

        unsigned long   vblank_next  = cpu_cycles + CYCLES_PER_VBLANK;
        unsigned long   iotimer_next = cpu_cycles + CYCLES_PER_IOTIMER;
        unsigned long   emu_ms = 0;
        unsigned long   wall_ms_base = SDL_GetTicks();
        unsigned        disp_tics    = SDL_GetTicks();
        unsigned        disp_interval_start = disp_tics;
        double          mips         = 0.0;
        unsigned        disp_count   = 0;

        while (1) {
            unsigned tics, count;

            /* Run a burst of CPU cycles */
            if (IkbdQueryBuffer()) {
                count = CPURun(500);
            } else {
                count = CPURun(CYCLES_PER_BURST / 10); /* ~800 instructions */
            }
            disp_count += count;

            if (count == 0) {
                SDL_Delay(1);
            }

            /* Cycle-based IOTimer (200 Hz) */
            while (cpu_cycles >= iotimer_next) {
                iotimer_next += CYCLES_PER_IOTIMER;
                IOTimer();
                Main_EventHandler();
            }

            /* Cycle-based VBlank (50 Hz) */
            if (cpu_cycles >= vblank_next) {
                vblank_next += CYCLES_PER_VBLANK;
                Redraw();
                VBlank();
            }

            /* Caption update (wall clock, ~10 Hz) */
            /* Real-time throttle */
            emu_ms = cpu_cycles / (ST_CLOCK_HZ / 1000);
            {
                unsigned long wall_ms = SDL_GetTicks() - wall_ms_base;
                if (emu_ms > wall_ms + 1)
                    SDL_Delay(emu_ms - wall_ms - 1);
            }

            tics = SDL_GetTicks();
            if (tics > disp_tics + DISP_INTERVAL) {
                /* Calculate how many millions of instructions are
                 * executed per second, and update caption accordingly.
                 * Also display the CPU state. */
                char animated_run[] = " .";
                char buffer[80];
                double interval_us = (tics - disp_interval_start) * 1000.0;
                double last_mips = ((double) disp_count) / interval_us;
                disp_interval_start = tics;
                disp_tics += DISP_INTERVAL;
                disp_count = 0;
                mips = (mips * 0.9) + (last_mips * 0.1);
                sprintf(buffer, "Castaway @ %0.1fmips ", mips);
                switch (cpu_state) {
                case -3:
                    strcat(buffer, "(crashed)");
                    break;
                case -2:
                    strcat(buffer, "(stopped)");
                    break;
                default:
                    *(buffer + strlen(buffer) + 1) = 0;
                    *(buffer + strlen(buffer)) = animated_run[(tics / 1000) % 2];
                    break;
                }
                SDL_WM_SetCaption(buffer, NULL);
            }
            /* Transfer next byte from IKBD output buffer into ACIA */
            IkbdWriteBuffer();
            /* Set interrupt request priority */
            SetInterruptPriority();
        }
    }
}



