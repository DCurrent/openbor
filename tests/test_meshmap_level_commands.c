/*
 * Exercise the level loader's real command lookup and complete-line argument
 * reader before the production mesh authoring parser. Direct argument-array
 * tests cannot detect the legacy loader dropping values at its text/count caps.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scriptcommon.h"
#include "commands.h"
#include "../engine/source/gamelib/meshmap_image.h"

#undef malloc
#undef calloc
#undef realloc

s_level *level;
char packfile[MAX_FILENAME_LEN] = "";

void *__wrap_checkAlloc(void *pointer, size_t size, const char *function,
                        const char *file, int line)
{
    (void)size; (void)function; (void)file; (void)line;
    assert(pointer);
    return pointer;
}

void borShutdown(int status, const char *message, ...)
{
    va_list arguments;
    (void)status;
    va_start(arguments, message);
    vfprintf(stderr, message, arguments);
    va_end(arguments);
    abort();
}

/* These declarations only change rules and active children. Loading images is
 * covered by the image and script suites, and must not occur in this fixture. */
int meshmap_image_replace_geometry(s_meshmap *map, const char *filename,
                                   const char *archive, char *error, size_t capacity)
{
    (void)map; (void)filename; (void)archive; (void)error; (void)capacity;
    assert(!"unexpected geometry load");
    return 0;
}

int meshmap_image_replace_artwork(s_meshmap *map, const char *filename,
                                  const char *archive, char *error, size_t capacity)
{
    (void)map; (void)filename; (void)archive; (void)error; (void)capacity;
    assert(!"unexpected artwork load");
    return 0;
}

#include "meshmap_level_loader.inc"

static int load_command(List *commands, char *line, char *error, size_t capacity)
{
    ArgList legacy;
    char legacy_text[MAX_ARG_LEN + 1];
    levelCommands command;

    /* Match load_level: the legacy reader identifies the command, then the
     * mesh-specific path must consume the original, untruncated source line. */
    assert(ParseArgs(&legacy, line, legacy_text));
    command = getLevelCommand(commands, legacy.args[0]);
    assert(command);
    return load_meshmap_command(command, line, error, capacity);
}

static void append_slot(char *line, size_t capacity, unsigned slot)
{
    size_t length = strlen(line);
    int written = snprintf(line + length, capacity - length, " %u", slot);
    assert(written > 0 && (size_t)written < capacity - length);
}

int main(void)
{
    s_level fixture = {0};
    List *commands = createLevelCommandList();
    s_meshmap *map;
    char line[MAX_ARG_LEN * 4], error[512];
    uint32_t id;
    uint64_t revision;
    unsigned i;

    level = &fixture;
    level->meshmaps = meshmap_store_create();
    assert(level->meshmaps);
    assert(getLevelCommand(commands, "meshmapflags") == CMD_LEVEL_MESHMAPFLAGS);
    assert(getLevelCommand(commands, "MeShMaPfLaGs") == CMD_LEVEL_MESHMAPFLAGS);
    assert(getLevelCommand(commands, "meshmaprules") == CMD_LEVEL_MESHMAPRULES);
    assert(getLevelCommand(commands, "meshmapmask") == CMD_LEVEL_MESHMAPMASK);

    for(i = 0; i < MESHMAP_BANK_SLOTS; ++i)
    {
        assert(meshmap_pack_id(i, 0, 0, 0, &id));
        assert(meshmap_bank_get(level->meshmaps, 1, id, 1));
    }
    map = meshmap_create(level->meshmaps, 0);
    assert(map);

    strcpy(line, "meshmapmask 0 0 slots");
    for(i = 0; i < MESHMAP_BANK_SLOTS; ++i) append_slot(line, sizeof(line), i);
    assert(meshmap_bank_set_mask(&level->meshmaps->root, 0));
    assert(load_command(commands, line, error, sizeof(error)));
    assert(level->meshmaps->root.active_mask == UINT64_MAX);

    /* The last token is beyond the legacy count limit. It must reach validation,
     * reject the whole declaration, and preserve the previous selection. */
    assert(meshmap_bank_set_mask(&level->meshmaps->root, UINT64_C(1) << 7));
    append_slot(line, sizeof(line), MESHMAP_BANK_SLOTS);
    assert(!load_command(commands, line, error, sizeof(error)));
    assert(level->meshmaps->root.active_mask == (UINT64_C(1) << 7));
    assert(strstr(error, "slots"));

    strcpy(line, "meshmapmask 0 0 slots");
    for(i = 0; i < MAX_ARG_LEN; ++i) append_slot(line, sizeof(line), 1);
    append_slot(line, sizeof(line), 63);
    assert(strlen(line) > MAX_ARG_LEN);
    assert(load_command(commands, line, error, sizeof(error)));
    assert(level->meshmaps->root.active_mask == ((UINT64_C(1) << 1) | (UINT64_C(1) << 63)));

    strcpy(line, "meshmapflags 0 0 0 0");
    for(i = 0; i < MAX_ARG_COUNT + 10; ++i) strcat(line, " collide");
    strcat(line, " walkable");
    assert(strlen(line) > MAX_ARG_LEN);
    assert(load_command(commands, line, error, sizeof(error)));
    assert(map->config.flags == (MESHMAP_COLLIDE | MESHMAP_WALKABLE));
    revision = map->revision;
    strcat(line, " typo");
    assert(!load_command(commands, line, error, sizeof(error)));
    assert(map->config.flags == (MESHMAP_COLLIDE | MESHMAP_WALKABLE));
    assert(map->revision == revision);

    strcpy(line, "MESHMAPFLAGS\t0 0 0 0 'DrAw' \"WaLkAbLe\" # all is a comment\nmeshmapflags 0 0 0 0 none");
    assert(load_command(commands, line, error, sizeof(error)));
    assert(map->config.flags == (MESHMAP_DRAW | MESHMAP_WALKABLE));
    strcpy(line, "meshmaprules 0 0 0 0 0 4294967295 45 -3");
    assert(load_command(commands, line, error, sizeof(error)));
    assert(map->config.side == MESHMAP_SIDE_NEGATIVE);
    assert(map->config.support_group == MESHMAP_GROUP_NONE);
    assert(map->config.slope_limit == 45 && map->config.layer == -3);
    assert(map->config.flags == (MESHMAP_DRAW | MESHMAP_WALKABLE));
    strcpy(line, "meshmaprules 0 0 0 0 2 4294967295 45 0");
    assert(!load_command(commands, line, error, sizeof(error)));

    meshmap_store_free(level->meshmaps);
    level = NULL;
    freeCommandList(commands);
    puts("meshmap level loader: full slot lists, long declarations, registration and atomic rejection passed");
    return 0;
}
