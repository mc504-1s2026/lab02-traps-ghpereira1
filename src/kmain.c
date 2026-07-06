#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

#define SHELL_LINE_SIZE 128
#define SHELL_READ_SIZE 256

static u64 alarm_deadline = 0;

static void put_u64_dec(u64 value)
{
    char buf[32];
    size_t i = 0;

    if (value == 0) {
        serial_putc('0');
        return;
    }

    while (value > 0 && i < sizeof(buf)) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0)
        serial_putc(buf[--i]);
}

static void shell_check_alarm()
{
    if (alarm_deadline != 0 && timer_read() >= alarm_deadline) {
        alarm_deadline = 0;
        serial_puts("alarm\n> ");
    }
}

static void shell_exec(char *line)
{
    while (*line == ' ')
        line++;

    if (strcmp(line, "uptime") == 0) {
        put_u64_dec(timer_read());
        serial_puts("s\n");
        return;
    }

    if (strcmp(line, "echo") == 0) {
        serial_puts("\n");
        return;
    }

    if (strncmp(line, "echo ", 5) == 0) {
        serial_puts(line + 5);
        serial_puts("\n");
        return;
    }

    if (strncmp(line, "alarm ", 6) == 0) {
        u64 delay = strtou64(line + 6, 10);
        alarm_deadline = timer_read() + delay;
        return;
    }

    if (line[0] != '\0')
        serial_puts("unknown command\n");
}

static void shell_run()
{
    char line[SHELL_LINE_SIZE];
    char input[SHELL_READ_SIZE];
    size_t line_len = 0;

    serial_puts("> ");

    while (1) {
        shell_check_alarm();

        size_t n = serial_read(input);

        for (size_t i = 0; i < n; i++) {
            char c = input[i];

            if (c == '\r') {
                serial_puts("\n");

                line[line_len] = '\0';
                shell_exec(line);
                line_len = 0;

                serial_puts("> ");
                continue;
            }

            if (c == '\n')
                continue;

            if (c == '\b' || c == 0x7f) {
                if (line_len > 0)
                    line_len--;
                continue;
            }

            if (line_len < SHELL_LINE_SIZE - 1) {
                line[line_len++] = c;
                serial_putc(c);
            }
        }
    }
}

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

	shell_run();
}
