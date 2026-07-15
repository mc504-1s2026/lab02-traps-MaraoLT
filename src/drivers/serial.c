#include <kernel/serial.h>
#include <kernel/panic.h>
#include <kernel/mm.h>
#include <kernel/trap.h>
#include <arch/csr.h>
#include <arch/spinlock.h>
#include <arch/plic.h>

#define UART_REG(r) ((volatile u8*)(SERIAL_BASE + (r)))
#define SERIAL_BUF_SIZE 1024

static char serial_buf[SERIAL_BUF_SIZE];
static size_t serial_head = 0;
static size_t serial_tail = 0;
static struct spinlock serial_lock;

void serial_init()
{
    *UART_REG(SERIAL_IER) = 0x00;
    *UART_REG(SERIAL_LCR) = 0x80;
    *UART_REG(0) = 0x01;
    *UART_REG(1) = 0x00;
    *UART_REG(SERIAL_LCR) = 0x03;
    *UART_REG(SERIAL_FCR) = 0x07;
    *UART_REG(SERIAL_IER) = SERIAL_IER_ERBFI;
}

void serial_irq_enable()
{
    plic_irq_set_priority(10, 1);
    plic_hart_set_threshold(0, 0);
    plic_hart_enable_irq(0, 10);

    csr_set(CSR_SIE, (1UL << 9)); // enable external interrupt
}

void serial_irq_disable()
{
    csr_clear(CSR_SIE, (1UL << 9)); // disable external interrupt
}

void serial_irq()
{
    spin_lock(&serial_lock);

    while (*UART_REG(5) & 0x01) {
        char c = *UART_REG(0);
        size_t next_head = (serial_head + 1) % SERIAL_BUF_SIZE;
        if (next_head != serial_tail) {
            serial_buf[serial_head] = c;
            serial_head = next_head;
        }
    }
    spin_unlock(&serial_lock);
}

size_t serial_read(char *buf)
{
    size_t count = 0;
    u64 flags = hart_irq_save();
    spin_lock(&serial_lock);

    while (serial_tail != serial_head) {
        buf[count++] = serial_buf[serial_tail];
        serial_tail = (serial_tail + 1) % SERIAL_BUF_SIZE;
    }
    spin_unlock(&serial_lock);
    hart_irq_restore(flags);

    return count;
}

void serial_puts(char *str)
{
    while (*str) {
        serial_putc(*str++);
    }
}

void serial_putc(char c)
{
    while (!(*UART_REG(5) & 0x20));
    *UART_REG(0) = (u8)c;
}
