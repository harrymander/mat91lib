/** @file   pio.h
    @author M. P. Hayes, UCECE
    @date   2 Jun 2007
    @brief  PIO abstraction for SAM4S microcontroller.
    @note   Macros and inline functions are used to avoid function
            call overhead and to allow compile-time constant folding. 
*/
#ifndef PIO_H
#define PIO_H

#include "config.h"
#include "mcu.h"


#define PIO_SAM4S

/* The SAM4S PIO lines are much more configurable than the SAM7.  They
   also can be configured open-drain (multi-drive), have optional
   Schmitt trigger inputs, have internal pulldown resistors, and can
   share the pin with four peripheral signals.  These options are not
   supported.  */


/* The 64 pin MCUs have two PIO ports; the 100 pin ones have three.  */
enum {PORT_A, PORT_B, PORT_C};

/* Enumerate all the PIOs.  */
#define PIO_DEFINE(PORT, PORTBIT) (((PORT) << 5) + (PORTBIT))


/** Private macro to lookup bitmask.  */
#define PIO_BITMASK_(PIO) (BIT((PIO) & 0x1F))


/** Private macro to lookup PIO controller base address.  */
#define PIO_PORT(PIO) ((PIO) >> 5)


/** Private macro to lookup PIO controller base address.  */
#define PIO_BASE(PIO) (PIO_PORT (PIO) == PORT_A ? PIOA : PIO_PORT (PIO) == PORT_B ? PIOB : PIOC)


/** Private macro to lookup PIO controller ID.  */
#define PIO_ID(PIO) (PIO_PORT (PIO) == PORT_A ? ID_PIOA : PIO_PORT (PIO) == PORT_B ? ID_PIOB : ID_PIOC)


typedef uint32_t pio_mask_t;

typedef enum pio_config_enum 
{
    PIO_INPUT = 1,          /* Configure as input pin.  */
    PIO_PULLUP,             /* Configure as input pin with pullup.  */
    PIO_PULLDOWN,           /* Configure as input pin with pulldown.  */
    PIO_OUTPUT_LOW,         /* Configure as output, initially low.  */
    PIO_OUTPUT_HIGH,        /* Configure as output, initially high.  */
    PIO_PERIPH_A,
    PIO_PERIPH = PIO_PERIPH_A,
    PIO_PERIPH_B,
    PIO_PERIPH_A_PULLUP,
    PIO_PERIPH_PULLUP = PIO_PERIPH_A_PULLUP,
    PIO_PERIPH_B_PULLUP
} pio_config_t;


/** Define the pins.  */
#define PA0_PIO PIO_DEFINE(PORT_A, 0)
#define PA1_PIO PIO_DEFINE(PORT_A, 1)
#define PA2_PIO PIO_DEFINE(PORT_A, 2)
#define PA3_PIO PIO_DEFINE(PORT_A, 3)
#define PA4_PIO PIO_DEFINE(PORT_A, 4)
#define PA5_PIO PIO_DEFINE(PORT_A, 5)
#define PA6_PIO PIO_DEFINE(PORT_A, 6)
#define PA7_PIO PIO_DEFINE(PORT_A, 7)
#define PA8_PIO PIO_DEFINE(PORT_A, 8)
#define PA9_PIO PIO_DEFINE(PORT_A, 9)
#define PA10_PIO PIO_DEFINE(PORT_A, 10)
#define PA11_PIO PIO_DEFINE(PORT_A, 11)
#define PA12_PIO PIO_DEFINE(PORT_A, 12)
#define PA13_PIO PIO_DEFINE(PORT_A, 13)
#define PA14_PIO PIO_DEFINE(PORT_A, 14)
#define PA15_PIO PIO_DEFINE(PORT_A, 15)
#define PA16_PIO PIO_DEFINE(PORT_A, 16)
#define PA17_PIO PIO_DEFINE(PORT_A, 17)
#define PA18_PIO PIO_DEFINE(PORT_A, 18)
#define PA19_PIO PIO_DEFINE(PORT_A, 19)
#define PA20_PIO PIO_DEFINE(PORT_A, 20)
#define PA21_PIO PIO_DEFINE(PORT_A, 21)
#define PA22_PIO PIO_DEFINE(PORT_A, 22)
#define PA23_PIO PIO_DEFINE(PORT_A, 23)
#define PA24_PIO PIO_DEFINE(PORT_A, 24)
#define PA25_PIO PIO_DEFINE(PORT_A, 25)
#define PA26_PIO PIO_DEFINE(PORT_A, 26)
#define PA27_PIO PIO_DEFINE(PORT_A, 27)
#define PA28_PIO PIO_DEFINE(PORT_A, 28)
#define PA29_PIO PIO_DEFINE(PORT_A, 29)
#define PA30_PIO PIO_DEFINE(PORT_A, 30)
#define PA31_PIO PIO_DEFINE(PORT_A, 31)

