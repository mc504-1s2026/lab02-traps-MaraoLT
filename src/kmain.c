#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

extern int _hartid[];
void kmain()
{
	printk_set_level(LOG_DEBUG);
	info("entered S-mode\n");
	info("booting on hart %d\n", _hartid[0]);
	info("setting up virtual memory...\n");
	vm_init();

	info("enabling traps...\n");
	trap_setup();
	info("enabling timer...\n");
	timer_irq_enable();
	info("enabling serial...\n");
	serial_init();
	serial_irq_enable();

    hart_irq_enable();

	/* implement your shell here */
    char cmdbuffer[128];
    size_t cmdlen = 0;
    serial_puts("> ");
    while (1) {
        char temp[16];

        size_t n = serial_read(temp);

        for (size_t i = 0; i < n; i++) {
            char c = temp[i];

            serial_putc(c); // echo the character back to the serial output

            if (c == '\r' || c == '\n') {
                serial_putc('\n');
                cmdbuffer[cmdlen] = '\0'; // null-terminate the command string

                if (strncmp(cmdbuffer, "uptime", 6) == 0) { // uptime
                    u64 sec = timer_read() / TIMER_FREQ;
                    info("%llus\n", sec);
                } else if (strncmp(cmdbuffer, "echo ", 5) == 0) { // echo
                    info("%s\n", cmdbuffer + 5);
                } else if (strncmp(cmdbuffer, "alarm ", 6) == 0) {
                    u64 sec = 0;
                    for (int j = 6; cmdbuffer[j] != '\0'; j++) {
                        if (cmdbuffer[j] >= '0' && cmdbuffer[j] <= '9') {
                            sec = (sec * 10) + (cmdbuffer[j] - '0');
                        }
                    }
                    if (sec > 0) timer_set_alarm(sec);
                }
                cmdlen = 0;
                serial_puts("> ");
            } else {
                if (cmdlen < sizeof(cmdbuffer) -1) {
                    cmdbuffer[cmdlen++] = c;
                }
            }

        }
    }
}
