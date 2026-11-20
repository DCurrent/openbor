/*
 * Damon V. Caskey
 * 2026-11-16
 * Compare launcher glyphs with independent expansion of the original font.
 * Native fixtures supply screen ownership, never expected text pixels.
 */
#define main unused_launcher_backend_suite
#include "test_gpu_renderer.c"
#undef main
#include "launcher_text.h"
#define hankaku_font10 launcher_reference_font
#include "hankaku.h"
#undef hankaku_font10

extern const unsigned char hankaku_font10[];
static unsigned launcher_images_checked, launcher_pixels_checked;
static unsigned char launcher_alternate_font[1600];
static size_t launcher_reject_malloc;
static unsigned launcher_malloc_failures;
static int launcher_reject_texture, launcher_reject_upload;
static unsigned launcher_texture_failures, launcher_upload_failures;
static void (APIENTRY *launcher_real_gen_textures)(GLsizei, GLuint *);
static void (APIENTRY *launcher_real_texture_image)(GLenum, GLint, GLint, GLsizei,
    GLsizei, GLint, GLenum, GLenum, const void *);
void *__real_malloc(size_t size);
void *__real_SDL_GL_GetProcAddress(const char *name);

/*
 * Damon V. Caskey
 * 2026-11-16
 * Reject only the explicitly armed long-format allocation.
 */
