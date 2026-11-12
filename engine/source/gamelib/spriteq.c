/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

// Sprite queueing and sorting

#include <stdio.h>
#include <string.h>
#include "types.h"
#include "screen.h"
#include "sprite.h"
#include "draw.h"
#include "globals.h"
#include "engine_profile.h"

// This should be enough for most games...
// But bear in mind that text is also composed of sprites!
#define			MAXQSPRITES		5000 // DEFAULT 5000

#define         SQT_SPRITE      0
#define         SQT_DOT         1
#define         SQT_LINE        2
#define         SQT_BOX         3
#define         SQT_CIRCLE      4
#define         SQT_SHADOW      5 //current unused, use common sprite method instead
#define         SQT_SCREEN      6

#define         SQ_MAX_PARAMS   3
#define         SPRITEQ_INSERTION_SORT_THRESHOLD 32

typedef struct
{
    int		x;
    int		y;
    int	    z;
    int    sortid;
    void  *frame;
    s_drawmethod drawmethod;
    int     params[SQ_MAX_PARAMS];
    int     type;
} qstruct;

static qstruct queue[MAXQSPRITES];
static qstruct *order[MAXQSPRITES];
static qstruct *sort_scratch[MAXQSPRITES];

static int spritequeue_len = 0;
static int spriteq_old_len = 0;
static int spriteq_locked = 0;

// Return the current sprite count.
int spriteq_get_sprite_count()
{
    return spritequeue_len;
}

// Return the current sprite .
int spriteq_get_sprite_max()
{
    return MAXQSPRITES;
}

void spriteq_add_frame(int x, int y, int z, s_sprite *frame, s_drawmethod *pdrawmethod, int sortid)
{
    if(frame == NULL)
    {
        return;
    }
    if(spritequeue_len >= MAXQSPRITES)
    {
        return;
    }
    queue[spritequeue_len].type = SQT_SPRITE;
    queue[spritequeue_len].x = x;
    queue[spritequeue_len].y = y;
    queue[spritequeue_len].z = z;
    queue[spritequeue_len].sortid = sortid;
    queue[spritequeue_len].frame = frame;
    if(pdrawmethod)
    {
        queue[spritequeue_len].drawmethod = *pdrawmethod;
    }
    else
    {
        queue[spritequeue_len].drawmethod.config &= ~DRAWMETHOD_CONFIG_ENABLED;
    }
    queue[spritequeue_len].params[0] = 0; // determin if the sprite's center should be readjusted;
    order[spritequeue_len] = &queue[spritequeue_len];
    ++spritequeue_len;
}

void spriteq_add_sprite(int x, int y, int z, int id, s_drawmethod *pdrawmethod, int sortid)
{
    extern s_sprite_map *sprite_map;
    s_sprite *loadsprite2(char * filename, int * width, int * height);
    s_sprite *frame = sprite_map[id].node->sprite;
    if(frame == NULL)
    {
        sprite_map[id].node->sprite = frame = loadsprite2(sprite_map[id].node->filename, NULL, NULL);
    }
    if(spritequeue_len >= MAXQSPRITES)
    {
        return;
    }
    queue[spritequeue_len].type = SQT_SPRITE;
    queue[spritequeue_len].x = x;
    queue[spritequeue_len].y = y;
    queue[spritequeue_len].z = z;
    queue[spritequeue_len].sortid = sortid;
    queue[spritequeue_len].frame = frame;
    if(pdrawmethod)
    {
        queue[spritequeue_len].drawmethod = *pdrawmethod;
    }
    else
    {
        queue[spritequeue_len].drawmethod.config &= ~DRAWMETHOD_CONFIG_ENABLED;
    }
    queue[spritequeue_len].params[0] = 1; // determin if the sprite's center should be readjusted;
    queue[spritequeue_len].params[1] = sprite_map[id].centerx; // centerx
    queue[spritequeue_len].params[2] = sprite_map[id].centery; // centery
    order[spritequeue_len] = &queue[spritequeue_len];
    ++spritequeue_len;
}

void spriteq_add_screen(int x, int y, int z, s_screen *ps, s_drawmethod *pdrawmethod, int sortid)
{
    if(spritequeue_len >= MAXQSPRITES)
    {
        return;
    }
    if(ps == NULL)
    {
        return;
    }
    queue[spritequeue_len].type = SQT_SCREEN;
    queue[spritequeue_len].x = x;
    queue[spritequeue_len].y = y;
    queue[spritequeue_len].z = z;
    queue[spritequeue_len].sortid = sortid;
    queue[spritequeue_len].frame = ps;
    if(pdrawmethod)
    {
        queue[spritequeue_len].drawmethod = *pdrawmethod;
    }
    else
    {
        queue[spritequeue_len].drawmethod.config &= ~DRAWMETHOD_CONFIG_ENABLED;
    }
    order[spritequeue_len] = &queue[spritequeue_len];
    ++spritequeue_len;
}

