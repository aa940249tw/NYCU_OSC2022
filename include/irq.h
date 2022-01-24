#ifndef __IRQ_H__
#define __IRQ_H__

#define IRQ_BASE 0x3F00B000
#define IRQ_BASIC_PENDING       ((volatile unsigned int*)(IRQ_BASE + 0x204))
#define IRQ_ENABLE_1            ((volatile unsigned int*)(IRQ_BASE + 0x210))
#define IRQ_DISABLE_1           ((volatile unsigned int*)(IRQ_BASE + 0x21C))
#define AUX_INT (1 << 29)

#endif