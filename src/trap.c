#include <kernel/trap.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <kernel/serial.h>
#include <arch/csr.h>
#include <arch/timer.h>
#include <arch/plic.h>

/* defined in src/trap_entry.S */
extern void trap_entry();

void handle_irq()
{
    u64 scause = csr_read(CSR_SCAUSE);
    u64 code = scause & ~(1UL << 63);

    if (code == 5) {
        timer_irq();
    } else if (code == 9) {
        u32 irq = plic_hart_claim_irq(0);

        if (irq == 10) {
            serial_irq();
        }
        if (irq != 0) {
            plic_hart_complete_irq(0, irq);
        }
    } else {
        warn("Unknown interrupt occurred! scause: 0x%lx, code: %lu\n", scause, code);
    }
}

void handle_exception()
{
    u64 scause = csr_read(CSR_SCAUSE);
    u64 stval = csr_read(CSR_STVAL);
    u64 sepc = csr_read(CSR_SEPC);

    u64 code = scause & ~(1UL << 63);

    error("Exception occurred! scause: 0x%lx, stval: 0x%lx, sepc: 0x%lx, code: %lu\n", scause, stval, sepc, code);

    BUG();
}

void trap_setup()
{
	csr_write(CSR_STVEC, (u64)trap_entry);
}

void handle_trap()
{
	u64 scause = csr_read(CSR_SCAUSE);
    int is_interrupt = scause >> 63;

    if (is_interrupt) {
        handle_irq();
    } else {
        handle_exception();
    }
}

void hart_irq_enable()
{
    u64 sstatus = csr_read(CSR_SSTATUS);
    csr_write(CSR_SSTATUS, sstatus | (1UL << 1));
}

u64 hart_irq_save()
{
    u64 sstatus = csr_read(CSR_SSTATUS);
    csr_write(CSR_SSTATUS, sstatus & ~(1UL << 1));

    return sstatus;
}

void hart_irq_restore(u64 flags)
{
    u64 sstatus = csr_read(CSR_SSTATUS);
    if (flags & (1UL << 1)) {
        sstatus |= (1UL << 1);
    } else {
        sstatus &= ~(1UL << 1);
    }
    csr_write(CSR_SSTATUS, sstatus);
}

void hart_irq_disable()
{
    u64 sstatus = csr_read(CSR_SSTATUS);
    csr_write(CSR_SSTATUS, sstatus & ~(1UL << 1));
}
