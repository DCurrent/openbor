/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

// Sprite queueing and sorting

#include <stdio.h>
#include "types.h"
#include "screen.h"
#include "sprite.h"
#include "draw.h"
#include "globals.h"
// This should be enough for most games...
// But bear in mind that text is also composed of sprites!
#define			MAXQSPRITES		500

#define         SQT_SPRITE      0
#define         SQT_DOT         1
#define         SQT_LINE        2
#define         SQT_BOX         3
#define         SQT_CIRCLE      4
#define         SQT_SHADOW      5 //current unused, use common sprite method instead
#define         SQT_SCREEN      6

#define         SQ_MAX_PARAMS   3


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

static int spritequeue_len = 0;
static int spriteq_old_len = 0;
static int spriteq_locked = 0;

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
        queue[spritequeue_len].drawmethod.flag = 0;
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
        queue[spritequeue_len].drawmethod.flag = 0;
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
        queue[spritequeue_len].drawmethod.flag = 0;
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
        queue[spritequeue_len].drawmethod.flag = 0;
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
        queue[spritequeue_len].drawmethod.flag = 0;
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
        queue[spritequeue_len].drawmethod.flag = 0;
    }
    order[spritequeue_len] = &queue[spritequeue_len];
    ++spritequeue_len;
}


// Double sort code (sorts high and low simultaneously from 2 directions)
// Can't get much faster than this - I think
static void spriteq_sort()
{
    int i, lidx, hidx, lz, hz, start, end, lsid, hsid;
    void *tempp;

    start = 0;
    end = spritequeue_len - 1;

    while(start < end)
    {
        lidx = end;
        hidx = start;

        lz = order[lidx]->z;
        hz = order[hidx]->z;
        lsid = order[lidx]->sortid;
        hsid = order[hidx]->sortid;

        // Search for lowest and highest Z coord
        for(i = start; i <= end; i++)
        {
            if(order[i]->z < lz || (order[i]->z == lz && order[i]->sortid < lsid))
            {
                lidx = i;
                lz = order[i]->z;
                lsid = order[i]->sortid;
            }
            if(order[i]->z > hz || (order[i]->z == hz && order[i]->sortid > hsid))
            {
                hidx = i;
                hz = order[i]->z;
                hsid = order[i]->sortid;
            }
        }

        // No need to sort equal values!
        if(hz == lz && hsid == lsid)
        {
            return;
        }

        // Exchange values (low)
        tempp = order[start];
        order[start] = order[lidx];
        order[lidx] = tempp;

        // Prevent confusion:
        // This value may already have been exchanged!
        if(hidx == start)
        {
            hidx = lidx;
        }

        // Exchange values (high)
        tempp = order[end];
        order[end] = order[hidx];
        order[hidx] = tempp;

        ++start;
        --end;
    }
}

// newonly is 1 means don't draw locked sprites
void spriteq_draw(s_screen *screen, int newonly, int minz, int maxz, int dx, int dy)
{
    int i, x, y;

    spriteq_sort();

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
    }
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




