/** @file   spi.c
    @author Michael Hayes
    @date   30 July 2007
    @brief  Routines for interfacing to the SPI bus.
*/

#include "config.h"
#include "spi.h"
#include "bits.h"

/* This driver only configures the SPI controller as a master.

   The AT91 SPI peripheral can transmit either 8 or 16 bit data with
   MSB first.  It has 4 separate chip select registers and can control
   4 different types of SPI peripheral using the separate chip select
   registers.  Each peripheral can have its own chip select that is
   automatically driven for each transfer.

   The AT91 SPI peripheral has been designed so that blocks of a data
   can be streamed using DMA to multiple devices on the bus (Variable
   Peripheral Selection).  It achieves this by using 32 bits to
   specify the 8 or 16 bit data plus a bit mask specifying which
   peripheral to write to.

   There are 4 control status registers (CSRs); one associated with
   each of the four SPI chip select (CS) signals.  These configure the
   number of bits per transfer and the CS timing for each of the 4
   channels.  The SPI controller chooses one of these 4 CSRs on the
   basis of the channel select mask in the mode register (fixed
   peripheral mode) or in the data (variable peripheral mode).  Now
   this is all fine and dandy if we have 4 SPI devices.  What if we
   want more?  Well one option is to decode the 4 CS signals to
   provide 15 channels; with four per group.  This requires external
   hardware so is not much use.  The alternative is to bit bash
   additional output ports. 

   With this driver we can share the 4 channels among multiple SPI
   devices.  However, automatic CS driving can only be performed for
   the few NPCS signals associated with a channel, otherwise the CS
   signals are bit-bashed.  If an NPCS pin is configured as an SPI
   peripheral it will be driven whenever its channel is used.  Thus if
   we want to have other CS signals driven by PIO lines, we need to
   switch the NPCS signals at the start and end of a transfer.

   There are two chip select modes: FRAME where the CS is asserted for
   multiple SPI tranmissions; and TOGGLE where the CS is only asserted
   for each SPI transmission.

   Note, functions that configure the SPI peripheral (such as
   spi_bits_set) only take effect when spi_config is called (usually
   when some I/O is to be performed).
*/


/* Number of independent SPI controllers.  */
#ifdef AT91C_BASE_SPI1
#define SPI_CONTROLLERS_NUM 2
#else
#define SPI_CONTROLLERS_NUM 1
#endif


/* Number of channels per controller.  */
#define SPI_CHANNELS_NUM 4


/* Currently the maximum of SPI devices is 32 due to the size
   of the mask used for recording active devices.  */
#ifndef SPI_DEVICES_NUM
#define SPI_DEVICES_NUM 8
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

#ifndef AT91C_BASE_SPI0
#define AT91C_BASE_SPI0 AT91C_BASE_SPI
#endif

#ifndef AT91C_BASE_SPI1
#define AT91C_BASE_SPI1 0
#endif

#define SPI_BASE_GET(channel) (((channel) < SPI_CHANNELS_NUM) ? AT91C_BASE_SPI0 : AT91C_BASE_SPI1)

#define SPI_CHANNEL_MASK(channel) (0x0f ^ BIT (channel & (SPI_CHANNELS_NUM - 1)))

#ifdef HOSTED
#define SPI_READY_P(BASE) (HOSTED || ((BASE)->SPI_SR & AT91C_SPI_RDRF))
#else
#define SPI_READY_P(BASE) ((BASE)->SPI_SR & AT91C_SPI_RDRF)
#endif

#define SPI_TXEMPTY_P(BASE) ((BASE)->SPI_SR & AT91C_SPI_TXEMPTY)

/* Set lastxfer bit for use in fixed mode.  */
#define SPI_LASTXFER(BASE) ((BASE)->SPI_CR = AT91C_SPI_LASTXFER)


/* Read SPI and then send new data.  Blocks while the new data is
   being sent by the spi.  */
