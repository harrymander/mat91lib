/** @file   spi.c
    @author Michael Hayes
    @date   30 July 2007
    @brief  Routines for interfacing to the SPI bus.
*/

#include "config.h"
#include "spi.h"
#include "bits.h"

/* The AT91 SPI peripheral can transmit either 8 or 16 bit data.  It
   has 4 separate chip select registers and can control 4 different
   types of SPI peripheral using the separate chip select registers.
   Each peripheral can have its own chip select that is automatically
   driven for each transfer.

   The AT91 SPI peripheral has been designed so that blocks of a data
   can be streamed using DMA to multiple devices on the bus (Variable
   Peripheral Selection).  It achieves this by using 32 bits to
   specify the 8 or 16 bit data plus a bit mask specifying which
   peripheral to write to.

   There are 4 control status registers (CSRs); one associated with
   each of the four SPI chip select (CS) signals.  These configure the
   number of bits per transfer and the CS timing for each of the 4
   channels.  The SPI controller choses one of these 4 CSRs on the
   basis of the channel select mask in the master register (fixed
   peripheral mode) or in the data (variable peripheral mode).  Now
   this is all fine and dandy if we have 4 SPI devices.  What if we
   want more?  Well one option is to decode the 4 CS signals to
   provide 15 channels; with four per group.  This requires external
   hardware so is not much use.  The alternative is to bit bash
   additional output ports. 

   With this driver we can share the 4 channels among multiple SPI
   devices.  However, automatic CS driving can only be performed for
   the few NPCS signals associated with a channel, otherwise the CS
   signals are bit-bashed.  There are two chip select modes: FRAME
   where the CS is asserted for multiple SPI tranmissions; and TOGGLE
   where the CS is only asserted for each SPI transmission.  The
   functions that configure the SPI peripheral (such as spi_bits_set)
   only take effect when spi_config is called (usually when some I/O
   is to be performed).
*/


#ifndef SPI_DEVICES_NUM
#define SPI_DEVICES_NUM 1
#endif


#ifndef HOSTED
#define HOSTED 0
#endif


/* Bit masks for the SPI mode settings.  */
enum 
{
    SPI_CPOL_MASK = BIT (0),
    SPI_NCPHA_MASK = BIT (1), 
    SPI_CSAAT_MASK = BIT (3)
};


/* Macros.  */


/* Read SPI and then send new data.  Blocks while the new data is
   being sent by the spi.  */
#define SPI_XFER(txdata, rxdata)                                        \
    do                                                                  \
    {                                                                   \
        AT91S_SPI *pSPI = AT91C_BASE_SPI;                               \
                                                                        \
        /* Dummy read from RDR to ensure RDRF clear.   */               \
        (rxdata) = pSPI->SPI_RDR;                                       \
                                                                        \
        /* Write data to TDR.  */                                       \
        pSPI->SPI_TDR = (txdata);                                       \
                                                                        \
        /* Wait until spi port finishes transmitting/receiving.  */     \
        while (!SPI_READY_P())                                          \
            continue;                                                   \
                                                                        \
        /* Read new data from RDR (this clears RDRF).  */               \
        (rxdata) = pSPI->SPI_RDR;                                       \
                                                                        \
    } while (0)


static uint8_t spi_devices_num = 0;
static spi_dev_t spi_devices[SPI_DEVICES_NUM];
static spi_dev_t *spi_config_last = 0;
static uint32_t spi_devices_enabled = 0;


static inline uint32_t
spi_channel_csr_get (spi_channel_t channel)
{
    return AT91C_BASE_SPI->SPI_CSR[channel];
}


static inline void
spi_channel_csr_set (spi_channel_t channel, uint32_t csr)
{
    AT91C_BASE_SPI->SPI_CSR[channel] = csr;
}


/** Set the delay (in clocks * 32) before starting a transmission on
    same SPI channel.  The default is 0.  */
static void
spi_channel_delay_set (spi_channel_t channel, uint16_t delay)
{
    uint32_t csr;

    csr = spi_channel_csr_get (channel);

    /* Set the DLYBCT (delay between consecutive transfers).  This is
       the only mechanism for automatically delaying the automatic
       deassertion of the chip select.  */
    BITS_INSERT (csr, delay, 24, 31);

    spi_channel_csr_set (channel, csr);
}


