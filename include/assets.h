#ifndef ASSETS_H
#define ASSETS_H

#include <stdbool.h>
#include <stddef.h>

#define ASSET_PATH_MAX 1024

/**************************************************************************
 * PROTOTYPES
 **************************************************************************/

bool asset_find_path(char *out, size_t out_size, const char *relative_path);

#endif