#define PB0_PIO PIO_DEFINE(PORT_B, 0)
#define PB1_PIO PIO_DEFINE(PORT_B, 1)
#define PB2_PIO PIO_DEFINE(PORT_B, 2)
#define PB3_PIO PIO_DEFINE(PORT_B, 3)
#define PB4_PIO PIO_DEFINE(PORT_B, 4)
#define PB5_PIO PIO_DEFINE(PORT_B, 5)
#define PB6_PIO PIO_DEFINE(PORT_B, 6)
#define PB7_PIO PIO_DEFINE(PORT_B, 7)
#define PB8_PIO PIO_DEFINE(PORT_B, 8)
#define PB9_PIO PIO_DEFINE(PORT_B, 9)
#define PB10_PIO PIO_DEFINE(PORT_B, 10)
#define PB11_PIO PIO_DEFINE(PORT_B, 11)
#define PB12_PIO PIO_DEFINE(PORT_B, 12)
#define PB13_PIO PIO_DEFINE(PORT_B, 13)
#define PB14_PIO PIO_DEFINE(PORT_B, 14)
#define PB15_PIO PIO_DEFINE(PORT_B, 15)
#define PB16_PIO PIO_DEFINE(PORT_B, 16)
#define PB17_PIO PIO_DEFINE(PORT_B, 17)
#define PB18_PIO PIO_DEFINE(PORT_B, 18)
#define PB19_PIO PIO_DEFINE(PORT_B, 19)
#define PB20_PIO PIO_DEFINE(PORT_B, 20)
#define PB21_PIO PIO_DEFINE(PORT_B, 21)
#define PB22_PIO PIO_DEFINE(PORT_B, 22)
#define PB23_PIO PIO_DEFINE(PORT_B, 23)
#define PB24_PIO PIO_DEFINE(PORT_B, 24)
#define PB25_PIO PIO_DEFINE(PORT_B, 25)
#define PB26_PIO PIO_DEFINE(PORT_B, 26)
#define PB27_PIO PIO_DEFINE(PORT_B, 27)
#define PB28_PIO PIO_DEFINE(PORT_B, 28)
#define PB29_PIO PIO_DEFINE(PORT_B, 29)
#define PB30_PIO PIO_DEFINE(PORT_B, 30)
#define PB31_PIO PIO_DEFINE(PORT_B, 31)


#define MOSI0_PIO PA13_PIO
#define MISO0_PIO PA12_PIO
#define SPCK0_PIO PA14_PIO

#define RD_PIO PA18_PIO
#define RK_PIO PA19_PIO
#define RF_PIO PA20_PIO

#define TD_PIO PA17_PIO
#define TK_PIO PA16_PIO
#define TF_PIO PA15_PIO


typedef uint32_t pio_t;


/** Configure PIO
    @param pio  */
static inline bool
pio_config_set (pio_t pio, pio_config_t config)
{
    switch (config)
    {
    case PIO_OUTPUT_HIGH:
        PIO_BASE (pio)->PIO_SODR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PER = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_OER = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PUDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_OUTPUT_LOW:
        PIO_BASE (pio)->PIO_CODR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PER = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_OER = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PUDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_INPUT:
        PIO_BASE (pio)->PIO_ODR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PER = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PUDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_PULLUP:
        PIO_BASE (pio)->PIO_ODR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PER = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PUER = PIO_BITMASK_ (pio);
        return 1;

    case PIO_PERIPH_A:
        PIO_BASE (pio)->PIO_ABCDSR[0] &= ~PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_ABCDSR[1] &= ~PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PDR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PUDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_PERIPH_B:
        PIO_BASE (pio)->PIO_ABCDSR[0] |= PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_ABCDSR[1] &= ~PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PDR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PUDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_PERIPH_A_PULLUP:
        PIO_BASE (pio)->PIO_ABCDSR[0] &= ~PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_ABCDSR[1] &= ~PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PDR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PUER = PIO_BITMASK_ (pio);
        return 1;

    case PIO_PERIPH_B_PULLUP:
        PIO_BASE (pio)->PIO_ABCDSR[0] &= ~PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_ABCDSR[1] |= PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PDR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_PUER = PIO_BITMASK_ (pio);
        return 1;

    default:
        return 0;
    }
}