/** Set the delay (in clocks) before the clock starts.  */
static void
spi_channel_clock_delay_set (spi_channel_t channel, uint16_t delay)
{
    uint32_t csr;

    csr = spi_channel_csr_get (channel);

    /* Set the DLYBS (delay before SPCK).  */
    BITS_INSERT (csr, delay, 16, 23);

    spi_channel_csr_set (channel, csr);
}


/** The minimum divisor value is 1, this gives the maximum rate of MCK.  */
static void
spi_channel_clock_divisor_set (spi_channel_t channel,
                               spi_clock_divisor_t clock_divisor)
{
    uint32_t csr;

    csr = spi_channel_csr_get (channel);

    BITS_INSERT (csr, clock_divisor, 8, 15);

    spi_channel_csr_set (channel, csr);
}


/* Set number of bits in transfer 8--16.  There is a silicon bug
   (39.2.4.5) where the number of bits cannot be odd if the SPI
   divisor is 1.  */
static void
spi_channel_bits_set (spi_channel_t channel, uint8_t bits)
{
    uint32_t csr;

    csr = spi_channel_csr_get (channel);

    BITS_INSERT (csr, bits - 8, 4, 7);

    spi_channel_csr_set (channel, csr);
}


/* Spi modes:
Mode 	CPOL 	CPHA  NCPHA
0 	0 	0     1       clock normally low    read on rising edge
1 	0 	1     0       clock normally low    read on falling edge
2 	1 	0     1       clock normally high   read on falling edge
3 	1 	1     0       clock normally high   read on rising edge

However, page 512 of the AT91SAM7Sx datasheet say "Note that in SPI
master mode the ATSAM7S512/256/128/64/321/32 does not sample the data
(MISO) on the opposite edge where data clocks out (MOSI) but the same
edge is used as shown in Figure 36-3 and Figure 36-4."  Figure 36-3
shows that CPOL=NCPHA=0 or CPOL=NCPHA=1 samples on the rising edge and
that the data changes sometime after the rising edge (about 2 ns).  To
be consistent with normal SPI operation, it is probably safe to say
that the data changes on the falling edge and should be sampled on the
rising edge.  Therefore, it appears that NCPHA should be treated the
same as CPHA.  Thus:

Mode 	CPOL 	CPHA  NCPHA
0 	0 	0     0       clock normally low    read on rising edge
1 	0 	1     1       clock normally low    read on falling edge
2 	1 	0     0       clock normally high   read on falling edge
3 	1 	1     1       clock normally high   read on rising edge
*/

static void
spi_channel_mode_set (spi_channel_t channel, spi_mode_t mode)
{
    uint32_t csr;

    csr = spi_channel_csr_get (channel);

    csr &= ~(SPI_CPOL_MASK | SPI_NCPHA_MASK);

    switch (mode)
    {
    case SPI_MODE_0:
        /* CPOL = 0, CPHA = 0.  */
        csr |= SPI_NCPHA_MASK;
        break;

    case SPI_MODE_1:
        /* CPOL = 0, CPHA = 1.  */
        break;

    case SPI_MODE_2:
        /* CPOL = 1, CPHA = 0.  */
        csr |= SPI_CPOL_MASK;
        csr |= SPI_NCPHA_MASK;
        break;

    case SPI_MODE_3:
        /* CPOL = 1, CPHA = 1.  */
        csr |= SPI_CPOL_MASK;
        break;
    }

    spi_channel_csr_set (channel, csr);
}


static void
spi_channel_cs_mode_set (spi_channel_t channel, spi_cs_mode_t mode)
{
    uint32_t csr;

    csr = spi_channel_csr_get (channel) & ~SPI_CSAAT_MASK;

    /* If framing with chip select then enable chip select active
       after transmission (CSAAT).  */
    if (mode == SPI_CS_MODE_FRAME)
        csr |= SPI_CSAAT_MASK;

    spi_channel_csr_set (channel, csr);
}


