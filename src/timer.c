#include <arch/timer.h>
#include <arch/csr.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <kernel/serial.h>

u64 timer_read()
{
    return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
    csr_set(CSR_SIE, (1UL << 5)); // enable timer interrupt

    csr_set(CSR_SSTATUS, (1UL << 1)); // enable global interrupts
}

void timer_irq_disable()
{
    csr_clear(CSR_SIE, (1UL << 5)); // disable timer interrupt
}

void timer_set_alarm(u64 secs)
{
    u64 now = timer_read();

    u64 target = now + (secs * TIMER_FREQ);
    csr_write(CSR_STIMECMP, target);

    timer_irq_enable();
}

void timer_irq()
{
    info("alarm\n");
    timer_irq_disable();
    serial_puts("> ");
}
