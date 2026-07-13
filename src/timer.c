#include <arch/timer.h>
#include <arch/csr.h>

u64 timer_read()
{
    return csr_read(CSR_TIME) / TIMER_FREQ;
}

void timer_irq_enable()
{
    timer_set_alarm(1);
    csr_set(CSR_SIE, CSR_SIE_STIE);
}

void timer_irq_disable()
{
    csr_clear(CSR_SIE, CSR_SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
    u64 now = csr_read(CSR_TIME);
    csr_write(CSR_STIMECMP, now + secs * TIMER_FREQ);
}

void timer_irq()
{
    timer_set_alarm(1);
}