static bool
spi_channel_cs_enable (spi_channel_t channel, port_mask_t bitmask)
{
    switch (channel)
    {
    case 0:
        if (bitmask == BIT (11))
        {
            *AT91C_PIOA_ASR = AT91C_PA11_NPCS0;
            *AT91C_PIOA_PDR = AT91C_PA11_NPCS0;
            return 1;
        }
        break;

    case 1:
        if (bitmask == BIT (9))
        {
            *AT91C_PIOA_BSR = AT91C_PA9_NPCS1;
            *AT91C_PIOA_PDR = AT91C_PA9_NPCS1;
            return 1;
        }
        else if (bitmask == BIT (31))
        {
            *AT91C_PIOA_ASR = AT91C_PA31_NPCS1;
            *AT91C_PIOA_PDR = AT91C_PA31_NPCS1;
            return 1;
        }
        break;

    case 2:
        if (bitmask == BIT (10))
        {
            *AT91C_PIOA_BSR = AT91C_PA10_NPCS2;
            *AT91C_PIOA_PDR = AT91C_PA10_NPCS2;
            return 1;
        }
        else if (bitmask == BIT (30))
        {
            *AT91C_PIOA_BSR = AT91C_PA30_NPCS2;
            *AT91C_PIOA_PDR = AT91C_PA30_NPCS2;
            return 1;
        }
        break;

    case 3:
        if (bitmask == BIT (3))
        {
            *AT91C_PIOA_BSR = AT91C_PA3_NPCS3;
            *AT91C_PIOA_PDR = AT91C_PA3_NPCS3;
            return 1;
        }
        else if (bitmask == BIT (5))        
        {
            *AT91C_PIOA_BSR = AT91C_PA5_NPCS3;
            *AT91C_PIOA_PDR = AT91C_PA5_NPCS3;
            return 1;
        }
        else if (bitmask == BIT (22))        
        {
            *AT91C_PIOA_BSR = AT91C_PA22_NPCS3;
            *AT91C_PIOA_PDR = AT91C_PA22_NPCS3;
            return 1;
        }
        break;

    default:
        break;
    }
    return 0;
}


static bool
spi_channel_cs_disable (spi_channel_t channel, port_mask_t bitmask)
{
    /* Disable the NPCSx signals from being asserted by reassigning them
       as GPIO.  */

    switch (channel)
    {
    case 0:
        if (bitmask == BIT (11))
        {
            *AT91C_PIOA_PER = AT91C_PA11_NPCS0;
            return 1;
        }
        break;

    case 1:
        if (bitmask == BIT (9))
        {
            *AT91C_PIOA_PER = AT91C_PA9_NPCS1;
            return 1;
        }
        else if (bitmask == BIT (31))
        {
            *AT91C_PIOA_PER = AT91C_PA31_NPCS1;
            return 1;
        }
        break;

    case 2:
        if (bitmask == BIT (10))
        {
            *AT91C_PIOA_PER = AT91C_PA10_NPCS2;
            return 1;
        }
        else if (bitmask == BIT (30))
        {
            *AT91C_PIOA_PER = AT91C_PA30_NPCS2;
            return 1;
        }
        break;

    case 3:
        if (bitmask == BIT (3))
        {
            *AT91C_PIOA_PER = AT91C_PA3_NPCS3;
            return 1;
        }
        else if (bitmask == BIT (5))        
        {
            *AT91C_PIOA_PER = AT91C_PA5_NPCS3;
            return 1;
        }
        else if (bitmask == BIT (22))        
        {
            *AT91C_PIOA_PER = AT91C_PA22_NPCS3;
            return 1;
        }
        break;

    default:
        break;
    }
    return 0;
}



void
spi_divisor_set (spi_t spi, spi_clock_divisor_t clock_divisor)
{
    spi->clock_divisor = clock_divisor;
}


void
spi_cs_negate_delay_set (spi_t spi, uint16_t delay)
{
    spi->cs_negate_delay = delay;
}


void
spi_cs_assert_delay_set (spi_t spi, uint16_t delay)
{
    spi->cs_assert_delay = delay;
}


