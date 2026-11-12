/*
 * Test the production sprite queue sorter without requiring a video backend.
 *
 * Build from the repository root:
 * gcc -std=c99 -O2 -ffunction-sections -fdata-sections \
 *   -Iengine -Iengine/source -Iengine/source/gamelib \
 *   tests/test_spriteq_sort.c -Wl,--gc-sections -o test_spriteq_sort
 *
 * The linker drops unused drawing functions and their backend references.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Access the real static queue and sort function in this test translation unit. */
#define PP_TEST 1 /* Keep standard assert/printf instead of engine log wrappers. */
#include "../engine/source/gamelib/spriteq.c"

static uint32_t random_state = UINT32_C(0x5A17B20C);
static int drawn_ordinals[MAXQSPRITES];
static int draw_count = 0;

/* Capture the public spriteq_draw() order without an actual display. */
void putpixel(unsigned x, unsigned y, int colour, s_screen *screen, s_drawmethod *method)
{
    (void)y;
    (void)colour;
    (void)screen;
    (void)method;
    drawn_ordinals[draw_count++] = (int)x;
}

void putsprite(int x, int y, s_sprite *frame, s_screen *screen, s_drawmethod *method)
{
    (void)x;
    (void)y;
    (void)frame;
    (void)screen;
    (void)method;
}

void putscreen(s_screen *dest, s_screen *src, int x, int y, s_drawmethod *method)
{
    (void)dest;
    (void)src;
    (void)x;
    (void)y;
    (void)method;
}

void putline(int sx, int sy, int ex, int ey, int colour, s_screen *screen, s_drawmethod *method)
{
    (void)sx;
    (void)sy;
    (void)ex;
    (void)ey;
    (void)colour;
    (void)screen;
    (void)method;
}

void putbox(int x, int y, int width, int height, int colour, s_screen *screen, s_drawmethod *method)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)colour;
    (void)screen;
    (void)method;
}

static uint32_t next_random(void)
{
    uint32_t x = random_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    random_state = x;
    return x;
}

static void prepare_queue(int count)
{
    int i;
    spritequeue_len = count;
    spriteq_old_len = 0;
    spriteq_locked = 0;
    for(i = 0; i < count; i++)
    {
        queue[i].x = i; /* Insertion ordinal and mutation sentinel. */
        order[i] = &queue[i];
    }
}

static void verify_queue(int count)
{
    unsigned char seen[MAXQSPRITES] = {0};
    qstruct *before[MAXQSPRITES];
    int i;

    assert(spritequeue_len == count);
    for(i = 0; i < count; i++)
    {
        ptrdiff_t index = order[i] - queue;
        assert(index >= 0 && index < count);
        assert(!seen[index]);
        seen[index] = 1;
        assert(queue[index].x == index); /* Records were never moved. */
        if(i > 0)
        {
            int comparison = spriteq_compare(order[i - 1], order[i]);
            assert(comparison <= 0);
            if(comparison == 0)
            {
                assert(order[i - 1]->x < order[i]->x); /* Stable ties. */
            }
        }
        before[i] = order[i];
    }

    spriteq_sort(); /* Sorting an unchanged queue must be idempotent. */
    assert(memcmp(before, order, (size_t)count * sizeof(*order)) == 0);
}

static void test_small_queues(void)
{
    int count, i;
    for(count = 0; count <= 64; count++)
    {
        prepare_queue(count);
        for(i = 0; i < count; i++)
        {
            queue[i].z = (count - i) % 5 - 2;
            queue[i].sortid = (count - i) % 3 - 1;
        }
        spriteq_sort();
        verify_queue(count);
    }
}

static void test_large_queues(void)
{
    const int lengths[] = {480, 1000, 2000, MAXQSPRITES};
    unsigned size_index;
    int iteration, i, count;

    for(size_index = 0; size_index < sizeof(lengths) / sizeof(lengths[0]); size_index++)
    {
        count = lengths[size_index];
        for(iteration = 0; iteration < 25; iteration++)
        {
            prepare_queue(count);
            for(i = 0; i < count; i++)
            {
                queue[i].z = (int)(next_random() % 97) - 48;
                queue[i].sortid = (int)(next_random() % 7) - 3;
            }
            queue[0].z = INT_MIN;
            queue[count - 1].z = INT_MAX;
            spriteq_sort();
            verify_queue(count);
        }

        /* Already sorted, all-equal, and reverse-key input distributions. */
        for(iteration = 0; iteration < 3; iteration++)
        {
            prepare_queue(count);
            for(i = 0; i < count; i++)
            {
                queue[i].z = iteration == 0 ? i : iteration == 1 ? 7 : count - i;
                queue[i].sortid = iteration == 1 ? 0 : i;
            }
            spriteq_sort();
            verify_queue(count);
        }
    }
}

static void test_locked_queue(void)
{
    int i;
    prepare_queue(4);
    for(i = 0; i < 4; i++)
    {
        queue[i].z = 4 - i;
        queue[i].sortid = i;
    }
    spriteq_lock();
    queue[4].x = 4;
    queue[4].z = 2;
    queue[4].sortid = 10;
    order[4] = &queue[4];
    spritequeue_len = 5;
    spriteq_sort();
    verify_queue(5);

    spriteq_clear(); /* Retains the first four entries and resets pointers. */
    assert(spritequeue_len == 4);
    for(i = 0; i < 4; i++) assert(order[i] == queue + i);
    spriteq_sort();
    verify_queue(4);

    spriteq_unlock();
    spriteq_clear();
    assert(spritequeue_len == 0);
}

static void test_draw_order_and_filters(void)
{
    const int expected_all[] = {11, 12, 13, 10};
    const int expected_layer[] = {12, 13};
    int i;

    prepare_queue(4);
    for(i = 0; i < 4; i++)
    {
        queue[i].type = SQT_DOT;
        queue[i].sortid = 0;
        queue[i].y = 0;
        queue[i].params[0] = 0;
    }
    queue[0].z = 2;
    queue[1].z = 0;
    queue[2].z = 1;
    queue[3].z = 1;

    /* Drawing twice cannot change equal-key composition. */
    for(i = 0; i < 2; i++)
    {
        draw_count = 0;
        spriteq_draw(NULL, 0, INT_MIN, INT_MAX, 10, 0);
        assert(draw_count == 4);
        assert(memcmp(drawn_ordinals, expected_all, sizeof(expected_all)) == 0);
    }

    draw_count = 0;
    spriteq_draw(NULL, 0, 1, 1, 10, 0);
    assert(draw_count == 2);
    assert(memcmp(drawn_ordinals, expected_layer, sizeof(expected_layer)) == 0);

    spriteq_lock();
    queue[4].x = 4;
    queue[4].y = 0;
    queue[4].z = 1;
    queue[4].sortid = 0;
    queue[4].type = SQT_DOT;
    queue[4].params[0] = 0;
    order[4] = &queue[4];
    spritequeue_len = 5;

    draw_count = 0;
    spriteq_draw(NULL, 1, INT_MIN, INT_MAX, 10, 0);
    assert(draw_count == 1 && drawn_ordinals[0] == 14);

    spriteq_unlock();
    spriteq_clear();
}

int main(void)
{
    test_small_queues();
    test_large_queues();
    test_locked_queue();
    test_draw_order_and_filters();
    puts("spriteq stable-sort tests passed");
    return 0;
}
