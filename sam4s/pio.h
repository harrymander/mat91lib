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
#define PIO_BITMASK_(PIO) (BIT(PIO & 0x1F))


/** Private macro to lookup port register.  */
#define PIO_PORT_(PIO) (((PIO) >> 5) == PORT_A ? PIOA : ((PIO) >> 5) == PORT_B ? PIOB : PIOC)

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


typedef uint32_t pio_t;


/** Configure PIO
    @param pio  */
static inline
bool pio_config_set (pio_t pio, pio_config_t config)
{
    switch (config)
    {
    case PIO_OUTPUT_HIGH:
        PIO_PORT_ (pio)->PIO_SODR = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PER = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_OER = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PUDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_OUTPUT_LOW:
        PIO_PORT_ (pio)->PIO_CODR = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PER = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_OER = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PUDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_INPUT:
        PIO_PORT_ (pio)->PIO_ODR = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PER = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PUDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_PULLUP:
        PIO_PORT_ (pio)->PIO_ODR = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PER = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PUER = PIO_BITMASK_ (pio);
        return 1;

    case PIO_PERIPH_A:
        PIO_PORT_ (pio)->PIO_ABCDSR[0] &= ~PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_ABCDSR[1] &= ~PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PDR = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PUDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_PERIPH_B:
        PIO_PORT_ (pio)->PIO_ABCDSR[0] |= PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_ABCDSR[1] &= ~PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PDR = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PUDR = PIO_BITMASK_ (pio);
        return 1;

    case PIO_PERIPH_A_PULLUP:
        PIO_PORT_ (pio)->PIO_ABCDSR[0] &= ~PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_ABCDSR[1] &= ~PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PDR = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PUER = PIO_BITMASK_ (pio);
        return 1;

    case PIO_PERIPH_B_PULLUP:
        PIO_PORT_ (pio)->PIO_ABCDSR[0] &= ~PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_ABCDSR[1] |= PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PDR = PIO_BITMASK_ (pio);
        PIO_PORT_ (pio)->PIO_PUER = PIO_BITMASK_ (pio);
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
    PIO_PORT_ (pio)->PIO_SODR = PIO_BITMASK_ (pio);
}


/** Set PIO low.
    @param pio  */
static inline
void pio_output_low (pio_t pio)
{
    PIO_PORT_ (pio)->PIO_CODR = PIO_BITMASK_ (pio);
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
    return (PIO_PORT_ (pio)->PIO_ODSR & PIO_BITMASK_ (pio)) != 0;
}


/** Read input state of PIO.
    @param pio
    @return state  */
static inline
bool pio_input_get (pio_t pio)
{
    return (PIO_PORT_ (pio)->PIO_PDSR & PIO_BITMASK_ (pio)) != 0;
}


/** Toggle PIO.
    @param pio  */
static inline
void pio_output_toggle (pio_t pio)
{
    pio_output_get (pio) ? pio_output_low (pio) : pio_output_high (pio);
}


/** Enable the clock for the port.  This is required for input
    operations.  FIXME for PIOB.  */
#define pio_init(pio) \
    PMC->PMC_PCER = BIT (AT91C_ID_PIOA)


/** Disable the clock for the port.  FIXME for PIOB.  */
#define pio_shutdown(pio) \
    PMC->PMC_PCDR = BIT (AT91C_ID_PIOA)

#endif