void
spi_bits_set (spi_t spi, uint8_t bits)
{
    spi->bits = bits;
}


/* This performs a software reset.  It puts the peripheral in slave mode.  */
void
spi_reset (void)
{
    AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SWRST;
}


static void
spi_lastxfer (void)
{
    /* Set lastxfer bit for use in fixed mode.  */
    AT91C_BASE_SPI->SPI_CR = AT91C_SPI_LASTXFER;
}


void
spi_enable (void)
{
    AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SPIEN;
}


void 
spi_disable (void)
{
    AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SPIDIS;
}


void
spi_mode_set (spi_t spi, spi_mode_t mode)
{
    spi->mode = mode;
}


void
spi_cs_mode_set (spi_t spi, spi_cs_mode_t mode)
{
    spi->cs_mode = mode;
}


/** Enable variable peripheral select.  */
void
spi_multichannel_select (void)
{
    AT91C_BASE_SPI->SPI_MR |= AT91C_SPI_PS_VARIABLE;
}


/** Enable fixed peripheral select and use specified channel.  */
void
spi_channel_select (spi_channel_t channel)
{
    AT91S_SPI *pSPI = AT91C_BASE_SPI;

    pSPI->SPI_MR &= ~AT91C_SPI_PS_VARIABLE;

    BITS_INSERT (pSPI->SPI_MR, SPI_CHANNEL_MASK (channel), 16, 19);
}


/** Enable fixed peripheral select and use specified channel.  */
static void
spi_channel_deselect (void)
{
    BITS_INSERT (AT91C_BASE_SPI->SPI_MR, 0x0f, 16, 19);
}


bool
spi_cs_enable (spi_t spi)
{
    spi->cs_auto = spi_channel_cs_enable (spi->channel, spi->cs.bitmask);
    return spi->cs_auto;
}


bool
spi_cs_disable (spi_t spi)
{
    spi->cs_auto = 0;
    return spi_channel_cs_disable (spi->channel, spi->cs.bitmask);
}


static inline void
spi_cs_assert (spi_t spi)
{
    port_pins_set_low (spi->cs.port, spi->cs.bitmask);
    spi->cs_active = 1;
}


static inline void
spi_cs_negate (spi_t spi)
{
    port_pins_set_high (spi->cs.port, spi->cs.bitmask);
    spi->cs_active = 0;
}


void
spi_config (spi_t spi)
{
    if (spi == spi_config_last)
        return;
    spi_config_last = spi;
    
    spi_channel_cs_mode_set (spi->channel, spi->cs_mode);
    spi_channel_mode_set (spi->channel, spi->mode);
    spi_channel_bits_set (spi->channel, spi->bits);
    spi_channel_clock_divisor_set (spi->channel, spi->clock_divisor);
    spi_channel_clock_delay_set (spi->channel, spi->cs_assert_delay);
    spi_channel_delay_set (spi->channel, (spi->cs_negate_delay + 31) / 32);

    spi_channel_select (spi->channel);
}


/** Initialise SPI for master mode.  */
spi_t
spi_init (const spi_cfg_t *cfg)
{
    spi_dev_t *spi;

    if (spi_devices_num >= SPI_DEVICES_NUM)
        return 0;

    spi = spi_devices + spi_devices_num;
    spi_devices_num++;

    spi->channel = cfg->channel;
    spi->cs = cfg->cs;

    spi_channel_csr_set (spi->channel, 0);

    spi_cs_mode_set (spi, SPI_CS_MODE_TOGGLE);
    spi_cs_enable (spi);
    spi->cs_active = 0;

    if (spi->cs.bitmask && !spi->cs_auto)
    {
        port_pins_config_output (spi->cs.port, spi->cs.bitmask);
        spi_cs_negate (spi);
    }

    spi_cs_assert_delay_set (spi, 0);
    spi_cs_negate_delay_set (spi, 0);
    spi_mode_set (spi, SPI_MODE_0);
    spi_bits_set (spi, 8);
    /* If clock divisor not specified, default to something slow.  */
    spi_divisor_set (spi, cfg->clock_divisor ? cfg->clock_divisor : 128);

    spi_wakeup (spi);
    return spi;
}


