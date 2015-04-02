/* File:        DuraBlisChild.c  Main source file
 * Author:      Michael L Anderson
 * Contact:     MichaelLAndersonEE@gmail.com
 * Platform:    PIC32MX130F064D on DuraBlis Child v4 board
 * Created:     16 May 14
 */

#define _SUPPRESS_PLIB_WARNING
    // TODO May need to migrate to Harmony
#define DEBUGGING 1
#include <xc.h>
#include <plib.h>
#include "configs.h"
#include "DuraBlisChild.h"
#include "serial.h"
#include "adc.h"
#include "pnet.h"

void hardwareInit(void);
void menuDisplay(void);
byte readNodeSw(void);
void serialParse();
void reportInfo(void);
void systemInit(void);

double temperNowF, rhumidNow;
double temperSecondaryF;
double temper1CalFactor = 1.0;
double temper2CalFactor = 1.0;
double rhumidCalFactor = 1.0;
double vRef = 2.992;
unsigned sysStat;  //, sysStatOld;
char ioBfr[IO_BFR_SIZE + 1];

byte pnetNodeNumber;
char pnetVer = 'A';
char uniqueID[11] = "C0123456";
char verStr[11] = "1.031715";

int main(void)
{
    unsigned long tmr1;
    int retVal;

    hardwareInit();
    systemInit();
    pnetNodeNumber = readNodeSw();
    if (pnetNodeNumber == 0x0F)
    {
        sysStat |= ST_DEBUG_MODE;
        rs485TransmitEna();
        putStr("\n\r DB CCS Child\r\n Ver ");   // DEB, not for PROD
        putStr(verStr);
        putStr(", MLA\n\r");
        menuDisplay();
        putPrompt();      
    }
    rs485TransmitDisa();

    while (1)
    {
        pnetNodeNumber = readNodeSw();
        if (pnetNodeNumber == 0xF) sysStat |= ST_DEBUG_MODE;
        else sysStat &= ~ST_DEBUG_MODE;
        serialParse();
        adcRHumid(ADC_SILENT);
        serialParse();
        adcTemper(ADC_SILENT);
        adcTemperSecondary(ADC_SILENT);

        retVal = adcFloodSensor(ADC_SILENT);
        if (retVal == 1) sysStat |= ST_LIQUID_DETECTED;
        else if (retVal == 0) sysStat &= ~ST_LIQUID_DETECTED;

        if (sysStat & ST_DEBUG_MODE) LED_GREEN = 1;

        if (tmr1++ > 5000)
        {
            tmr1 = 0;
            childStateMach(CANCEL);     // This is a crude timeout.  Not sure how to improve it.  We will
            rs485TransmitDisa();        // occasionally lose valid messages.  But we need to break out of junky ones.
        }
    }      // EO while (1)
}

byte readNodeSw(void)
{
    byte dipSw = 0x0F;
    if (NODEID_1) dipSw &= 0xE;
    if (NODEID_2) dipSw &= 0xD;
    if (NODEID_4) dipSw &= 0xB;
    if (NODEID_8) dipSw &= 0x7;
    return(dipSw);
}

    // ---------------------
