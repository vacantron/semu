#include "globals.h"

device_t *devices[MAX_PLIC_DEVICE];
unsigned int device_idx;

plic_t plic;

char *g_disk_file;
emu_state_t *g_emu;