void
spi_wakeup (spi_t spi)
{
    uint8_t dev_num;
    bool wakeup;

    dev_num = spi - spi_devices;

    if (spi_devices_enabled & BIT (dev_num))
        return;

    wakeup = !spi_devices_enabled;

    spi_devices_enabled |= BIT (dev_num);

    if (!wakeup)
        return;

    /* Configure PIO for MISO, MOSI, SPCK, NPCS[3:0] (chip selects).
       There is a choice of which pins to use for some of these signals:
       MISO  PA12(A)
       MOSI  PA13(A)
       SPCK  PA14(A)
       NPCS0 PA11(A)
       NPCS1 PA9(B)  PA31(A)
       NPCS2 PA10(B) PA30(B)
       NPCS3 PA3(B)  PA5(B)  PA22(B)
    */

    *AT91C_PIOA_ASR = AT91C_PA13_MOSI | AT91C_PA12_MISO | AT91C_PA14_SPCK; 

    /* Disable pins as GPIO.  */
    *AT91C_PIOA_PDR = AT91C_PA13_MOSI | AT91C_PA12_MISO | AT91C_PA14_SPCK;

    /* Disable pullups.  */
    *AT91C_PIOA_PPUDR = AT91C_PA13_MOSI | AT91C_PA12_MISO | AT91C_PA14_SPCK;

    /* Enable SPI peripheral clock.  */
    AT91C_BASE_PMC->PMC_PCER = BIT (AT91C_ID_SPI);

    /* Reset the SPI (it is possible to reset the SPI even if disabled).  */
    spi_reset ();

    /* Desire PS = 0 (fixed peripheral select)
       PCSDEC = 0 (no decoding of chip selects)
       MSTR = 1 (master mode)
       MODFDIS = 1 (mode fault detection disabled)
       CSAAT = 0 (chip select rises after transmission)
    */

    AT91C_BASE_SPI->SPI_MR = AT91C_SPI_MSTR + AT91C_SPI_MODFDIS;

    spi_enable ();
}


void
spi_shutdown (spi_t spi)
{
    uint8_t dev_num;

    dev_num = spi - spi_devices;

    if (! (spi_devices_enabled & BIT (dev_num)))
        return;

    spi_devices_enabled &= ~BIT (dev_num);

    spi_config_last = 0;

    if (spi_devices_enabled)
        return;
            
    spi_disable ();

    /* Make SPI pins GPIO.  */
    *AT91C_PIOA_PER = AT91C_PA13_MOSI | AT91C_PA14_SPCK;

    /* Disable pullups (they should be disabled anyway).  */
    *AT91C_PIOA_PPUDR = AT91C_PA13_MOSI | AT91C_PA12_MISO | AT91C_PA14_SPCK;

    /* Force lines low to prevent powering devices.  */
    *AT91C_PIOA_CODR = AT91C_PA13_MOSI | AT91C_PA14_SPCK;

    /* Disable SPI peripheral clock.  */
    AT91C_BASE_PMC->PMC_PCDR = BIT (AT91C_ID_SPI);
}


/* Return non-zero if there is a character ready to be read.  */
bool
spi_read_ready_p (spi_t spi __UNUSED__)
{
#if HOSTED
    return 1;
#else
    return SPI_READY_P ();
#endif
}


/* Return non-zero if a character can be written without blocking.  */
bool
spi_write_ready_p (spi_t spi __UNUSED__)
{
    return SPI_READY_P ();
}


/* Write character to SPI, return received character.  */
uint8_t
spi_xferc (spi_t spi, char ch)
{
    uint8_t rxdata;

    spi_config (spi);
    spi_cs_assert (spi);
    SPI_XFER (ch, rxdata);
    spi_cs_negate (spi);

    return rxdata;
}


/* Read character from SPI by sending a dummy word.  */
uint8_t
spi_getc (spi_t spi)
{
    return spi_xferc (spi, 0);
}