void serialParse(void)
{
    char charCh;
    byte hexRead, oldHexRead;
  
    charCh = getChar();
    if (!charCh) return;

    if (sysStat & ST_DEBUG_MODE)        // Super commands for Monitor
    {
        rs485TransmitEna();
        putChar(charCh);

        switch(charCh)
        {
            case 'd':
                putStr("\tNode ID switch test.\n\r");
                putStr("\tHit any key to quit.\n\r");
                rs485TransmitDisa();
                while(1)
                {                  
                    charCh = getChar();
                    if (charCh) break;
                    hexRead = readNodeSw();
                    if (hexRead != oldHexRead)
                    {
                        rs485TransmitEna();
                        sprintf(ioBfr, "\n\r Node ID: %02Xh", hexRead);
                        putStr(ioBfr);
                        oldHexRead = hexRead;
                        rs485TransmitDisa();
                    }
                }
                break;

            case 'f':
                putStr("\tFlood sensor test: ");
                adcFloodSensor(ADC_LOQUACIOUS);
                break;

            case 'h':
                adcRHumid(ADC_LOQUACIOUS);
                break;

            case 'r':
                reportInfo();             
                break;

            case 't':
                adcTemper(ADC_LOQUACIOUS);
                break;

            case 'T':
                adcTemperSecondary(ADC_LOQUACIOUS);
                break;


            case '1':
                RELAY1 ^= 1;
                if (RELAY1) putStr("\tRelay 1 on");
                else putStr("\tRelay 1 off");
                break;

            case '2':
                RELAY2 ^= 1;
                if (RELAY2) putStr("\tRelay 2 on");
                else putStr("\tRelay 2 off");
                break;

            case '3':
                SwiPwr1 ^= 1;
                if (SwiPwr1) putStr("\tSwi Pwr1 on");
                else putStr("\tSwi Pwr1 off");
                break;

            case '4':
                SwiPwr2 ^= 1;
                if (SwiPwr2) putStr("\tSwi Pwr2 on");
                else putStr("\tSwi Pwr2 off");
                break;

            case '?':
                menuDisplay();                
                break;
        }                       // EO switch
        putPrompt();
        rs485TransmitDisa();
    }       // EO if debug
    else
    {
        childStateMach(charCh);
    }
}

    // ----------------------
void delay_us(unsigned T)   // TODO calibrate
{
    unsigned t;
    for (t=0; t<T; t++ ) ;
}

