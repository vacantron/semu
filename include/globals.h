#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "device.h"
#include "plic.h"

extern device_t *devices[MAX_PLIC_DEVICE];
extern unsigned int device_idx;

extern plic_t plic;

/* TODO: remove these */
extern char *g_disk_file;
extern emu_state_t *g_emu;
