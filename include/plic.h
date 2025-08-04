#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MAX_PLIC_DEVICE 32

/* assume we have only one hart and only have a sucessful claim for hart0 now */
typedef struct {
    bool source_ip[MAX_PLIC_DEVICE];
    bool gateway_ip[MAX_PLIC_DEVICE];
    uint32_t claim;
    bool lock;
    uint32_t enable;
} plic_t;