void hardwareInit(void)
{
        // Configure the device for maximum performance, but do not change the PBDIV clock divisor.
        // Given the options, this function will change the program Flash wait states,
        // RAM wait state and enable prefetch cache, but will not change the PBDIV.
        // The PBDIV value is already set via the pragma FPBDIV option above.
    SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

    PORTSetPinsAnalogIn(IOPORT_A, BIT_1 | BIT_0);
        // Could also use mPORTDSetPinsDigitalOut(BIT_6 | BIT_7);
    //PORTSetPinsDigitalOut(IOPORT_A, BIT_3 | BIT_2);
    PORTSetPinsDigitalOut(IOPORT_A, BIT_10 | BIT_9 | BIT_8 | BIT_7 | BIT_6 | BIT_5 | BIT_4 );

    PORTSetPinsAnalogIn(IOPORT_B, BIT_2 | BIT_1 | BIT_0);
    PORTSetPinsDigitalOut(IOPORT_B, BIT_15 | BIT_14 | BIT_13 | BIT_12 | BIT_11 | BIT_10 | BIT_9 | BIT_7 | BIT_4 | BIT_3);
    PORTSetPinsDigitalIn(IOPORT_B, BIT_8 | BIT_6 | BIT_5);

    PORTSetPinsDigitalOut(IOPORT_C, BIT_9 | BIT_7 | BIT_6 | BIT_3 | BIT_2 |BIT_1 | BIT_0);
    PORTSetPinsDigitalIn(IOPORT_C, BIT_8 | BIT_5 | BIT_4);

        // This is the waggle to set reprogrammable peripheral
        // Assume ints & DMA disa
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    U1RXR = 0b0001;     // U2.15 RPB6 as RxD1 (PICO) input
    RPB7R = 0b0001;     // U2.16 RPB7 as TxD1 (POCI) output
    SDI1R = 0b0011;     // U2.22 RPB11 is SDI1
    RPB8R = 0b0011;     // U2.17 RPB8 is SDO1
    SYSKEY = 0x33333333;    // Junk relocks it

    OSCCONbits.SOSCEN = 1;
    OSCCONbits.SOSCRDY = 1;
        // Serial port on UART1
        // Note, Mode & Sta have atomic bit clr, set, & inv registers
    U1MODEbits.ON = 1;      // Ena UART1, per UEN, with UTXEN
    U1MODEbits.SIDL = 0;    // Continue oper in idle mode
    U1MODEbits.IREN = 0;    // Disa IrDA
    U1MODEbits.RTSMD = 1;   // U1RTS_n pin in simplex mode
    U1MODEbits.UEN = 0b00;  // U1CTS_n & U1RTS_n unused
    U1MODEbits.WAKE = 0;    // Disa wakeup
    U1MODEbits.LPBACK = 0;  // Disa loopback
    U1MODEbits.ABAUD = 0;   // Disa autobaud
    U1MODEbits.RXINV = 0;   // U1Rx 0 for idle high
    U1MODEbits.BRGH = 0;    // Std speed 16x, 1: high speed is 4x baud clk
    U1MODEbits.PDSEL = 0b00;    // 8 bit data, no parity
    U1MODEbits.STSEL = 0;   // 1 stop bit

    U1STAbits.ADM_EN = 0;   // Disa automatic address mode detect
    U1STAbits.UTXISEL = 0b01;   // Interrupt gen when all chars transmitted
    U1STAbits.UTXINV = 0;  // U1Tx idle is 1.
    U1STAbits.URXEN = 1;    // Ena U1Rx
    U1STAbits.UTXBRK = 0;   // Disa Break (Start bit, then 12 0's, Stop)
    U1STAbits.UTXEN = 1;    // Ena U1Tx
    U1STAbits.URXISEL = 0b01;   // Interrupt flag asserted while buffer is 1/2 or more full
    U1STAbits.ADDEN = 0;    // Disa address mode

    U1BRG = 162;            // 80 for 19200 baud = PBCLK / (16 x (BRG + 1)), PBCLK = 25 MHz @ div / 1, Jitter 0.47%
                            // 162 for 9600, 325 for 4800

    RS485_DE = 0;
    RS485_RE_n = 0;

    AD1CON1bits.FORM = 0b000;   // Integer 16-bit, 10 lsb's
    AD1CON1bits.SSRC = 0b111;   // Internal ctr ends sampling and starte conver, auto.
    AD1CON1bits.ASAM = 1;       // Sampling begins immediately after last conversion completes; SAMP bit is automatically set
    AD1CON1bits.SAMP = 1;       // Ena sampling

    AD1CON2bits.VCFG = 0b001;   // Vref+ has bandgap ref, use Vss
    AD1CON2bits.OFFCAL = 0;     // Disa offset cal mode
    AD1CON2bits.CSCNA = 0;      // Disa scan the inputs
    AD1CON2bits.BUFM = 0;       // Buffer is one 16-bit word
    AD1CON2bits.ALTS = 0;       // Always use sample A input mux settings

    AD1CON3bits.ADRC = 1;       // ADC clk from FRC
    AD1CON3bits.SAMC = 0b11111; // 31 x TAD autosample time
    AD1CON3bits.ADCS = 0xFF;    // 512 x TPB = TAD, TPB from PBCLK

        // These don't matter if we scan.
    AD1CHSbits.CH0NB = 0;       // Sample B Ch 0 neg input is Vrefl
    AD1CHSbits.CH0SB = 0b00000; // Select B: AN0
    AD1CHSbits.CH0NA = 0;       // Sample A Ch 0 neg input is Vrefl
    AD1CHSbits.CH0SA = 0b00000; // Select B: AN0

    AD1CON1bits.ON = 1;         // This must be last step.

//    SPI1CONbits.MSTEN = 1;      // Master mode.  Shd be first command.
//    SPI1CONbits.FRMEN = 0;      // Framed SPI disa
//    SPI1CONbits.MSSEN = 0;      // Slave select SPI support disa
//    SPI1CONbits.MCLKSEL = 0;    // Use PBCLK for BRG
//    SPI1CONbits.ENHBUF = 0;     // Enhanced buffer disa
//    SPI1CONbits.ON = 0;         // Turn it off for now
//    SPI1CONbits.SIDL = 0;       // Continue in Idle
//    SPI1CONbits.DISSDO = 0;     // Use SDOx
//    SPI1CONbits.MODE16 = 0;     // 8 bit transfer
//    SPI1CONbits.MODE32 = 0;     // 8 bit transfer
//    SPI1CONbits.SMP = 0;        // Sample data in middle of phase
//    SPI1CONbits.CKE = 0;        // Serial output data changes on transition from idle clock state to active clock state
//    SPI1CONbits.SSEN = 0;       // Do not use /SS1 pin
//    SPI1CONbits.CKP = 0;        // Idle clock is low
//
//    SPI1CONbits.DISSDI = 0;     // Use SDI1 pin
//
//    SPI1CON2bits.AUDEN = 0;     // Audio protocol disa
//    SPI1BRG = 0x100;

   // spiIoxConfig();

    LED_GREEN = 0;
}

