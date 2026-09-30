#ifndef DARLING_FRAME_H
#define DARLING_FRAME_H

#include <stdbool.h>

#include "graphics/graphics.h"   // graphvex R3: Rect, Color, DisplayList
#include "panel.h"               // graphvex R3: Panel (the UI component)
#include "window/window.h"       // hotcwap R1: the OS window

// darling R4 — frame.h
//
// THE JFRAME. A real, draggable OS window (owned by hotcwap, R1) that hosts a
// tree of Panels and presents them. This is the top of the UI:
//
//   Frame *f = Frame_2("darling", 760, 520);
//   Panel *p = Frame_addPanel(f, &(PanelDesc){ .anchor = PART_CENTER, ... });
//   Frame_run(f);   // show the window and pump until it closes
//
// Presentation today is software: the frame is raster-rendered by graphvex and
// handed to Window_presentRGBA. The GPU (Metal) seam replaces this one call
// later without changing a single call site here.

typedef struct Frame Frame;

Frame *Frame_2(const char *title, int widthPx, int heightPx);
void Frame_destroy(Frame *frame);

Window *Frame_window(const Frame *frame);
void Frame_setTitle(Frame *frame, const char *title);
void Frame_setBackground(Frame *frame, Color color);
Color Frame_background(const Frame *frame);

// Children (owned). Frame_addPanel creates the panel from the desc and returns
// it so the caller can keep tweaking it.
Panel *Frame_addPanel(Frame *frame, const PanelDesc *desc);
int    Frame_count(const Frame *frame);
Panel *Frame_panel(const Frame *frame, int index);
void   Frame_removePanels(Frame *frame);

// The frame's content rect in native px (what children anchor against).
Rect Frame_root(const Frame *frame);

void Frame_paint(const Frame *frame, DisplayList *dl);
void Frame_render(Frame *frame);   // render + present one frame
void Frame_run(Frame *frame);      // show the window and loop until it closes

#endif // DARLING_FRAME_H