void spriteq_add_dot(int sx, int sy, int z, int colour, s_drawmethod *pdrawmethod)
{
    if(spritequeue_len >= MAXQSPRITES)
    {
        return;
    }
    queue[spritequeue_len].type = SQT_DOT;
    queue[spritequeue_len].x = sx;
    queue[spritequeue_len].y = sy;
    queue[spritequeue_len].z = z;
    queue[spritequeue_len].sortid = 0;
    queue[spritequeue_len].params[0] = colour;
    queue[spritequeue_len].frame = NULL;
    if(pdrawmethod)
    {
        queue[spritequeue_len].drawmethod = *pdrawmethod;
    }
    else
    {
        queue[spritequeue_len].drawmethod.config &= ~DRAWMETHOD_CONFIG_ENABLED;
    }
    order[spritequeue_len] = &queue[spritequeue_len];
    ++spritequeue_len;
}

void spriteq_add_line(int sx, int sy, int ex, int ey, int z, int colour, s_drawmethod *pdrawmethod)
{
    if(spritequeue_len >= MAXQSPRITES)
    {
        return;
    }
    queue[spritequeue_len].type = SQT_LINE;
    queue[spritequeue_len].x = sx;
    queue[spritequeue_len].y = sy;
    queue[spritequeue_len].params[0] = colour;
    queue[spritequeue_len].params[1] = ex;
    queue[spritequeue_len].params[2] = ey;
    queue[spritequeue_len].z = z;
    queue[spritequeue_len].sortid = 0;
    queue[spritequeue_len].frame = NULL;
    if(pdrawmethod)
    {
        queue[spritequeue_len].drawmethod = *pdrawmethod;
    }
    else
    {
        queue[spritequeue_len].drawmethod.config &= ~DRAWMETHOD_CONFIG_ENABLED;
    }
    order[spritequeue_len] = &queue[spritequeue_len];
    ++spritequeue_len;
}

void spriteq_add_box(int x, int y, int width, int height, int z, int colour, s_drawmethod *pdrawmethod)
{
    if(spritequeue_len >= MAXQSPRITES)
    {
        return;
    }
    queue[spritequeue_len].type = SQT_BOX;
    queue[spritequeue_len].x = x;
    queue[spritequeue_len].y = y;
    queue[spritequeue_len].params[0] = colour;
    queue[spritequeue_len].params[1] = width;
    queue[spritequeue_len].params[2] = height;
    queue[spritequeue_len].z = z;
    queue[spritequeue_len].sortid = 0;
    queue[spritequeue_len].frame = NULL;
    if(pdrawmethod)
    {
        queue[spritequeue_len].drawmethod = *pdrawmethod;
    }
    else
    {
        queue[spritequeue_len].drawmethod.config &= ~DRAWMETHOD_CONFIG_ENABLED;
    }
    order[spritequeue_len] = &queue[spritequeue_len];
    ++spritequeue_len;
}


/*
 * Compare the two queue keys without subtraction, which could overflow for
 * sprites using extreme positive or negative layer values.
 */
static int spriteq_compare(const qstruct *left, const qstruct *right)
{
    if(left->z < right->z) return -1;
    if(left->z > right->z) return 1;
    if(left->sortid < right->sortid) return -1;
    if(left->sortid > right->sortid) return 1;
    return 0;
}

/*
 * Stable sort by layer, then sort ID. Equal keys retain queue submission
 * order, including when the same queue is drawn more than once.
 *
 * Sort the pointer array rather than moving queue records: queue locking and
 * the "new only" draw option identify entries by their original addresses.
 * The temporary array is allocated once, not on every frame.
 */
static void spriteq_sort()
{
    int i, j, width, start, middle, end, left, right, write;
    qstruct *current;
    qstruct **source = order;
    qstruct **destination = sort_scratch;
    qstruct **temp;

    if(spritequeue_len < 2) return;

    /* Already sorted queues, including identical-key queues, need no work. */
    for(i = 1; i < spritequeue_len; i++)
    {
        if(spriteq_compare(order[i - 1], order[i]) > 0) break;
    }
    if(i == spritequeue_len) return;

    /* Avoid merge passes for the short queues common in menus and HUDs. */
    if(spritequeue_len <= SPRITEQ_INSERTION_SORT_THRESHOLD)
    {
        for(i = 1; i < spritequeue_len; i++)
        {
            current = order[i];
            j = i;
            while(j > 0 && spriteq_compare(current, order[j - 1]) < 0)
            {
                order[j] = order[j - 1];
                --j;
            }
            order[j] = current;
        }
        return;
    }

    /* Bottom-up merge sort guarantees O(n log n) worst-case work. */
    for(width = 1; width < spritequeue_len; width *= 2)
    {
        for(start = 0; start < spritequeue_len; start += width * 2)
        {
            middle = start + width;
            if(middle > spritequeue_len) middle = spritequeue_len;
            end = start + width * 2;
            if(end > spritequeue_len) end = spritequeue_len;

            left = start;
            right = middle;
            for(write = start; write < end; write++)
            {
                /* Left wins a tie, preserving the original submission order. */
                if(left < middle &&
                    (right == end || spriteq_compare(source[left], source[right]) <= 0))
                {
                    destination[write] = source[left++];
                }
                else
                {
                    destination[write] = source[right++];
                }
            }
        }

        temp = source;
        source = destination;
        destination = temp;
    }

    /* The draw path always reads order[], regardless of the number of passes. */
    if(source != order)
    {
        memcpy(order, source, spritequeue_len * sizeof(*order));
    }
}