/** Set PIO high.
    @param pio  */
static inline
void pio_output_high (pio_t pio)
{
    PIO_BASE (pio)->PIO_SODR = PIO_BITMASK_ (pio);
}


/** Set PIO low.
    @param pio  */
static inline
void pio_output_low (pio_t pio)
{
    PIO_BASE (pio)->PIO_CODR = PIO_BITMASK_ (pio);
}


/** Set PIO to desired state.
    @param pio 
    @param state  */
static inline
void pio_output_set (pio_t pio, bool state)
{
    state ? pio_output_high (pio) : pio_output_low (pio);
}


/** Get output state of PIO.
    @param pio
    @return state  */
static inline
bool pio_output_get (pio_t pio)
{
    return (PIO_BASE (pio)->PIO_ODSR & PIO_BITMASK_ (pio)) != 0;
}


/** Read input state of PIO.
    @param pio
    @return state  */
static inline
bool pio_input_get (pio_t pio)
{
    return (PIO_BASE (pio)->PIO_PDSR & PIO_BITMASK_ (pio)) != 0;
}


/** Toggle PIO.
    @param pio  */
static inline
void pio_output_toggle (pio_t pio)
{
    pio_output_get (pio) ? pio_output_low (pio) : pio_output_high (pio);
}


/** Enable the clock for the port.  This is required for input
    operations.  */
static inline
void pio_init (pio_t pio)
{
    mcu_pmc_enable (PIO_ID (pio));
}


/** Disable the clock for the port.  */
static inline
void pio_shutdown (pio_t pio)
{
    mcu_pmc_disable (PIO_ID (pio));
}



typedef enum pio_irq_config_enum 
{
    PIO_IRQ_FALLING_EDGE = 1, 
    PIO_IRQ_RISING_EDGE, 
    PIO_IRQ_ANY_EDGE, 
    PIO_IRQ_LOW_LEVEL, 
    PIO_IRQ_HIGH_LEVEL
} pio_irq_config_t;


/** Configure PIO for interrupt
    @param pio  */
static inline bool 
pio_irq_config_set (pio_t pio, pio_irq_config_t config)
{

    /* The PIO Controller can be programmed to generate an interrupt
       when it detects an edge or a level on an I/O line, regardless
       of how it is configured: input, output, peripheral, etc.

       For input change detection, the PIO controller clock must be
       enabled.  */

    switch (config)
    {
    case PIO_IRQ_FALLING_EDGE:
        PIO_BASE (pio)->PIO_ESR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_FELLSR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_AIMDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_IRQ_RISING_EDGE:
        PIO_BASE (pio)->PIO_ESR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_REHLSR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_AIMDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_IRQ_ANY_EDGE:
        PIO_BASE (pio)->PIO_AIMER = PIO_BITMASK_ (pio);
        return 1;

    case PIO_IRQ_LOW_LEVEL:
        PIO_BASE (pio)->PIO_LSR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_FELLSR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_AIMDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_IRQ_HIGH_LEVEL:
        PIO_BASE (pio)->PIO_LSR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_REHLSR = PIO_BITMASK_ (pio);
        PIO_BASE (pio)->PIO_AIMDR = PIO_BITMASK_ (pio);
        return 1;

    default:
        return 0;
    }
}


static inline void
pio_irq_enable (pio_t pio)
{
    PIO_BASE (pio)->PIO_IER = PIO_BITMASK_ (pio);
}


static inline void
pio_irq_disable (pio_t pio)
{
    PIO_BASE (pio)->PIO_IDR = PIO_BITMASK_ (pio);
}


#endif
