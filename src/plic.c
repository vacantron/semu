#include <stdlib.h>

#include "device.h"
#include "riscv.h"
#include "riscv_private.h"

#include "plic.h"

/* Make PLIC as simple as possible: 32 interrupts, no priority */

void plic_update_interrupts(vm_t *vm, plic_state_t *plic)
{
    /* Update pending interrupts */
    plic->ip |= plic->active & ~plic->masked;
    plic->masked |= plic->active;
    /* Send interrupt to target */
    for (uint32_t i = 0; i < vm->n_hart; i++) {
        if (plic->ip & plic->ie[i])
            vm->hart[i]->sip |= RV_INT_SEI_BIT;
        else
            vm->hart[i]->sip &= ~RV_INT_SEI_BIT;
    }
}

static bool plic_reg_read(plic_state_t *plic, uint32_t addr, uint32_t *value)
{
    /* no priority support: source priority hardwired to 1 */
    if (1 <= addr && addr <= 31)
        return true;

    if (addr == 0x400) {
        *value = plic->ip;
        return true;
    }

    int addr_mask = MASK(ilog2(addr)) ^ (1 & addr);
    int context = (addr_mask & addr);
    context >>= __builtin_ffs(context) - (__builtin_ffs(context) & 1);
    switch (addr & ~addr_mask) {
    case 0x800:
        *value = plic->ie[context];
        return true;
    case 0x80000:
        *value = 0;
        /* no priority support: target priority threshold hardwired to 0 */
        return true;
    case 0x80001:
        /* claim */
        *value = 0;
        uint32_t candidates = plic->ip & plic->ie[context];
        if (candidates) {
            *value = ilog2(candidates);
            plic->ip &= ~(1 << (*value));
        }
        return true;
    default:
        return false;
    }
}

static bool plic_reg_write(plic_state_t *plic, uint32_t addr, uint32_t value)
{
    /* no priority support: source priority hardwired to 1 */
    if (1 <= addr && addr <= 31)
        return true;

    int addr_mask = MASK(ilog2(addr)) ^ (1 & addr);
    int context = (addr_mask & addr);
    context >>= __builtin_ffs(context) - (__builtin_ffs(context) & 1);
    switch (addr & ~addr_mask) {
    case 0x800:
        value &= ~1;
        plic->ie[context] = value;
        return true;
    case 0x80000:
        /* no priority support: target priority threshold hardwired to 0 */
        return true;
    case 0x80001:
        /* completion */
        if (plic->ie[context] & (1 << value))
            plic->masked &= ~(1 << value);
        return true;
    default:
        return false;
    }
}

void plic_read(hart_t *vm,
               plic_state_t *plic,
               uint32_t addr,
               uint8_t width,
               uint32_t *value)
{
    switch (width) {
    case RV_MEM_LW:
        if (!plic_reg_read(plic, addr >> 2, value))
            vm_set_exception(vm, RV_EXC_LOAD_FAULT, vm->exc_val);
        break;
    case RV_MEM_LBU:
    case RV_MEM_LB:
    case RV_MEM_LHU:
    case RV_MEM_LH:
        vm_set_exception(vm, RV_EXC_LOAD_MISALIGN, vm->exc_val);
        return;
    default:
        vm_set_exception(vm, RV_EXC_ILLEGAL_INSN, 0);
        return;
    }
}

void plic_write(hart_t *vm,
                plic_state_t *plic,
                uint32_t addr,
                uint8_t width,
                uint32_t value)
{
    switch (width) {
    case RV_MEM_SW:
        if (!plic_reg_write(plic, addr >> 2, value))
            vm_set_exception(vm, RV_EXC_STORE_FAULT, vm->exc_val);
        break;
    case RV_MEM_SB:
    case RV_MEM_SH:
        vm_set_exception(vm, RV_EXC_STORE_MISALIGN, vm->exc_val);
        return;
    default:
        vm_set_exception(vm, RV_EXC_ILLEGAL_INSN, 0);
        return;
    }
}

static void _init(device_t *dev)
{
    dev->instance = (void *) &plic;
}

static void _read(device_t *dev,
                  hart_t *hart UNUSED,
                  uint32_t addr UNUSED,
                  uint32_t width UNUSED,
                  uint32_t *value UNUSED);

static void _update(device_t *dev, hart_t *hart)
{
    plic_t *plic = (plic_t *) dev->instance;
    if (plic->lock)
        goto update_hart_ip;

    for (uint32_t i = 0; i < MAX_PLIC_DEVICE; i++) {
        plic->gateway_ip[i] = plic->source_ip[i];
    }

update_hart_ip:
    uint32_t value = 0;
    _read(dev, hart, 0x1000, 4, &value);

    if (value)
        hart->sip |= RV_INT_SEI_BIT;
    else
        hart->sip &= ~(RV_INT_SEI_BIT);
}

static void _read(device_t *dev,
                  hart_t *hart UNUSED,
                  uint32_t addr UNUSED,
                  uint32_t width UNUSED,
                  uint32_t *value UNUSED)
{
    plic_t *plic = (plic_t *) dev->instance;
    addr &= 0x3ffffff;

    /* claim */
    if (addr == (0x200004 + 0x1000 * 0)) {
        /* get highest priority, clear source ip, lock gateway */
        uint32_t ip = 0;
        for (unsigned int i = 0; i < MAX_PLIC_DEVICE; i++) {
            if (plic->gateway_ip[i]) {
                ip = i;
                break;
            }
        }

        if (!ip) {
            *value = 0;
            return;
        }

        plic->source_ip[ip] = false;
        plic->gateway_ip[ip] = false;
        plic->claim = ip;
        plic->lock = true;

        *value = ip;
        return;
    }

    if (addr == (0x200000)) {
        *value = 0;
        return;
    }

    if (addr == 0x2000) {
        *value = plic->enable;
        return;
    }

    if (addr == 0x1000) {
        uint32_t val = 0;
        for (uint32_t i = 0; i < MAX_PLIC_DEVICE; i++) {
            if (!plic->gateway_ip[i])
                continue;
            val |= ((plic->gateway_ip[i] ? 1 : 0) << i);
        }
        *value = val;
        return;
    }
}

static void _write(device_t *dev UNUSED,
                   hart_t *hart UNUSED,
                   uint32_t addr UNUSED,
                   uint32_t width UNUSED,
                   uint32_t value UNUSED)
{
    plic_t *plic = (plic_t *) dev->instance;
    addr &= 0x3ffffff;

    /* completion */
    if (addr == (0x200004 + 0x1000 * 0)) {
        if (plic->claim != value)
            return;
        plic->lock = false;
        return;
    }

    if (addr == 0x2000) {
        plic->enable = (value & ~1u);
        return;
    }
}

/* TODO: remove hardcoded address, use libfdt */
static device_t dev = {.name = "plic0",
                       .init = _init,
                       .step = _update,
                       .read = _read,
                       .write = _write,
                       .addr_lo = 0,
                       .addr_hi = 0x4000000};

/* TODO: refine priority */
register_device(plic0, 116, &dev);
