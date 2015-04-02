/* File:        configs.h 
 * Purpose:     Low level config bit inits
 * Author:      Michael L Anderson
 * Contact:     MichaelLAndersonEE@gmail.com
 * Platform:    PIC32MX130F064B on DuraBlis Child v3 board
 * Created:     16 May 14
 */

    // Configuration Bits
#pragma config  PMDL1WAY = OFF          // Allow multiple reconfigurations
#pragma config  IOL1WAY = OFF           // Allow multiple reconfigurations
#pragma config  FUSBIDIO = OFF          // Controlled by Port Function
#pragma config  FPLLIDIV = DIV_4	// 4x PLL Input Divider
#pragma config  FPLLMUL = MUL_20	// 20x PLL Multiplier
#pragma config  FPLLODIV = DIV_8	// System PLL Output Divide by 8
#pragma config  FNOSC = PRI             // Primary Osc (XT,HS,EC)
#pragma config  FSOSCEN = OFF           // Sec osc disabled
#pragma config  IESO = OFF              // Internal/External Switch Over Disabled
#pragma config  POSCMOD = HS            // Pri osc HS osc mode
#pragma config  OSCIOFNC = OFF          // LKO Output Signal Active on the OSCO Pin Disabled
#pragma config  FPBDIV = DIV_1          // Peripheral clock divisor Pb_Clk is Sys_Clk/2
#pragma config  FCKSM = CSDCMD          // Clock Switch Disable, FSCM Disabled
#pragma config  WDTPS = PS128           // WDT postscaler 1:128
#pragma config  WINDIS = ON             // Watchdog Timer is in Window Mode
#pragma config  FWDTEN = OFF            // WDT Disabled (SWDTEN Bit Controls)
#pragma config  FWDTWINSZ = WINSZ_50	// WDT Window Size is 50%
#pragma config  DEBUG = OFF             // Background Debugger is Disabled
#pragma config  JTAGEN = OFF            // JTAG Disabled
#pragma config  ICESEL = ICS_PGx2       // Communicate on PGEC2/PGED2
#pragma config  PWP = PWP2K             // Program Flash Write Protect, First 2K
#pragma config  BWP = OFF               // Boot Flash write Protection Disabled
#pragma config  CP = OFF                // Code Protection Disabled