#define SPI_XFER(pSPI, txdata, rxdata)                                  \
    do                                                                  \
    {                                                                   \
        /* Dummy read from RDR to ensure RDRF clear.   */               \
        (rxdata) = pSPI->SPI_RDR;                                       \
                                                                        \
        /* Write data to TDR.  */                                       \
        pSPI->SPI_TDR = (txdata);                                       \
                                                                        \
        /* Wait until spi port finishes transmitting/receiving.  */     \
        while (!SPI_READY_P (pSPI))                                     \
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
    AT91S_SPI *pSPI = SPI_BASE_GET (channel);

    return pSPI->SPI_CSR[channel & (SPI_CHANNELS_NUM - 1)];
}


static inline void
spi_channel_csr_set (spi_channel_t channel, uint32_t csr)
{
    AT91S_SPI *pSPI = SPI_BASE_GET (channel);

    pSPI->SPI_CSR[channel & (SPI_CHANNELS_NUM - 1)] = csr;
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


enum {PERIPH_A = 0, PERIPH_B};

typedef struct spi_cs_struct
{
    uint8_t channel;
    pio_t pio;
    uint8_t periph;
} spi_cs_t;


#define SPI_CS(CHANNEL, PIO, PERIPH) \
    {.channel = (CHANNEL), .pio = (PIO), .periph = (PERIPH)}


/* NB, if you get `error: initializer element is not constant' then
   need to compile without -std=c99 or -std=gnu99  */

#if SPI_CONTROLLERS_NUM == 2
/* AT91SAM7X  */
static const spi_cs_t spi_cs[] = 
{
    SPI_CS (0, PIO_DEFINE (PORT_A, 21), PERIPH_B),
    SPI_CS (1, PIO_DEFINE (PORT_A, 25), PERIPH_B),
    SPI_CS (2, PIO_DEFINE (PORT_A, 26), PERIPH_B),
    SPI_CS (3, PIO_DEFINE (PORT_A, 29), PERIPH_B),
    SPI_CS (1, PIO_DEFINE (PORT_B, 10), PERIPH_B),
    SPI_CS (2, PIO_DEFINE (PORT_B, 11), PERIPH_B),
    SPI_CS (3, PIO_DEFINE (PORT_B, 16), PERIPH_B),
};

#define SPI0_PINS (AT91C_PA17_MOSI0 | AT91C_PA16_MISO0 | AT91C_PA18_SPCK0)
#define SPI1_PINS (AT91C_PA23_MOSI1 | AT91C_PA24_MISO1 | AT91C_PA22_SPCK1)

#else
/* AT91SAM7S  */
static const spi_cs_t spi_cs[] = 
{
    SPI_CS (0, PIO_DEFINE (PORT_A, 11), PERIPH_A),
    SPI_CS (1, PIO_DEFINE (PORT_A, 9), PERIPH_B),
    SPI_CS (1, PIO_DEFINE (PORT_A, 31), PERIPH_A),
    SPI_CS (2, PIO_DEFINE (PORT_A, 10), PERIPH_B),
    SPI_CS (2, PIO_DEFINE (PORT_A, 30), PERIPH_B),
    SPI_CS (3, PIO_DEFINE (PORT_A, 3), PERIPH_B),
    SPI_CS (3, PIO_DEFINE (PORT_A, 5), PERIPH_B),
    SPI_CS (3, PIO_DEFINE (PORT_A, 22), PERIPH_B)
};

#define SPI0_PINS (AT91C_PA13_MOSI | AT91C_PA12_MISO | AT91C_PA14_SPCK)

#endif


#define SPI_CS_NUM ARRAY_SIZE (spi_cs)


static pio_config_t
spi_channel_cs_config_get (spi_channel_t channel, pio_t cs)
{
    unsigned int i;

    for (i = 0; i < SPI_CS_NUM; i++)
    {
        if (channel == spi_cs[i].channel
            && cs.port == spi_cs[i].pio.port
            && cs.bitmask == spi_cs[i].pio.bitmask)
        {
            return spi_cs[i].periph == PERIPH_A ? PIO_PERIPH_A : PIO_PERIPH_B;
        }
    }
    return PIO_OUTPUT_HIGH;
}


static void
spi_update (spi_t spi)
{
    /* Need to force an update of the config.  */
    if (spi == spi_config_last)
        spi_config_last = 0;
}


void
spi_clock_divisor_set (spi_t spi, spi_clock_divisor_t clock_divisor)
{
    if (clock_divisor == 0)
        clock_divisor = 1;
    spi->clock_divisor = clock_divisor;
    spi_update (spi);
}


spi_clock_speed_t
spi_clock_speed_set (spi_t spi, spi_clock_speed_t clock_speed)
{
    spi_clock_divisor_set (spi, (F_CPU + clock_speed - 1) / clock_speed);
    clock_speed = F_CPU / spi->clock_divisor;
    return clock_speed;
}


void
spi_cs_negate_delay_set (spi_t spi, uint16_t delay)
{
    spi->cs_negate_delay = delay;
    spi_update (spi);
}


void
spi_cs_assert_delay_set (spi_t spi, uint16_t delay)
{
    spi->cs_assert_delay = delay;
    spi_update (spi);
}


void
spi_bits_set (spi_t spi, uint8_t bits)
{
    spi->bits = bits;
    spi_update (spi);
}


/* This performs a software reset for the specified controller (not an
   individual channel).  It puts the peripheral in slave mode.  */
static void
spi_reset (AT91S_SPI *pSPI)
{
    pSPI->SPI_CR = AT91C_SPI_SWRST;
}


/* This enables the specified controller (not an individual
   channel).  */
static void
spi_enable (AT91S_SPI *pSPI)
{
    pSPI->SPI_CR = AT91C_SPI_SPIEN;
}


static void 
spi_disable (AT91S_SPI *pSPI)
{
    pSPI->SPI_CR = AT91C_SPI_SPIDIS;
}


static void 
spi_setup (AT91S_SPI *pSPI)
{
    /* Desire PS = 0 (fixed peripheral select)
       PCSDEC = 0 (no decoding of chip selects)
       MSTR = 1 (master mode)
       MODFDIS = 1 (mode fault detection disabled)
       CSAAT = 0 (chip select rises after transmission)
    */
    pSPI->SPI_MR = AT91C_SPI_MSTR | AT91C_SPI_MODFDIS;
}


void
spi_mode_set (spi_t spi, spi_mode_t mode)
{
    spi->mode = mode;
    spi_update (spi);
}


void
spi_cs_mode_set (spi_t spi, spi_cs_mode_t mode)
{
    spi->cs_mode = mode;
    spi_update (spi);
}


/** Enable fixed peripheral select and use specified channel.  When an
    SPI transfer takes place, the channel is used to look up the
    correct CSR.  */
static void
spi_channel_select (spi_channel_t channel)
{
    AT91S_SPI *pSPI = SPI_BASE_GET (channel);

    pSPI->SPI_MR &= ~AT91C_SPI_PS_VARIABLE;

    /* Insert bit pattern to specify which CSR to use and which NPCS
       to drive.  Note, if a value of 0xf is used then the SPI controller
       will hang.  */
    BITS_INSERT (pSPI->SPI_MR, SPI_CHANNEL_MASK (channel), 16, 19);
}


/** Force CS to go low.  Return non-zero if deferred.  */
bool
spi_cs_assert (spi_t spi)
{
    if (spi->cs_active || spi->cs_mode == SPI_CS_MODE_HIGH)
        return 0;

    spi->cs_active = 1; 

    if (spi->cs_config ==  PIO_OUTPUT_HIGH)
    {
        pio_config_set (spi->cs, PIO_OUTPUT_LOW);
        return 0;
    }
    else
    {
        /* The CS will be automatically driven low.  */
        pio_config_set (spi->cs, spi->cs_config);            
        return 1;
    }
}


/** Force CS to go high.  */
void
spi_cs_negate (spi_t spi)
{
    /* The CS may have automatically been driven high, but ensure it stays high
       in case we switch to another device using the same channel.  */
    pio_config_set (spi->cs, PIO_OUTPUT_HIGH);
    spi->cs_active = 0;
}


void
spi_config (spi_t spi)
{
    /* There are 4 sets of SPI registers, one for each channel.  We
       could check which SPI instance is currently using a channel and
       thus avoid reprogramming the registers unnecessarily.  */

    if (spi == spi_config_last)
        return;
    spi_config_last = spi;

    spi_channel_select (spi->channel);    
    spi_channel_cs_mode_set (spi->channel, spi->cs_mode);
    spi_channel_mode_set (spi->channel, spi->mode);
    spi_channel_bits_set (spi->channel, spi->bits);
    spi_channel_clock_divisor_set (spi->channel, spi->clock_divisor);
    spi_channel_clock_delay_set (spi->channel, spi->cs_assert_delay);
    spi_channel_delay_set (spi->channel, (spi->cs_negate_delay + 31) / 32);
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

    spi->cs_config = spi_channel_cs_config_get (spi->channel, spi->cs); 
    spi_cs_negate (spi);

    spi_cs_assert_delay_set (spi, 0);
    spi_cs_negate_delay_set (spi, 0);
    spi_mode_set (spi, cfg->mode);
    spi_bits_set (spi, cfg->bits ? cfg->bits : 8);
    /* If clock divisor not specified, default to something slow.  */
    spi_clock_divisor_set (spi, cfg->clock_divisor ? cfg->clock_divisor : 128);

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

    /* Configure PIO for MISO, MOSI, SPCK.    */
    *AT91C_PIOA_ASR = SPI0_PINS;

    /* Disable pins as GPIO.  */
    *AT91C_PIOA_PDR = SPI0_PINS;

    /* Disable pullups.  */
    *AT91C_PIOA_PPUDR = SPI0_PINS;

    /* Enable SPI peripheral clock.  */
    AT91C_BASE_PMC->PMC_PCER = BIT (AT91C_ID_SPI);
   
    spi_reset (AT91C_BASE_SPI0);
    spi_setup (AT91C_BASE_SPI0);
    spi_enable (AT91C_BASE_SPI0);

#if SPI_CONTROLLERS_NUM == 2
    /* Configure PIO for MISO, MOSI, SPCK.    */
    *AT91C_PIOB_ASR = SPI1_PINS;

    /* Disable pins as GPIO.  */
    *AT91C_PIOB_PDR = SPI1_PINS;

    /* Disable pullups.  */
    *AT91C_PIOB_PPUDR = SPI1_PINS;

    /* Enable SPI peripheral clock.  */
    AT91C_BASE_PMC->PMC_PCER = BIT (AT91C_ID_SPI1);

    spi_reset (AT91C_BASE_SPI1);
    spi_setup (AT91C_BASE_SPI1);
    spi_enable (AT91C_BASE_SPI1);
#endif
}


/** This only takes effect when the last SPI device is shutdown.  Then
    the CS lines are forced low.  This can only be done when there is
    no more activity on the SPI bus.  */
void
spi_shutdown (spi_t spi)
{
    uint8_t dev_num;
    int i;

    dev_num = spi - spi_devices;

    if (! (spi_devices_enabled & BIT (dev_num)))
        return;

    spi_devices_enabled &= ~BIT (dev_num);

    spi_config_last = 0;

    if (spi_devices_enabled)
        return;
           
    spi_disable (AT91C_BASE_SPI0); 

#if SPI_CONTROLLERS_NUM == 2
    spi_disable (AT91C_BASE_SPI1); 

    /* Enable pins as GPIO.  */
    *AT91C_PIOA_PER = SPI0_PINS;
    *AT91C_PIOB_PER = SPI1_PINS;

    /* Force lines low to prevent powering devices.  */
    *AT91C_PIOA_CODR = SPI0_PINS;
    *AT91C_PIOB_CODR = SPI1_PINS;

    /* Disable SPI peripheral clocks.  */
    AT91C_BASE_PMC->PMC_PCDR = BIT (AT91C_ID_SPI0) | BIT (AT91C_ID_SPI1);
#else
    /* Enable pins as GPIO.  */
    *AT91C_PIOA_PER = SPI0_PINS;

    /* Disable pullups.  */
    *AT91C_PIOA_PPUDR = SPI0_PINS;

    /* Force lines low to prevent powering devices.  */
    *AT91C_PIOA_CODR = SPI0_PINS;

    /* Disable SPI peripheral clock.  */
    AT91C_BASE_PMC->PMC_PCDR = BIT (AT91C_ID_SPI);
#endif

    /* Set all the chip select pins low.  */
    for (i = 0; i < spi_devices_num; i++)
    {
        pio_config_set (spi_devices[i].cs, PIO_OUTPUT_LOW);
    }
}


spi_ret_t
spi_transfer_8 (spi_t spi, const void *txbuffer, void *rxbuffer,
                spi_size_t len, bool terminate)
{
    spi_size_t i;
    const uint8_t *txdata = txbuffer;
    uint8_t *rxdata = rxbuffer;
    AT91S_SPI *pSPI = SPI_BASE_GET (spi->channel);
        
    if (!len)
        return 0;

    if (!txdata)
        txdata = rxdata;

    spi_config (spi);

    if (spi->cs_config != PIO_OUTPUT_HIGH)
    {
        /* The CS won't be asserted until the transfer takes place.  */
        spi_cs_assert (spi);

        for (i = 0; i < len; i++)
        {
            uint8_t rx;
            uint8_t tx;

            tx = *txdata++;

            if (terminate && i >= len - 1)
            {
                /* Ensure CS driven high at end of transfer.  */
                spi_cs_negate (spi);
            }

            SPI_XFER (pSPI, tx, rx);

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
        
        SPI_XFER (pSPI, tx, rx);

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
    AT91S_SPI *pSPI = SPI_BASE_GET (spi->channel);
        
    if (!len)
        return 0;

    if (!txdata)
        txdata = rxdata;

    spi_config (spi);

    if (spi->cs_config != PIO_OUTPUT_HIGH)
    {
        /* The CS won't be asserted until the transfer takes place.
           There is only a marginal benefit using automatic chip
           select assertion.  It is primarily designed for use with
           DMA.  */
        spi_cs_assert (spi);

        for (i = 0; i < len; i += 2)
        {
            uint16_t rx;
            uint16_t tx;

            if (terminate && i >= len - 1)
                SPI_LASTXFER (pSPI);

            tx = *txdata++;

            SPI_XFER (pSPI, tx, rx);

            if (rxdata)
                *rxdata++ = rx;
        }

        if (terminate)
            spi_cs_negate (spi);

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
        
        SPI_XFER (pSPI, tx, rx);

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


/* Return non-zero if there is a character ready to be read.  */
bool
spi_read_ready_p (spi_t spi)
{
#if HOSTED
    return 1;
#else
    return SPI_READY_P (SPI_BASE_GET (spi->channel));
#endif
}


/* Return non-zero if a character can be written without blocking.  */
bool
spi_write_ready_p (spi_t spi)
{
    return SPI_READY_P (SPI_BASE_GET (spi->channel));
}


/* Write character to SPI, return received character.  */
uint8_t
spi_xferc (spi_t spi, char ch)
{
#if 0
    uint8_t txdata;
    uint8_t rxdata;

    txdata = ch;
    spi_transfer_8 (spi, &txdata, &rxdata, 1, 1);

    return rxdata;
#else
    uint8_t rxdata;
    AT91S_SPI *pSPI = SPI_BASE_GET (spi->channel);

    spi_config (spi);
    /* This is a nop if CS automatically driven.  */
    spi_cs_assert (spi);
    SPI_XFER (pSPI, ch, rxdata);
    /* This is a nop if CS automatically driven.  */
    spi_cs_negate (spi);

    return rxdata;
#endif
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
