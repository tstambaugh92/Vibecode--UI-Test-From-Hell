#include "assets.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ASSET_DATA_ENV
#define ASSET_DATA_ENV "VIBECODE_DATA_DIR"
#endif

#ifndef ASSET_SYSTEM_DATA_DIR
#define ASSET_SYSTEM_DATA_DIR "/usr/share/vibecode_ui_test_from_hell"
#endif

/**************************************************************************
 * FILE: assets.c
 *
 * Asset path resolver for media/data files. The app may be run from the repo,
 * from bin/, from an AppImage, or from a system install; this module keeps
 * those location rules centralized so audio/images/video can all use the same
 * lookup behavior.
 **************************************************************************/

/**************************************************************************
 * file_exists
 *
 * Purpose:
 *   Test whether a path names a readable file using only standard C I/O.
 *
 * Input:
 *   path - candidate filesystem path
 *
 * Output:
 *   bool - true when fopen succeeds
 **************************************************************************/
static bool file_exists(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;
    fclose(fp);
    return true;
}

/**************************************************************************
 * copy_if_readable
 *
 * Purpose:
 *   Copy a candidate path to the caller's output buffer if it is readable.
 *
 * Input:
 *   out      - destination buffer
 *   out_size - destination buffer size
 *   path     - candidate path
 *
 * Output:
 *   bool - true when path was readable and fit in out
 **************************************************************************/
static bool copy_if_readable(char *out, size_t out_size, const char *path) {
    if (!out || out_size == 0 || !path || !file_exists(path)) return false;

    int written = snprintf(out, out_size, "%s", path);
    return written >= 0 && (size_t)written < out_size;
}

/**************************************************************************
 * try_joined_path
 *
 * Purpose:
 *   Join a base directory and relative asset path, then test readability.
 *   A slash is inserted only when the base path does not already end in one.
 *
 * Input:
 *   out           - destination buffer
 *   out_size      - destination buffer size
 *   base          - candidate asset root directory
 *   relative_path - asset path below that root
 *
 * Output:
 *   bool - true when a readable joined path was found
 **************************************************************************/
static bool try_joined_path(char *out, size_t out_size, const char *base, const char *relative_path) {
    if (!base || base[0] == '\0') return false;

    char candidate[ASSET_PATH_MAX];
    size_t base_len = strlen(base);
    const char *sep = (base_len > 0 && (base[base_len - 1] == '/' || base[base_len - 1] == '\\')) ? "" : "/";
    int written = snprintf(candidate, sizeof(candidate), "%s%s%s", base, sep, relative_path);
    if (written < 0 || (size_t)written >= sizeof(candidate)) return false;

    return copy_if_readable(out, out_size, candidate);
}

/**************************************************************************
 * asset_find_path
 *
 * Purpose:
 *   Resolve a project-relative asset path into a readable filesystem path.
 *
 * Search order:
 *   1. Absolute paths passed directly by a caller
 *   2. $VIBECODE_DATA_DIR/<relative_path> for AppImage/custom launches
 *   3. ./<relative_path> for running from the project root
 *   4. ../<relative_path> for running from one directory below the root
 *   5. SDL_GetBasePath()/<relative_path> for assets beside the executable
 *   6. SDL_GetBasePath()/../<relative_path> for bin/app launched elsewhere
 *   7. /usr/share/vibecode_ui_test_from_hell/<relative_path> for installs
 *
 * Input:
 *   out           - destination buffer for resolved path
 *   out_size      - size of destination buffer
 *   relative_path - asset path relative to the project/data root
 *
 * Output:
 *   bool - true when a readable file was found and copied into out
 **************************************************************************/
bool asset_find_path(char *out, size_t out_size, const char *relative_path) {
    if (!out || out_size == 0 || !relative_path || relative_path[0] == '\0') return false;

    if (relative_path[0] == '/' || relative_path[0] == '\\') {
        return copy_if_readable(out, out_size, relative_path);
    }

    const char *data_dir = getenv(ASSET_DATA_ENV);
    if (try_joined_path(out, out_size, data_dir, relative_path)) return true;

    if (copy_if_readable(out, out_size, relative_path)) return true;

    char parent_candidate[ASSET_PATH_MAX];
    int written = snprintf(parent_candidate, sizeof(parent_candidate), "../%s", relative_path);
    if (written >= 0 && (size_t)written < sizeof(parent_candidate)) {
        if (copy_if_readable(out, out_size, parent_candidate)) return true;
    }

    const char *base_path = SDL_GetBasePath();
    if (try_joined_path(out, out_size, base_path, relative_path)) return true;

    char exe_parent[ASSET_PATH_MAX];
    written = snprintf(exe_parent, sizeof(exe_parent), "%s../%s", base_path ? base_path : "", relative_path);
    if (base_path && written >= 0 && (size_t)written < sizeof(exe_parent)) {
        if (copy_if_readable(out, out_size, exe_parent)) return true;
    }

    return try_joined_path(out, out_size, ASSET_SYSTEM_DATA_DIR, relative_path);
}