/* Write character to SPI, ignore received character.  */
void
spi_putc (spi_t spi, char ch)
{
    spi_xferc (spi, ch);
}


spi_ret_t
spi_transfer_8 (spi_t spi, const void *txbuffer, void *rxbuffer,
                spi_size_t len, bool terminate)
{
    spi_size_t i;
    const uint8_t *txdata = txbuffer;
    uint8_t *rxdata = rxbuffer;
        
    if (!len)
        return 0;

    if (!txdata)
        txdata = rxdata;

    spi_config (spi);

    if (spi->cs_auto)
    {
        spi_channel_cs_mode_set (spi->channel, spi->cs_mode);

        for (i = 0; i < len; i++)
        {
            uint8_t rx;
            uint8_t tx;

            tx = *txdata++;

            if (terminate && i >= len - 1)
                spi_channel_cs_mode_set (spi->channel, SPI_CS_MODE_TOGGLE);

            SPI_XFER (tx, rx);

            if (rxdata)
                *rxdata++ = rx;
        }

        return i;
    }

    if (spi->cs_mode == SPI_CS_MODE_FRAME)
        spi_cs_assert (spi);

    for (i = 0; i < len; i++)
    {
        uint8_t rx;
        uint8_t tx;

        if (spi->cs_mode == SPI_CS_MODE_TOGGLE)
            spi_cs_assert (spi);

        tx = *txdata++;
        
        SPI_XFER (tx, rx);

        if (rxdata)
            *rxdata++ = rx;

        if (spi->cs_mode == SPI_CS_MODE_TOGGLE)
            spi_cs_negate (spi);
    }

    if (terminate && spi->cs_mode == SPI_CS_MODE_FRAME)
        spi_cs_negate (spi);

    return i;
}


spi_ret_t
spi_transfer_16 (spi_t spi, const void *txbuffer, void *rxbuffer,
                 spi_size_t len, bool terminate)
{
    spi_size_t i;
    const uint16_t *txdata = txbuffer;
    uint16_t *rxdata = rxbuffer;
        
    if (!len)
        return 0;

    if (!txdata)
        txdata = rxdata;

    spi_config (spi);

    if (spi->cs_auto)
    {
        /* There is only a marginal benefit using automatic chip
           select assertion.  It primarily designed for use with
           DMA.  */

        for (i = 0; i < len; i += 2)
        {
            uint16_t rx;
            uint16_t tx;

            if (terminate && i >= len - 1)
                spi_lastxfer ();

            tx = *txdata++;

            SPI_XFER (tx, rx);

            if (rxdata)
                *rxdata++ = rx;
        }
        return i;
    }

    if (spi->cs_mode == SPI_CS_MODE_FRAME)
        spi_cs_assert (spi);

    for (i = 0; i < len; i += 2)
    {
        uint16_t rx;
        uint16_t tx;

        if (spi->cs_mode == SPI_CS_MODE_TOGGLE)
            spi_cs_assert (spi);

        tx = *txdata++;
        
        SPI_XFER (tx, rx);

        if (rxdata)
            *rxdata++ = rx;

        if (spi->cs_mode == SPI_CS_MODE_TOGGLE)
            spi_cs_negate (spi);
    }

    if (terminate && spi->cs_mode == SPI_CS_MODE_FRAME)
        spi_cs_negate (spi);

    return i;
}


spi_ret_t
spi_transfer (spi_t spi, const void *txbuffer, void *rxbuffer,
              spi_size_t len, bool terminate)
{
    /* If terminate is zero we should lock the SPI peripheral
       for this device until terminate is non-zero.  */

    if (spi->bits <= 8)
        return spi_transfer_8 (spi, txbuffer, rxbuffer, len, terminate);
    else
        return spi_transfer_16 (spi, txbuffer, rxbuffer, len, terminate);
}


spi_ret_t
spi_write (spi_t spi, const void *buffer, spi_size_t len, bool terminate)
{
    return spi_transfer (spi, buffer, 0, len, terminate);
}


spi_ret_t
spi_read (spi_t spi, void *buffer, spi_size_t len, bool terminate)
{
    return spi_transfer (spi, 0, buffer, len, terminate);
}