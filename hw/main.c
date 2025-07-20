#include "common.h"
#include "device.h"
#include "feature.h"

extern void systemc_dram_init(void);
extern void systemc_dram_write(uint32_t addr, uint32_t val);
extern uint32_t systemc_dram_read(uint32_t addr);

extern int semu_init(emu_state_t *, int, char **);
extern int semu_run(emu_state_t *);

int main(int argc, char **argv)
{
    systemc_dram_init();

    int ret;
    emu_state_t emu;
    ret = semu_init(&emu, argc, argv);
    if (ret)
        return ret;

    return semu_run(&emu);
}
