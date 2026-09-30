#include "event/hit.h"

// darling R4 — event/hit.c
// The coordinate resolver. Pure math, no scene ownership, no allocation.

static void node_origin(const HitNode *n, int count, int i, float *ox, float *oy) {
    float x = 0.0f, y = 0.0f;
    int cur = i, guard = 0;
    while (cur >= 0 && cur < count && guard++ <= count) {
        x += n[cur].rect.x;
        y += n[cur].rect.y;
        cur = n[cur].parent;
    }
    *ox = x;
    *oy = y;
}

static int node_depth(const HitNode *n, int count, int i) {
    int d = 0, cur = n[i].parent, guard = 0;
    while (cur >= 0 && cur < count && guard++ <= count) {
        d++;
        cur = n[cur].parent;
    }
    return d;
}

static bool node_visible_chain(const HitNode *n, int count, int i) {
    int cur = i, guard = 0;
    while (cur >= 0 && cur < count && guard++ <= count) {
        if (!n[cur].visible) return false;
        cur = n[cur].parent;
    }
    return true;
}

// every clipping ancestor (and self) must contain the point
static bool node_clip_chain(const HitNode *n, int count, int i, float px, float py) {
    int cur = i, guard = 0;
    while (cur >= 0 && cur < count && guard++ <= count) {
        if (n[cur].clip) {
            float ox, oy;
            node_origin(n, count, cur, &ox, &oy);
            const Rect r = n[cur].rect;
            if (!(px >= ox && px < ox + r.w && py >= oy && py < oy + r.h)) return false;
        }
        cur = n[cur].parent;
    }
    return true;
}

bool Hit_resolve(const HitNode *nodes, int count, float winX, float winY, Hit *out) {
    if (!nodes || count <= 0 || !out) return false;
    out->id = -1;
    out->index = -1;
    out->winX = winX;
    out->winY = winY;
    out->parentX = winX;
    out->parentY = winY;
    out->localX = 0.0f;
    out->localY = 0.0f;

    int best = -1, bestDepth = -1, bestZ = 0;
    for (int i = 0; i < count; i++) {
        if (!nodes[i].visible) continue;
        float ox, oy;
        node_origin(nodes, count, i, &ox, &oy);
        const Rect r = nodes[i].rect;
        if (!(winX >= ox && winX < ox + r.w && winY >= oy && winY < oy + r.h)) continue;
        if (!node_visible_chain(nodes, count, i)) continue;
        if (!node_clip_chain(nodes, count, i, winX, winY)) continue;
        int depth = node_depth(nodes, count, i);
        int z = nodes[i].z;
        if (depth > bestDepth || (depth == bestDepth && z >= bestZ)) {
            best = i;
            bestDepth = depth;
            bestZ = z;
        }
    }
    if (best < 0) return false;

    float sox, soy, pox = 0.0f, poy = 0.0f;
    node_origin(nodes, count, best, &sox, &soy);
    int parent = nodes[best].parent;
    if (parent >= 0 && parent < count) {
        node_origin(nodes, count, parent, &pox, &poy);
    } else {
        pox = winX;
        poy = winY;
    }
    int root = best, guard = 0;
    while (nodes[root].parent >= 0 && nodes[root].parent < count && guard++ <= count)
        root = nodes[root].parent;

    out->id = nodes[best].id;
    out->index = best;
    out->parentX = best == root ? winX : winX - pox;
    out->parentY = best == root ? winY : winY - poy;
    out->localX = winX - sox;
    out->localY = winY - soy;
    return true;
}
