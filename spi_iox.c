/* File:   spi_iox.c  SPI based functions for MCP23SO8 IO Expander
 * Author: Michael L Anderson
 * Contact: MichaelLAndersonEE@gmail.com
 * Platform: PIC32MX130F016H on BW-CCS-Child   board.
 * Created: 18 Aug 14
 */

#include <xc.h>
#include <plib.h>
#include "DuraBlisChild.h"
#include "spi_iox.h"

static void spiIoxWriteAt(byte address, byte wrDatum);
static void spiWrite(byte datum);

const byte commandRead = 0x41;      // This is partly strapped by IC pins
const byte commandWrite = 0x40;     // and partly built into IC decode.
const byte addressGPIO = 9;

void spiIoxConfig(void)
{
    spiIoxWriteAt(0, 0xFF);     // 00 IODIR all inputs
    spiIoxWriteAt(1, 0x00);     // 01 IPOL all straight
    spiIoxWriteAt(2, 0x00);     // 02 GPINTEN, no interrupt on change
    spiIoxWriteAt(3, 0x00);     // 03 DEFVAL, don't care
    spiIoxWriteAt(4, 0x00);     // 04 INTCON, no interrupts
    spiIoxWriteAt(5, 0b0011000);    // No sequential op, no slew rate
}                               // No address pins

void spiIoxWriteAt(byte address, byte wrDatum)
{
    IOXP_CS_n = 0;
    spiWrite(commandWrite);
    spiWrite(address);
    spiWrite(wrDatum);
    IOXP_CS_n = 1;
    _nop();
}

    // Low level primitive
void spiWrite(byte datum)
{
   byte b, mask = 0x80;

   for (b = 0; b < 8; b++)
    {
        SPICLK = 1;
        _nop();
        if (datum & mask) SPIDATO = 1;
        else SPIDATO = 0;
        _nop();
        SPICLK = 0;
        mask = mask >> 1;
    }
}

    // Low level primitive
byte spiRead(void)
{
   byte b = 0, mask = 0x80;

   for (b = 0; b < 8; b++)
    {
        SPICLK = 1;
        _nop();
        if (SPIDATI & mask) b |= mask;
        _nop();
        SPICLK = 0;
        mask >>= 1;
    }
}
 // -------------------
    // TODO needs exceptopn handlign
byte spiIoxRead(void)
{
    byte retVal;

    IOXP_CS_n = 0;
    spiWrite(commandRead);
    spiWrite(addressGPIO);
    retVal = spiRead();
    IOXP_CS_n = 1;
    return(retVal);
}


//    SPI1CONbits.ON = 0;         // Resets
//    SPI1BUF = 0;                // Per protocol
//    tmr1 = SPI1BUF;             // Read clrs .SPIBRF
//    SPI1STATbits.SPIROV = 0;
//    SPI1CONbits.ON = 1;
////    for (tmr1 = 0; tmr1 < 500; tmr1++)
////    {
////        if (SPI1STATbits.SPIRBF == 1) break;
////    }
//
////    if (tmr1 >= 499)
////    {
////        putStr("\tspiReadIOX timeout");
////    }
////    else retVal =  SPI1BUF;
//
//    delay_us(10);
//    retVal =  SPI1BUF;
//   IOXP_CS_n = 1;
//    SPI1CONbits.ON = 0;