void *__wrap_malloc(size_t size)
{
    if(launcher_reject_malloc && size == launcher_reject_malloc)
    {
        ++launcher_malloc_failures;
        return NULL;
    }
    return __real_malloc(size);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Fail atlas allocation through the driver's existing loader boundary.
 */
static void APIENTRY launcher_gen_textures(GLsizei count, GLuint *textures)
{
    int index;
    if(launcher_reject_texture)
    {
        for(index = 0; index < count; ++index) textures[index] = 0;
        ++launcher_texture_failures;
        return;
    }
    launcher_real_gen_textures(count, textures);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Produce a real GL allocation error only for the font's private atlas.
 */
static void APIENTRY launcher_texture_image(GLenum target, GLint level, GLint format,
    GLsizei width, GLsizei height, GLint border, GLenum external_format,
    GLenum type, const void *pixels)
{
    if(launcher_reject_upload && width == 80 && height == 100)
    {
        ++launcher_upload_failures;
        width = -1;
    }
    launcher_real_texture_image(target, level, format, width, height, border,
                                 external_format, type, pixels);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Keep shader compilation, geometry and ordinary textures on the real driver.
 */
void *__wrap_SDL_GL_GetProcAddress(const char *name)
{
    void *address = __real_SDL_GL_GetProcAddress(name);
    if(!strcmp(name, "glGenTextures"))
    {
        launcher_real_gen_textures = address;
        return address ? (void *)launcher_gen_textures : NULL;
    }
    if(!strcmp(name, "glTexImage2D"))
    {
        launcher_real_texture_image = address;
        return address ? (void *)launcher_texture_image : NULL;
    }
    return address;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Use the production variadic formatting boundary without the game/audio menu.
 */
static int launcher_printf(s_screen *screen, int x, int y, unsigned foreground,
                            unsigned background, int fill, const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = launcher_text_vprintf(screen, x, y, foreground, background, fill, format, args);
    va_end(args);
    return result;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Express the original byte repertoire without calling production mapping.
 */
static unsigned launcher_reference_index(unsigned character)
{
    if(character >= 32 && character <= 127) return character - 32;
    if(character >= 160 && character <= 223) return character - 64;
    return 0;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Expand font bits directly into logical RGB pixels. Clipping uses wide signed
 * coordinates; unsupported nonzero bytes remain ordinary five-pixel cells.
 */
static void launcher_reference_text(unsigned *pixels, int width, int height,
    int x, int y, unsigned foreground, unsigned background, int fill,
    const unsigned char *text, const unsigned char *font)
{
    size_t index;
    int column, row;
    for(index = 0; text[index]; ++index)
    {
        unsigned glyph = launcher_reference_index(text[index]);
        for(row = 0; row < 10; ++row)
            for(column = 0; column < 5; ++column)
            {
                int64_t dx = (int64_t)x + (int64_t)index * 5 + column;
                int64_t dy = (int64_t)y + row;
                int ink = (font[glyph * 10 + row] >> column) & 1;
                if(dx >= 0 && dx < width && dy >= 0 && dy < height && (ink || fill))
                    pixels[(size_t)dy * width + (size_t)dx] =
                        (ink ? foreground : background) & 0x00ffffffu;
            }
    }
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Paint expected rectangles independently of native image composition.
 */
static void launcher_reference_box(unsigned *pixels, int width, int height,
    int x, int y, int box_width, int box_height, unsigned color)
{
    int row, column;
    for(row = y; row < y + box_height; ++row)
        for(column = x; column < x + box_width; ++column)
            if(column >= 0 && column < width && row >= 0 && row < height)
                pixels[row * width + column] = color & 0x00ffffffu;
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Download only at an explicit verification boundary and compare every pixel.
 */
static void launcher_verify(s_screen *screen, const unsigned *expected, const char *label)
{
    int index;
    gpu_sync_cpu(screen);
    for(index = 0; index < screen->width * screen->height; ++index)
    {
        unsigned actual = test_pixel(screen, index % screen->width, index / screen->width);
        if(actual != expected[index])
        {
            fprintf(stderr, "%s at(%d,%d): got%06x expected%06x\n", label,
                    index % screen->width, index / screen->width, actual, expected[index]);
            assert(actual == expected[index]);
        }
    }
    ++launcher_images_checked;
    launcher_pixels_checked += (unsigned)(screen->width * screen->height);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Validate every byte and bounded in-place mapping, including mapped zeros
 * inside a run and an embedded source terminator that must stop processing.
 */
static void launcher_test_mapping(void)
{
    static const unsigned char expected_a[10] = {0, 12, 18, 18, 30, 18, 18, 18, 0, 0};
    unsigned character;
    unsigned char bounded[] = {32, 1, 160, 'B', 0x5a};
    unsigned char terminated[] = {'A', 0, 'B', 0x5a};
    assert(sizeof(launcher_reference_font) == 1600);
    assert(!memcmp(launcher_reference_font + ('A' - 32) * 10, expected_a, 10));
    for(character = 1; character < 256; ++character)
    {
        unsigned char text[] = {(unsigned char)character, 'A', 0, 0x5a};
        assert(launcher_text_map(text, sizeof(text)) == 2);
        assert(text[0] == launcher_reference_index(character));
        assert(text[1] == 'A' - 32 && text[2] == 0 && text[3] == 0x5a);
    }
    assert(launcher_text_map(bounded, 4) == 4);
    assert(bounded[0] == 0 && bounded[1] == 0 && bounded[2] == 96 &&
           bounded[3] == 'B' - 32 && bounded[4] == 0x5a);
    assert(launcher_text_map(terminated, sizeof(terminated)) == 1);
    assert(terminated[1] == 0 && terminated[2] == 'B' && terminated[3] == 0x5a);
    assert(launcher_text_map(NULL, 17) == 0);
    assert(launcher_text_map(terminated, 0) == 0);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Empty and fully clipped runs allocate no source or font texture. Invalid
 * glyphs still reject the whole run before any target ownership changes.
 */
static void launcher_test_noops(void)
{
    static const int positions[][2] = {{-5, 0}, {17, 0}, {0, -10}, {0, 13},
                                       {INT_MIN, 0}, {INT_MAX, 0}, {0, INT_MIN}, {0, INT_MAX}};
    s_screen *screen = test_screen(17, 13, PIXEL_32);
    unsigned char glyph = 33;
    gpu_renderer_stats before = *gpu_gl_get_stats();
    size_t index;
    assert(gpu_gl_draw_launcher_text(screen, 0, 0, 0, 0, 1, NULL, 0, NULL, 0));
    assert(launcher_printf(screen, 0, 0, 1, 2, 1, "%s", ""));
    for(index = 0; index < sizeof(positions) / sizeof(positions[0]); ++index)
        assert(gpu_gl_draw_launcher_text(screen, positions[index][0], positions[index][1],
                                         0, 0, 1, &glyph, 1, hankaku_font10, 160));
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(gpu_gl_get_stats()->launcher_text_draws == before.launcher_text_draws);
    assert(gpu_gl_get_stats()->launcher_font_resources == 0);
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Permit immutable font creation and color-palette transfers while excluding
 * destination image uploads, readbacks and copies from normal text draws.
 */
static void launcher_check_transfers(const gpu_renderer_stats *before)
{
    const gpu_renderer_stats *after = gpu_gl_get_stats();
    assert(after->readbacks == before->readbacks);
    assert(after->destination_copies == before->destination_copies);
    assert(after->uploads - before->uploads ==
           after->launcher_font_uploads - before->launcher_font_uploads +
           after->palette_uploads - before->palette_uploads);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Verify one ordinary string against the independent image without allowing
 * font draws to synchronize, overwrite, or reupload the native scene.
 */
static void launcher_text_case(const unsigned char *text, int x, int y,
    unsigned foreground, unsigned background, int fill, int width, int height,
    const char *label)
{
    s_screen *screen = test_screen(width, height, PIXEL_32);
    unsigned *expected = malloc((size_t)width * height * sizeof(*expected));
    gpu_renderer_stats before;
    int index;
    assert(expected);
    for(index = 0; index < width * height; ++index) expected[index] = 0x00396b8du;
    assert(gpu_draw_box(screen, 0, 0, width, height, 0x00396b8du, &plainmethod));
    before = *gpu_gl_get_stats();
    assert(launcher_printf(screen, x, y, foreground, background, fill, "%s", text));
    launcher_check_transfers(&before);
    for(index = 0; index < width * height; ++index) assert(((unsigned *)screen->data)[index] == 0);
    launcher_reference_text(expected, width, height, x, y, foreground, background,
                             fill, text, launcher_reference_font);
    launcher_verify(screen, expected, label);
    free(expected);
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Cover the complete byte repertoire, exact RGB8 colors, opaque black and
 * every cell background while reusing the same immutable font atlas.
 */
static void launcher_test_glyphs(void)
{
    unsigned character;
    int fill;
    for(fill = 0; fill < 2; ++fill)
        for(character = 1; character < 256; ++character)
        {
            unsigned char text[] = {(unsigned char)character, 0};
            unsigned foreground = fill ? 0 : (character | ((255 - character) << 8) |
                                                        (((character * 17) & 255) << 16));
            launcher_text_case(text, 1, 1, foreground, 0x00abcdefu, fill,
                               7, 12, "complete byte repertoire");
            assert(gpu_gl_get_stats()->launcher_font_uploads == 1);
            assert(gpu_gl_get_stats()->launcher_font_resources == 1);
        }
    launcher_text_case((const unsigned char *)" Ag 09~", 1, 1, 0x003517f9u, 0, 1,
                       39, 13, "black background and blank cells");
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Controls retain their original blank-cell advance; edge clipping preserves
 * the original bit phase and never changes text layout or screen globals.
 */
static void launcher_test_layout(void)
{
    static const int positions[][2] = {{-14, -9}, {-7, -3}, {-2, 2}, {3, -4},
                                       {8, 6}, {17, 12}, {-16, 2}, {20, 1}, {1, 14}};
    size_t index;
    int fill;
    launcher_text_case((const unsigned char *)"A\nB\tC\rD", 1, 1, 0x0013579bu,
                       0x00f1e3d5u, 1, 39, 23, "controls occupy blank cells");
    for(fill = 0; fill < 2; ++fill)
        for(index = 0; index < sizeof(positions) / sizeof(positions[0]); ++index)
            launcher_text_case((const unsigned char *)"Ag~", positions[index][0], positions[index][1],
                               0x00fefdffu, 0x00030507u, fill, 19, 13, "clipped glyph phase");
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Exercise both formatting storage paths and embedded NUL termination. Long
 * text starts offscreen so an incorrectly truncated visible suffix is exposed.
 */
static void launcher_test_formatting(void)
{
    s_screen *screen = test_screen(79, 23, PIXEL_32);
    unsigned expected[79 * 23];
    char long_text[551], expected_text[558];
    gpu_renderer_stats before;
    int index;
    memset(long_text, ' ', 543);
    memcpy(long_text + 543, "Visible", 8);
    memcpy(expected_text, long_text, 550);
    memcpy(expected_text + 550, ":0007/Q", 8);
    for(index = 0; index < 79 * 23; ++index) expected[index] = 0x00396b8du;
    assert(gpu_draw_box(screen, 0, 0, 79, 23, 0x00396b8du, &plainmethod));
    before = *gpu_gl_get_stats();
    assert(launcher_printf(screen, -543 * 5 + 2, 1, 0x00e3a107u, 0x00010305u, 1,
                            "%s:%04d/%c", long_text, 7, 'Q'));
    launcher_reference_text(expected, 79, 23, -543 * 5 + 2, 1, 0x00e3a107u,
                             0x00010305u, 1, (unsigned char *)expected_text, launcher_reference_font);
    assert(launcher_printf(screen, 2, 12, 0x00020408u, 0x00fedcbau, 1, "A%cB", 0));
    launcher_reference_text(expected, 79, 23, 2, 12, 0x00020408u, 0x00fedcbau, 1,
                             (const unsigned char *)"A", launcher_reference_font);
    launcher_check_transfers(&before);
    assert(gpu_gl_get_stats()->launcher_font_uploads == before.launcher_font_uploads);
    for(index = 0; index < 79 * 23; ++index) assert(((unsigned *)screen->data)[index] == 0);
    launcher_verify(screen, expected, "long formatting and embedded NUL");
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Reproduce launcher painter order around preview images, background boxes
 * and later text. Native images and the final target stay GPU-authoritative.
 */
static void launcher_test_ordering(void)
{
    s_screen *screen = test_screen(53, 31, PIXEL_32);
    s_screen *background = test_screen(53, 31, PIXEL_32);
    s_screen *preview = test_screen(16, 12, PIXEL_32);
    unsigned expected[53 * 31];
    gpu_renderer_stats before;
    int index;
    for(index = 0; index < 53 * 31; ++index) expected[index] = 0x00204060u;
    assert(gpu_draw_box(background, 0, 0, 53, 31, 0x00204060u, &plainmethod));
    assert(gpu_draw_box(preview, 0, 0, 16, 12, 0x009abcdeu, &plainmethod));
    assert(gpu_draw_screen(screen, background, 0, 0, &plainmethod));
    before = *gpu_gl_get_stats();
    assert(launcher_printf(screen, 4, 3, 0x00fedcba, 0, 0, "BGM"));
    launcher_reference_text(expected, 53, 31, 4, 3, 0x00fedcba, 0, 0,
                             (const unsigned char *)"BGM", launcher_reference_font);
    assert(gpu_draw_screen(screen, preview, 8, 5, &plainmethod));
    launcher_reference_box(expected, 53, 31, 8, 5, 16, 12, 0x009abcdeu);
    assert(launcher_printf(screen, 10, 8, 0, 0x00ffffffu, 0, "Ag"));
    launcher_reference_text(expected, 53, 31, 10, 8, 0, 0x00ffffffu, 0,
                             (const unsigned char *)"Ag", launcher_reference_font);
    assert(gpu_draw_box(screen, 20, 10, 7, 3, 0x00020406u, &plainmethod));
    launcher_reference_box(expected, 53, 31, 20, 10, 7, 3, 0x00020406u);
    assert(launcher_printf(screen, 30, 17, 0x00090705u, 0x00fffdfbu, 1, "UX"));
    launcher_reference_text(expected, 53, 31, 30, 17, 0x00090705u, 0x00fffdfbu, 1,
                             (const unsigned char *)"UX", launcher_reference_font);
    launcher_check_transfers(&before);
    for(index = 0; index < 53 * 31; ++index)
        assert(((unsigned *)screen->data)[index] == 0 && ((unsigned *)background->data)[index] == 0);
    for(index = 0; index < 16 * 12; ++index) assert(((unsigned *)preview->data)[index] == 0);
    launcher_verify(screen, expected, "background, text, preview and later text ordering");
    release_test_screen(preview);
    release_test_screen(background);
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Preflight the entire run, preserve the old atlas across allocation/upload
 * errors and reject failed formatting without altering a single scene pixel.
 */
static void launcher_test_failures(void)
{
    s_screen *screen = test_screen(29, 13, PIXEL_32);
    s_screen *indexed = test_screen(3, 3, PIXEL_8);
    unsigned expected[29 * 13];
    unsigned char glyphs[] = {33, 160};
    char long_text[301];
    gpu_renderer_stats before;
    int index;
    for(index = 0; index < 29 * 13; ++index) expected[index] = 0x00396b8du;
    assert(gpu_draw_box(screen, 0, 0, 29, 13, 0x00396b8du, &plainmethod));
    before = *gpu_gl_get_stats();
    assert(!gpu_gl_draw_launcher_text(screen, 0, 0, 1, 2, 1, glyphs, 2, hankaku_font10, 160));
    assert(!gpu_gl_draw_launcher_text(screen, 0, 0, 1, 2, 1, glyphs, SIZE_MAX, hankaku_font10, 160));
    assert(!gpu_gl_draw_launcher_text(screen, 0, 0, 1, 2, 1, glyphs, 1, hankaku_font10, 159));
    assert(!gpu_gl_draw_launcher_text(screen, 0, 0, 1, 2, 1, glyphs, 1, NULL, 160));
    assert(!gpu_gl_draw_launcher_text(screen, 0, 0, 1, 2, 1, NULL, 1, hankaku_font10, 160));
    assert(!gpu_gl_draw_launcher_text(indexed, 0, 0, 1, 2, 1, NULL, 0, NULL, 0));
    assert(!gpu_gl_draw_launcher_text(NULL, 0, 0, 1, 2, 1, NULL, 0, NULL, 0));
    assert(!launcher_printf(screen, 0, 0, 1, 2, 1, NULL));
    memset(long_text, 'A', sizeof(long_text) - 1);
    long_text[sizeof(long_text) - 1] = 0;
    launcher_reject_malloc = sizeof(long_text);
    assert(!launcher_printf(screen, 0, 0, 1, 2, 1, "%s", long_text));
    launcher_reject_malloc = 0;
    assert(launcher_malloc_failures == 1);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(gpu_gl_get_stats()->launcher_text_draws == before.launcher_text_draws);
    launcher_verify(screen, expected, "invalid run and failed formatting are atomic");

    memcpy(launcher_alternate_font, launcher_reference_font, sizeof(launcher_alternate_font));
    launcher_alternate_font[33 * 10 + 1] ^= 1;
    before = *gpu_gl_get_stats();
    launcher_reject_texture = 1;
    assert(!gpu_gl_draw_launcher_text(screen, 1, 1, 0x00030507u, 0, 1,
                                      glyphs, 1, launcher_alternate_font, 160));
    launcher_reject_texture = 0;
    assert(launcher_texture_failures == 1);
    launcher_reject_upload = 1;
    assert(!gpu_gl_draw_launcher_text(screen, 1, 1, 0x00030507u, 0, 1,
                                      glyphs, 1, launcher_alternate_font, 160));
    launcher_reject_upload = 0;
    assert(launcher_upload_failures == 1);
    assert(gpu_gl_get_stats()->launcher_font_uploads == before.launcher_font_uploads);
    assert(gpu_gl_get_stats()->launcher_font_resources == 1);
    assert(gpu_gl_get_stats()->launcher_text_draws == before.launcher_text_draws);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(gpu_gl_get_stats()->uploads == before.uploads);
    assert(!gpu_allow_cpu_operation(screen, GPU_OPERATION_SCREEN));
    launcher_verify(screen, expected, "failed atlas replacement is atomic");
    assert(launcher_printf(screen, 7, 1, 0x00a1b3c5u, 0x00010305u, 1, "B"));
    assert(gpu_gl_get_stats()->launcher_font_uploads == before.launcher_font_uploads);
    launcher_reference_text(expected, 29, 13, 7, 1, 0x00a1b3c5u, 0x00010305u, 1,
                             (const unsigned char *)"B", launcher_reference_font);
    assert(gpu_gl_draw_launcher_text(screen, 1, 1, 0x00030507u, 0, 1,
                                     glyphs, 1, launcher_alternate_font, 160));
    assert(gpu_gl_get_stats()->launcher_font_uploads == before.launcher_font_uploads + 1);
    launcher_reference_text(expected, 29, 13, 1, 1, 0x00030507u, 0, 1,
                             (const unsigned char *)"A", launcher_alternate_font);
    launcher_verify(screen, expected, "atlas rollback and native recovery");
    release_test_screen(indexed);
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Preserve live scene pixels while retiring private font storage. The next
 * actual context lazily rebuilds the atlas and resumes ordinary native draws.
 */
static void launcher_test_context(SDL_Window *window, SDL_GLContext *context)
{
    s_screen *screen = test_screen(17, 13, PIXEL_32);
    unsigned expected[17 * 13], shadow[17 * 13];
    gpu_renderer_stats before;
    int index;
    for(index = 0; index < 17 * 13; ++index) expected[index] = 0x003d2a1fu;
    assert(gpu_draw_box(screen, 0, 0, 17, 13, 0x003d2a1fu, &plainmethod));
    assert(launcher_printf(screen, 1, 1, 0x00010307u, 0x00fdfbf9u, 1, "A"));
    launcher_reference_text(expected, 17, 13, 1, 1, 0x00010307u, 0x00fdfbf9u, 1,
                             (const unsigned char *)"A", launcher_reference_font);
    gpu_unregister_backend();
    assert(!launcher_printf(screen, 1, 1, 1, 2, 1, "Detached backend"));
    for(index = 0; index < 17 * 13; ++index) assert(((unsigned *)screen->data)[index] == 0);
    before = *gpu_gl_get_stats();
    gpu_gl_shutdown();
    assert(gpu_gl_get_stats()->launcher_font_resources == 0);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks + 1);
    for(index = 0; index < 17 * 13; ++index)
        assert(test_pixel(screen, index % 17, index / 17) == expected[index]);
    memcpy(shadow, screen->data, sizeof(shadow));
    assert(!launcher_printf(screen, 1, 1, 1, 2, 1, "No backend"));
    assert(!memcmp(shadow, screen->data, sizeof(shadow)));
    assert(!gpu_allow_cpu_operation(screen, GPU_OPERATION_SCREEN));
    SDL_GL_DeleteContext(*context);
    *context = SDL_GL_CreateContext(window);
    assert(*context && LoadGLFunctions() && gpu_gl_init());
    gpu_set_native_required(1);
    assert(gpu_gl_get_stats()->launcher_font_resources == 0);
    before = *gpu_gl_get_stats();
    assert(launcher_printf(screen, 6, 2, 0x00b1c3d5u, 0, 0, "B"));
    assert(gpu_gl_get_stats()->launcher_font_uploads == 1);
    assert(gpu_gl_get_stats()->readbacks == before.readbacks);
    assert(!memcmp(shadow, screen->data, sizeof(shadow)));
    launcher_reference_text(expected, 17, 13, 6, 2, 0x00b1c3d5u, 0, 0,
                             (const unsigned char *)"B", launcher_reference_font);
    launcher_verify(screen, expected, "context replacement and surviving scene");
    release_test_screen(screen);
}

/*
 * Damon V. Caskey
 * 2026-11-16
 * Exercise real desktop GL without a software text implementation or retry.
 */
int main(void)
{
    SDL_Window *window;
    SDL_GLContext context;
    assert(sizeof(void *) == 8);
    launcher_test_mapping();
    if(SDL_Init(SDL_INIT_VIDEO) != 0) return 77;
    gpu_test_context_attributes();
    window = SDL_CreateWindow("OpenBOR launcher text tests", 0, 0, 64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    context = window ? SDL_GL_CreateContext(window) : NULL;
    if(!context)
    {
        if(window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    assert(LoadGLFunctions() && gpu_gl_init());
    gpu_set_native_required(1);
    launcher_test_noops();
    launcher_test_glyphs();
    launcher_test_layout();
    launcher_test_formatting();
    launcher_test_ordering();
    launcher_test_failures();
    launcher_test_context(window, &context);
    assert(gpu_gl_get_stats()->fallback_draws == 0);
    gpu_gl_shutdown();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("GPU launcher text tests passed: %u complete images, %u exact RGB8 pixels; byte mapping, glyphs, fill, clipping, formatting, ordering, no implicit readback, atlas failure/recovery and context replacement.\n",
           launcher_images_checked, launcher_pixels_checked);
    return 0;
}
