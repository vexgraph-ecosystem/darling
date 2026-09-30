#ifndef DARLING_EVENT_HIT_H
#define DARLING_EVENT_HIT_H

#include <stdbool.h>
#include <stdint.h>

#include "graphics/graphics.h"   // R4 borrows the R3 Rect shape (no driver handles)

// darling R4 — event/hit.h
//
// RAW EVENTS IN, RESOLVED COORDINATES OUT. Widgets used to each re-derive the
// pointer's position; instead ONE resolver walks the container tree (top-most
// child first, honouring clip + visibility) and answers the pointer's position
// in EVERY coordinate space at once:
//
//     window (0,0 top-left)  •  root  •  parent container  •  self/local
//
// The node model is deliberately tiny (rect + parent index + clip + z) so the
// math is pure and headless; darling's Container/Panel maps 1:1 onto it
// (x,y,w,h <- resolved layout rect in parent space).

typedef struct HitNode {
    int32_t id;        // caller identity (Panel id, widget id, ...)
    int32_t parent;    // index into the array; -1 = root (rect is window space)
    Rect    rect;      // layout rect in PARENT space (native px, Y-down)
    bool    clip;      // clips descendants (and itself) to rect
    bool    visible;   // hidden nodes never hit
    int32_t z;         // higher = on top among siblings
} HitNode;

typedef struct Hit {
    int32_t id;        // hit node id, -1 = miss
    int32_t index;     // hit node index, -1 = miss
    float winX, winY;       // pointer vs. window origin (0,0 top-left, native px)
    float parentX, parentY; // pointer vs. the hit node's PARENT origin (0,0 top-left)
    float localX, localY;   // pointer vs. the hit node's OWN origin (0,0 top-left)
} Hit;

// Resolve a window-space point. Returns true and fills out on a hit.
bool Hit_resolve(const HitNode *nodes, int count, float winX, float winY, Hit *out);

#endif // DARLING_EVENT_HIT_H
