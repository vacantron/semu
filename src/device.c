#include "device.h"
#include "plic.h"

void devices_init()
{
    /* TODO: equip libfdt */
    for (unsigned int i = 0; i < device_idx; i++) {
        device_t *dev = devices[i];
        dev->init(dev);
        dev->intr_notifier = &plic.source_ip[i + 1];
    }
}

void devices_step(hart_t *hart)
{
    for (unsigned int i = 0; i < device_idx; i++) {
        device_t *dev = devices[i];
        dev->step(dev, hart);
    }
}

bool devices_load(hart_t *hart UNUSED,
                  uint32_t addr UNUSED,
                  uint32_t width UNUSED,
                  uint32_t *value UNUSED)
{
    for (unsigned int i = 0; i < device_idx; i++) {
        device_t *dev = devices[i];

        if (!device_addr_in_range(dev, addr & ~(0xf0000000)))
            continue;

        dev->read(dev, hart, addr & ~(0xf0000000), width, value);

        /* assume no overlapping */
        return true;
    }

    return false;
}

bool devices_store(hart_t *hart UNUSED,
                   uint32_t addr UNUSED,
                   uint32_t width UNUSED,
                   uint32_t value UNUSED)
{
    for (unsigned int i = 0; i < device_idx; i++) {
        device_t *dev = devices[i];

        if (!device_addr_in_range(dev, addr & ~(0xf0000000)))
            continue;

        dev->write(dev, hart, addr & ~(0xf0000000), width, value);

        /* assume no overlapping */
        return true;
    }

    return false;
}

bool device_addr_in_range(device_t *dev, uint32_t addr)
{
    if (addr < dev->addr_lo || addr >= dev->addr_hi)
        return false;

    return true;
}
