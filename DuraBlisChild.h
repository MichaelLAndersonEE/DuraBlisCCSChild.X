/* File:   DuraBlisChild.h  Project wide defines and global variables
 * Author: Michael L Anderson
 * Contact: MichaelLAndersonEE@gmail.com
 * Platform: PIC32MX130F064D on DuraBlisChild board
 * Created: 16 May 14
 */

#ifndef DURABLISCHILD_H
#define	DURABLISCHILD_H

#include <stdbool.h>    // To use bool type

#define SYS_FREQ	(25000000)  //  Primary external osc freq
#define IO_BFR_SIZE     30          // Volatile general IO buffer

    // Child status, which Parent may inquire
    // This must be semantically consistent with parent
//efine ST_YOU_TALKING_TO_ME  0x200
#define ST_DEBUG_MODE       0x100
    // Only lower nibbles reported in Pnet 'A'
#define	ST_LIQUID_DETECTED  0x80
#define ST_TSEN1_OKAY       0x40
#define ST_TSEN2_OKAY       0x20
#define ST_HSEN_OKAY        0x10
#define ST_K1_ON            0x08
#define ST_K2_ON            0x04
#define ST_SwiPwr1_ON       0x02
#define ST_SwiPwr2_ON       0x01

    // Time read modes
#define TIME_UPDATE             0x01
//#define TIME_NEW_MINUTE         0x02
//#define TIME_NEW_SECOND         0x03
#define TIME_LOQUACIOUS         0x04
#define TIMEOUT_HUMAN           0xAAAA   // Related factor
#define TIMEOUT_PNET            0x2222

    /**** Hardware mapping definitions...   ****/

    // Analog channels
#define ANCH_TEMPER     2   // AN2 Pin 21 RB2
#define ANCH_RELHUM     1   // AN1 Pin 20 RA1
#define ANCH_FLOOD      4   // AN4 Pin 23 RB2
#define ANCH_TEMPER2    3   // AN3 Pin 23 RB1

    // Hardware lines
#define NODEID_1        PORTBbits.RB8       // i Pin 44
#define NODEID_2        PORTBbits.RB5       // i Pin 41
#define NODEID_4        PORTCbits.RC5       // i Pin 38
#define NODEID_8        PORTCbits.RC4       // i Pin 37
#define LED_GREEN       LATBbits.LATB13     // o0 Pin 11
#define SMEM_CS_n       LATBbits.LATB9      // o1 Pin 1
#define RELAY1          LATCbits.LATC2      // o1 Pin 27
#define RELAY2          LATCbits.LATC1      // o1 Pin 26
#define SwiPwr1         LATCbits.LATC0      // o1 Pin 24
#define SwiPwr2         LATBbits.LATB3      // o1 Pin 25
#define RS485_DE        LATAbits.LATA4      // o1 Pin 34
#define RS485_RE_n      LATAbits.LATA9      // o1 Pin 35
#define RS485_TERMRES   LATCbits.LATC3      // o1 Pin 36

typedef unsigned char byte;
extern char ioBfr[];

//void delay_ms(unsigned t);        // Dumb delay fcn.
void delay_us(unsigned t);        // Dumb delay fcn.

#endif