/*
 * Damon V. Caskey
 * 2026-11-12
 * Play the sorted queue into its selected destination, retaining new-only and
 * layer-range filtering. Measure sorting separately from playback and classify
 * screen captures independently from the frame's main destination.
 */
void spriteq_draw(s_screen *screen, int newonly, int minz, int maxz, int dx, int dy)
{
    int i, x, y;
    int accepted = 0;
    int main_target = engine_profile_is_main_target(screen);
    ep_scope sort_scope = engine_profile_span_begin();
    ep_scope playback_scope;
    uint64_t gpu_scope;

    /* Sorting sees the complete queue even when this draw accepts only a layer
     * range or the unlocked suffix. Record both the examined and accepted sizes. */
    spriteq_sort();
    engine_profile_span_end(sort_scope, main_target ? EP_SPAN_MAIN_SORT : EP_SPAN_CAPTURE_SORT);
    engine_profile_count(main_target ? EP_COUNT_MAIN_QUEUE_CALLS : EP_COUNT_CAPTURE_QUEUE_CALLS, 1);
    engine_profile_count(main_target ? EP_COUNT_MAIN_ENTRIES : EP_COUNT_CAPTURE_ENTRIES, spritequeue_len);

    /* Optional GPU timestamps bracket playback alone. Their asynchronous result
     * belongs to the originating frame even if another capture follows it. */
    gpu_scope = engine_profile_gpu_begin(main_target ? EP_GPU_MAIN_QUEUE : EP_GPU_CAPTURE);
    playback_scope = engine_profile_span_begin();

    for(i = 0; i < spritequeue_len; i++)
    {
        if((newonly && spriteq_locked && order[i] < queue + spriteq_old_len) || order[i]->z < minz || order[i]->z > maxz)
        {
            continue;
        }

        x = order[i]->x + dx;
        y = order[i]->y + dy;

        switch(order[i]->type)
        {
        case SQT_SPRITE: // sprite

            if(order[i]->params[0])// determin if the sprite's center should be readjusted;
            {
                ((s_sprite *)(order[i]->frame))->centerx = order[i]->params[1];
                ((s_sprite *)(order[i]->frame))->centery = order[i]->params[2];
            }
            putsprite(x, y, order[i]->frame, screen, &(order[i]->drawmethod));
            break;
        case SQT_SCREEN: // draw a screen instead of sprite
            putscreen(screen, (s_screen *)(order[i]->frame), x, y, &(order[i]->drawmethod));
            break;
        case SQT_DOT:
            putpixel(x, y, order[i]->params[0], screen, &(order[i]->drawmethod));
            break;
        case SQT_LINE:
            putline(x, y, order[i]->params[1] + dx, order[i]->params[2] + dy, order[i]->params[0], screen, &(order[i]->drawmethod));
            break;
        case SQT_BOX:
            putbox(x, y, order[i]->params[1], order[i]->params[2], order[i]->params[0], screen, &(order[i]->drawmethod));
            break;
        default:
            continue;
        }
        accepted++;
    }

    /* Accepted commands can still be clipped by their rasterizer. Keep this
     * command count distinct from the backend's native-draw and fallback counts. */
    engine_profile_span_end(playback_scope, main_target ? EP_SPAN_MAIN_PLAYBACK : EP_SPAN_CAPTURE_PLAYBACK);
    engine_profile_gpu_end(gpu_scope);
    engine_profile_count(main_target ? EP_COUNT_MAIN_ACCEPTED : EP_COUNT_CAPTURE_ACCEPTED, accepted);
}

// UT: lock the spriteq, don't clearn old sprites
void spriteq_lock()
{
    spriteq_old_len = spritequeue_len;
    spriteq_locked = 1;
}

// don't forget to unlock the queue, or we'll have troubles
// if the sprite is unloaded for some reason
void spriteq_unlock()
{
    spriteq_locked = 0;
}

int  spriteq_islocked()
{
    return (spriteq_locked != 0);
}

void spriteq_clear()
{
    int i;
    if(spriteq_locked)
    {
        // when locked, always draw previous sprites,
        // and only clear new sprites
        spritequeue_len = spriteq_old_len;
        for(i = 0; i < spriteq_old_len; i++)
        {
            order[i] = queue + i;
        }
    }
    else
    {
        spriteq_old_len = spritequeue_len;
        spritequeue_len = 0;
    }
}