void menuDisplay(void)
{
    putStr("\n\r * Child / commands *\n\r");
    putStr(" d - rd DIPSWs\n\r");
    putStr(" f - flood sensor\n\r");
    putStr(" h - rd rhumid sen\n\r");
    putStr(" r - report info\n\r");
    putStr(" R - smem Read test\n\r");
    putStr(" t - rd T sen\n\r");
    putStr(" T - rd secondary T sen\n\r");
   //putStr(" W - smem destr. Wr test *\n\r");
    putStr(" 1 - toggle rly 1\n\r");
    putStr(" 2 - toggle rly 2\n\r");
    putStr(" 3 - toggle swi pwr1\n\r");
    putStr(" 4 - toggle swi pwr2\n\r");
    putStr(" ? - this menu\n\r");
    putStr("\n\r");
}

    // --------------------------------
void reportInfo(void)
{
    putPrompt();
    putStr("\n\r CCS Child Monitor, MLA");
    putStr("\n\r Ver: ");
    putStr(verStr);
    putStr("\n\r PNet ver: ");
    putChar(pnetVer);
    putStr("\n\r UID: ");
    putStr(uniqueID);
    sprintf(ioBfr, "\n\r Node: %02X", pnetNodeNumber);
    putStr(ioBfr);
    //putChar('0' + pnetNodeNumber);
    putStr("\n\r Sys st: ");
    putUns2Hex(sysStat);
    putStr("\n\r Relays: ");
    if (RELAY1) putChar('+'); else putChar('-');
    if (RELAY2) putChar('+'); else putChar('-');
    putStr("\n\r Switched Pwr: ");
    if (SwiPwr1) putChar('+'); else putChar('-');
    if (SwiPwr2) putChar('+'); else putChar('-');

    sprintf(ioBfr, "\n\r vRef: %4.03f V\r\n", vRef);
    putStr(ioBfr);

    adcTemper(ADC_SILENT);
    putStr("\n\r Temp now: ");
    sprintf(ioBfr, "%2.01foF [%4.03f]", temperNowF, temper1CalFactor);
    putStr(ioBfr);

    adcTemperSecondary(ADC_SILENT);
    putStr("\n\r Sec temp now: ");
    sprintf(ioBfr, "%2.01foF [%4.03f]",  temperSecondaryF, temper2CalFactor);
    putStr(ioBfr);

    adcRHumid(ADC_SILENT);
    putStr("\n\r Humid now: ");
    sprintf(ioBfr, "%2.01f [%4.03f]",rhumidNow, rhumidCalFactor);
    putStr(ioBfr);
    putStr("\n\r");
}

void systemInit(void)
{
    sysStat =  ST_TSEN1_OKAY + ST_TSEN2_OKAY + ST_HSEN_OKAY;
    RELAY1 = RELAY2 = SwiPwr1 = SwiPwr2 = 0;
}

 