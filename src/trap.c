#include <arch/csr.h>
#include <arch/plic.h>
#include <arch/timer.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <kernel/serial.h>
#include <kernel/trap.h>
#include <kernel/types.h>

extern void trap_entry();

void handle_irq()
{
    u64 scause = csr_read(CSR_SCAUSE);

    if (scause == TRAP_TIMER_IRQ) {
        timer_irq();
        return;
    }

    if (scause == TRAP_EXTERNAL_IRQ) {
        u32 irq = plic_hart_claim_irq(0);

        if (irq == 0)
            return;

        if (irq == IRQ_SERIAL) {
            serial_irq();
        } else {
            warn("unexpected external irq %d\n", irq);
        }

        plic_hart_complete_irq(0, irq);
        return;
    }

    panic("unhandled interrupt: scause=%p sepc=%p stval=%p\n",
          scause,
          csr_read(CSR_SEPC),
          csr_read(CSR_STVAL));
}

void handle_exception()
{
    panic("unhandled exception: scause=%p sepc=%p stval=%p\n",
          csr_read(CSR_SCAUSE),
          csr_read(CSR_SEPC),
          csr_read(CSR_STVAL));
}

void trap_setup()
{
    hart_irq_disable();
    csr_write(CSR_STVEC, (u64)trap_entry);
}

void handle_trap()
{
    u64 scause = csr_read(CSR_SCAUSE);

    if (scause & TRAP_IRQ_BIT)
        handle_irq();
    else
        handle_exception();
}

void hart_irq_enable()
{
    csr_set(CSR_SSTATUS, CSR_SSTATUS_SIE);
}

u64 hart_irq_save()
{
    u64 flags = csr_read(CSR_SSTATUS) & CSR_SSTATUS_SIE;

    hart_irq_disable();

    return flags;
}

void hart_irq_restore(u64 flags)
{
    if (flags & CSR_SSTATUS_SIE)
        hart_irq_enable();
    else
        hart_irq_disable();
}

void hart_irq_disable()
{
    csr_clear(CSR_SSTATUS, CSR_SSTATUS_SIE);
}
