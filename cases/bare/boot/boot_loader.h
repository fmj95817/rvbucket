#ifndef BOOT_LOADER_H
#define BOOT_LOADER_H

#include "boot_image.h"
#include "boot_media.h"

extern int boot_load_image(boot_media_t *media, rvb_bin_header_t *header);

#endif
