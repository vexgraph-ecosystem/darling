#include "darling/frame.h"

#include <stdlib.h>
#include <unistd.h>

#include "image.h"   // graphvex R3
#include "vulkan/vulkan_backend.h"   // graphvex R3: the GPU backend

// darling R4 — frame.c
// The JFrame: a hotcwap window + owned child Panels, software-rendered and
// presented until the GPU seam takes over.

struct Frame {
    Window *window;
    Color background;
    Panel **panels;
    int count, cap;
    Image *shot;         // reused capture buffer
    DisplayList *dl;     // reused paint list
    int lastW, lastH;    // current target size — the single size authority
};

// The ONE resize surface. The window's resize event calls only this; it resizes
// everything: the render target, the layout canvas, and repaints. The canvas
// grows with the window but never below its minimum, so growing reflows the
// panels while shrinking holds them (they go out of bounds and clip).
static void frame_on_resize(void *userdata) {
    Frame *f = (Frame *)userdata;
    Frame_setSize(f, Window_width(f->window), Window_height(f->window));
}

static Frame *s_active = NULL;   // the most recently created Frame (CAPTURE)

Frame *Frame_2(const char *title, int widthPx, int heightPx) {
    // GPU rendering: the Vulkan backend draws the panels; capture reads them back.
    Graphics_register(VulkanBackend_row());
    Graphics_use(BACKEND_VULKAN);

    Frame *f = calloc(1, sizeof *f);
    if (!f) return NULL;
    f->window = Window_create(title ? title : "darling",
                              widthPx > 0 ? widthPx : 800,
                              heightPx > 0 ? heightPx : 600);
    if (!f->window) {
        free(f);
        return NULL;
    }
    f->background = COLOR_RGBA(18, 20, 28, 255);
    f->shot = Image_0();
    f->dl = DisplayList_0();
    f->lastW = f->lastH = 0;
    // ONE resize surface: the window reports a geometry step, we call Frame_setSize.
    Window_setResizeRenderHook(f->window, frame_on_resize, f);
    s_active = f;
    return f;
}

void Frame_destroy(Frame *frame) {
    if (!frame) return;
    Frame_removePanels(frame);
    free(frame->panels);
    Image_destroy(frame->shot);
    DisplayList_free(frame->dl);
    if (frame->window) Window_destroy(frame->window);
    free(frame);
}

Window *Frame_window(const Frame *frame) { return frame ? frame->window : NULL; }
void Frame_setTitle(Frame *frame, const char *title) {
    if (frame && frame->window) Window_setTitle(frame->window, title);
}
void  Frame_setBackground(Frame *frame, Color color) { if (frame) frame->background = color; }
Color Frame_background(const Frame *frame) { return frame ? frame->background : COLOR_CLEAR; }

Panel *Frame_addPanel(Frame *frame, const PanelDesc *desc) {
    if (!frame) return NULL;
    if (frame->count == frame->cap) {
        frame->cap = frame->cap ? frame->cap * 2 : 8;
        Panel **grown = realloc(frame->panels, (size_t)frame->cap * sizeof *grown);
        if (!grown) { frame->cap = 0; frame->count = 0; return NULL; }
        frame->panels = grown;
    }
    Panel *p = Panel_new(desc);
    if (!p) return NULL;
    frame->panels[frame->count++] = p;
    return p;
}

int    Frame_count(const Frame *frame) { return frame ? frame->count : 0; }
Panel *Frame_panel(const Frame *frame, int index) {
    if (!frame || index < 0 || index >= frame->count) return NULL;
    return frame->panels[index];
}

void Frame_removePanels(Frame *frame) {
    if (!frame) return;
    for (int i = 0; i < frame->count; i++) Panel_destroy(frame->panels[i]);
    frame->count = 0;
}

Rect Frame_root(const Frame *frame) {
    if (!frame) return (Rect){0, 0, 0, 0};
    // the layout root IS the window (native px): panels anchor to the window and
    // reflow on resize; anything spilling past the window edge is clipped.
    return (Rect){0, 0, (float)frame->lastW, (float)frame->lastH};
}

void Frame_paint(const Frame *frame, DisplayList *dl) {
    if (!frame || !dl) return;
    // The background is the render target's clear, not a quad (no redundant
    // full-frame pass).
    Rect root = Frame_root(frame);
    for (int i = 0; i < frame->count; i++) {
        Panel *p = frame->panels[i];
        Panel_paint(p, Panel_resolve(p, root), dl);
    }
}

void Frame_setSize(Frame *frame, int widthPx, int heightPx) {
    if (!frame || widthPx <= 0 || heightPx <= 0) return;
    if (widthPx == frame->lastW && heightPx == frame->lastH) return;   // no-op
    frame->lastW = widthPx;
    frame->lastH = heightPx;
    Graphics_resize((uint32_t)widthPx, (uint32_t)heightPx);
    Frame_render(frame);
}

void Frame_render(Frame *frame) {
    if (!frame || !frame->window) return;
    if (frame->lastW <= 0 || frame->lastH <= 0) {
        Frame_setSize(frame, Window_width(frame->window), Window_height(frame->window));
        return;
    }
    int wpx = frame->lastW;
    int hpx = frame->lastH;
    if (!Graphics_begin()) return;
    Graphics_clear(frame->background);
    DisplayList_clear(frame->dl);
    Frame_paint(frame, frame->dl);
    Graphics_submit(frame->dl);
    Graphics_end();

    if (Graphics_capture(frame->shot))
        Window_presentRGBA(frame->window, Image_pixels(frame->shot), Image_stride(frame->shot),
                           wpx, hpx);
}

void Frame_run(Frame *frame) {
    if (!frame || !frame->window) return;
    Window_show(frame->window);
    Frame_render(frame);                       // first paint
    // Poll-then-park: drain everything queued (which also mirrors window
    // state), then BLOCK until the OS has work. An idle window costs no CPU;
    // input, moves and live-resize ticks wake us. (The Park Cost Law.)
    while (!Window_shouldClose(frame->window)) {
        Window_pollEvents();                   // drain + reflect everything queued
        if (Window_shouldClose(frame->window)) break;
        Window_waitEvents(frame->window, 0);   // block until the next OS event
    }
}

// ── screenshots ─────────────────────────────────────────────────────────────
Frame *Frame_active(void) { return s_active; }

Image *Frame_capture(Frame *frame) {
    if (!frame) return NULL;
    Frame_render(frame);             // render the current state, then read it
    return frame->shot;
}

bool Frame_savePNG(Frame *frame, const char *path) {
    Image *img = Frame_capture(frame);
    if (!img || !path) return false;
    return Window_writePNG(Image_pixels(img), Image_stride(img),
                           (int)Image_width(img), (int)Image_height(img), path);
}
