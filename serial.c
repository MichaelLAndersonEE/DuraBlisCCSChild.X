/* File:   serial.c  UART consoleIO
 * Author: Michael L Anderson
 * Contact: MichaelLAndersonEE@gmail.com
 * Platform: PIC32MX130F064B on BW-CCS-Child board, ver 3.
 * Exar SP3222 RS232 driver
 * Created: 16 May 14
 */

#include <xc.h>
#include "DuraBlisChild.h"
#include "serial.h"

void rs485TransmitEna(void)
{
    RS485_RE_n = 1;
    RS485_DE = 1;       // Enable Tx
    LED_GREEN = 1;
    delay_us(30);
}

void rs485TransmitDisa(void)
{
    char chIn;
    while(U1STAbits.TRMT == 0) ;
    RS485_RE_n = 0;
    RS485_DE = 0;       // Disable Tx
    chIn =  U1RXREG;      // Clears URXDA
    LED_GREEN = 0;
}
//void rs485TransmitEna(byte onOff)
//{
//    char chIn;
//
//    if (onOff == 1)
//    {
//       // RS485_RE_n = 1;
//        RS485_DE = 1;       // Enable Tx
//       // RS485_RE_n = 1;
//        delay_us(30);
//    }
//    else
//    {
//        while(U1STAbits.TRMT == 0) ;
//        RS485_DE = 0;       // Disable Tx
//        RS485_RE_n = 0;
//        chIn =  U1RXREG;      // Clears URXDA
//    }
//}

void putUns2Hex(unsigned uns)
{
    putByte2Hex(uns >> 8);
    putByte2Hex(uns);
}

//void put3Nib2Hex(unsigned uns)
//{
//    unsigned arg;
//    arg = uns >> 8;
//    if (arg > 9) putChar(arg + 'A' - 10);
//    else (putChar(arg + '0'));
//    arg = uns >> 4;
//    if (arg > 9) putChar(arg + 'A' - 10);
//    else (putChar(arg + '0'));
//    arg = uns & 0x000F;
//    if (arg > 9) putChar(arg + 'A' - 10);
//    else (putChar(arg + '0'));
//}
//
void putByte2Hex(byte me)
{
    putNib2Hex(me >> 4);
    putNib2Hex(me);

//    byte arg;
//    arg = me >> 4;
//    if (arg > 9) putChar(arg + 'A' - 10);
//    else (putChar(arg + '0'));
//    arg = me & 0x0F;
//    if (arg > 9) putChar(arg + 'A' - 10);
//    else (putChar(arg + '0'));
}

    // Console out lower nibble
void putNib2Hex(byte nib)
{
    nib &= 0x0F;
    if (nib > 9) putChar(nib + 'A' - 10);
    else (putChar(nib + '0'));
}

void putPrompt(void)
{
    putStr("\r\n > ");
}

    // This is a noninterrupt UART read. It is transparent;
    // returning 0 if there is nothing in.
char getChar(void)
{
    char ch;

   // COMM_RX_ENA_n = COMM_SHDN_n = 0;
    if (U1STAbits.URXDA)
    {
        ch =  U1RXREG;      // Clears URXDA
        return(ch);
    }
    else return(0);
}

    // Case invariant
int parseHex3Byte(char nibHi, char nibMid, char nibLo, unsigned *hexaFlexagon)
{
    unsigned buffer = 0;
    if (nibHi >= '0' && nibHi <= '9') buffer = (nibHi - '0') << 8;
    else if (nibHi >= 'a' && nibHi <= 'f') buffer = (nibHi - 'a' + 10) << 8;
    else if (nibHi >= 'A' && nibHi <= 'F') buffer = (nibHi - 'A' + 10) << 8;
    else return(-1);

    if (nibMid >= '0' && nibMid <= '9') buffer += (nibMid - '0') << 4;
    else if (nibMid >= 'a' && nibMid <= 'f') buffer = (nibMid - 'a' + 10) << 4;
    else if (nibMid >= 'A' && nibMid <= 'F') buffer = (nibMid - 'A' + 10) << 4;
    else return(-1);

    if (nibLo >= '0' && nibLo <= '9') buffer += (nibLo - '0');
    else if (nibLo >= 'a' && nibLo <= 'f') buffer += (nibLo - 'a' + 10);
    else if (nibLo >= 'A' && nibLo <= 'F') buffer += (nibLo - 'A' + 10);
    else return(-1);
    *hexaFlexagon = buffer;
    return(1);
}

    // ------------------------
    // TODO needs work.  Need retval for null CR
//int getStr(char *buffer, byte numChars, unsigned timeOut)
//{
//    byte bufIdx = 0;
//    char inCh;
//
////    if (buffer[0] == '1')   // DEB
////    {
////        buffer[0] = 'a';    // DEB
////        buffer[1] = 'k';
////        buffer[2] = '1';    // Node num
////        buffer[3] = '1';    // PNet ver
////        buffer[4] = '0';    // EQ hi
////        buffer[5] = '1';    // EQ lo
////        buffer[6] = 13;     // CR
////        putStr("ak1101\r\n");
////        return(6);
////    }
//    //else return(0);
//
//    while(timeOut--)
//    {
//            // DEB
//        //if (timeOut % 1000 == 0) putChar('*');
//
//        if ((inCh = getChar()) > 0)
//        {
//            timeOut = TIMEOUT_HUMAN;
//            buffer[bufIdx++] = inCh;
//            putChar(inCh);
//        }
//       // if (timeRead(TIME_OUT_HUMAN)) return(-1);
//        if (bufIdx > numChars) return(-2);
//        if (inCh == 13)
//        {
//            buffer[bufIdx] = 0;
//            return(bufIdx);
//        }
//        if (timeOut > TIMEOUT_PNET) delay_us(4000);
//        else delay_us(100);     // TODO really cheesy
//    }
//    if (timeOut == 0) return(-1);
//}

    // Print string up to 40 chars
void putStr(const char *str)
{
    byte me;

  //  rs485TransmitEna(1);
    for (me=0; me<41; me++)
    {
        if (str[me] == 0) return;
        U1TXREG = str[me];
        while (U1STAbits.UTXBF) ;
    }
  //  rs485TransmitEna(0);
}

void putChar(char c)
{
 //   rs485TransmitEna(1);
    U1TXREG = c;
    while (U1STAbits.UTXBF) ;
 //   rs485TransmitEna(0);
}

//char *itoa(char *buf, int val, int base)
//{
//    char *cp = buf;
//
//    if(val < 0)
//    {
//	*buf++ = '-';
//	val = -val;
//    }
//    utoa(buf, val, base);
//    return cp;
//}
//
//char *utoa(char *buf, unsigned val, int base)
//{
//    unsigned	v;
//    char	c;
//
//    v = val;
//    do
//    {
//	v /= base;
//	buf++;
//    } while(v != 0);
//    *buf-- = 0;
//    do
//    {
//	c = val % base;
//	val /= base;
//	if(c >= 10)
//	c += 'A'-'0'-10;
//	c += '0';
//	*buf-- = c;
//    } while(val != 0);
//    return buf;
//}
