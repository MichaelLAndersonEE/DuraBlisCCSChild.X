/* File:   serial.h  UART consoleIO
 * Author: Michael L Anderson
 * Contact: MichaelLAndersonEE@gmail.com
 * Platform: PIC32MX130F064B on DuraBlis CCS Child board, ver3
 * Created: 16 May 14
 */

#ifndef SERIAL_H
#define	SERIAL_H

#ifdef	__cplusplus
extern "C" {
#endif

#define LINE_FEED           10
#define CARRIAGE_RETURN     13
#define CANCEL              24

//void rs485TransmitEna(byte mode);
void rs485TransmitEna(void);
void rs485TransmitDisa(void);
int getStr(char *buffer, byte numChars, unsigned timeOut);
int parseHex3Byte(char nibHi, char nibMid, char nibLo, unsigned *hexaFlexagon);
void putUns2Hex(unsigned uns);
void putByte2Hex(byte me);
//void put3Nib2Hex(unsigned uns);
void putNib2Hex(byte);
void putPrompt(void);
void putChar(char c);
char getChar(void);
void putStr(const char *);

char *itoa(char *buf, int val, int base);
char *utoa(char *buf, unsigned val, int base);

#ifdef	__cplusplus
}
#endif

#endif	/* SERIAL_H */

