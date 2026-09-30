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
    int lastW, lastH;    // last rendered target size (skip a redundant resize)
};

static void frame_resize_render(void *userdata) {
    Frame_render((Frame *)userdata);   // re-layout + re-render every resize step
}

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
    // Render on the resize cadence: AppKit runs its own nested tracking loop
    // during a live drag, so the ambient Frame_run loop can't keep up. This hook
    // re-lays-out + re-renders at the new size on EVERY geometry step — the
    // window publishes the tick, the Frame draws it (the Continuous Real-Time
    // Live Resize Law). Presentation goes synchronous inside Window_presentRGBA
    // while Window_isLiveResizing() is true.
    Window_setResizeRenderHook(f->window, frame_resize_render, f);
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
    if (!frame || !frame->window) return (Rect){0, 0, 0, 0};
    return (Rect){0, 0, (float)Window_width(frame->window), (float)Window_height(frame->window)};
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

void Frame_render(Frame *frame) {
    if (!frame || !frame->window) return;
    int wpx = Window_width(frame->window);
    int hpx = Window_height(frame->window);
    if (wpx <= 0 || hpx <= 0) return;

    // Reallocate the target ONLY when the size actually changed; a steady window
    // reuses its framebuffer, paint list and capture buffer every frame.
    if (wpx != frame->lastW || hpx != frame->lastH) {
        Graphics_resize((uint32_t)wpx, (uint32_t)hpx);
        frame->lastW = wpx;
        frame->lastH = hpx;
    }
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
    while (!Window_shouldClose(frame->window)) {
        Window_pollEvents();                   // drain everything queued
        usleep(8000);                          // bounded park (~8ms tick)
    }
}
