/* File:   adc.c  Analog functions low and high level
 * Author: Michael L Anderson
 * Contact: MichaelLAndersonEE@gmail.com
 * Platform: PIC32MX130F064B on DuraBlis Child board, ver 3
 * Created: 10 Jul 14
 */

#include <xc.h>
#include "DuraBlisChild.h"
#include "adc.h"
#include "serial.h"

#define NUM_SA   16
#define T1_SEN_STDDEV_MAX    2           // Std dev in ADC cts
#define T2_SEN_STDDEV_MAX    4           // T2 on cable and no filtering
#define RH_SEN_STDDEV_MAX   4           // Honeywell sensor is skittish
#define FLOOD_SEN_STDDEV_MAX    5       // High Z, high noise
#define FLOOD_SEN_MIN_V     0.03        // With 1 MOhm pulldown
#define FLOOD_THRESHOLD_V   0.8         // Based on empirical testing with tap & bottled water.
#define FLOOD_TIME_IN       500         // Flood reqs time to settle down

static unsigned adcUrConvert(byte chan);
static double adcMeanConvert(byte chan, byte opMode);
static double sqrtBabylon(double S);
static double sigmaLastADC;

    // TODO set/clr error bits
void adcRHumid(byte opMode)
{
    extern double rhumidNow, rhumidCalFactor, vRef;
    extern byte sysStat;
    double adcV;
        // Honeywell HIH5030
        // TODO Sensor has temperature dependence and slight hysteresis

    if (opMode == ADC_LOQUACIOUS) putStr("\tRHumid hex vals:\r\n");

    adcV = adcMeanConvert(ANCH_RELHUM, opMode);
    adcV = adcV * 3.3 / vRef;               // Ratiometric device, calibrated at nominal 3.3 V
   // rhumidNow = 47.059 * adcV - 23.529;        // Inferred from Funnysmell datasheet
    rhumidNow = 50 * adcV - 25;        // Inferred from Funnysmell datasheet
    rhumidNow *= rhumidCalFactor;

    sysStat &= ~ST_HSEN_OKAY;
    if (sigmaLastADC < RH_SEN_STDDEV_MAX && adcV > 0.2) sysStat |= ST_HSEN_OKAY;

    if (opMode == ADC_LOQUACIOUS)
    {
        putStr("\r\n\tRel humidity: ");
        sprintf(ioBfr, "%3.02f %% (%3.02fV)", rhumidNow, adcV);
        putStr(ioBfr);
        putStr("\n\r");
    }
}

    // Microchip MCP9700A
void adcTemper(byte opMode)
{
    extern double temperNowF, temper1CalFactor;
    extern byte sysStat;
    double adcV;

    if (opMode == ADC_LOQUACIOUS) putStr("\tTemp sen hex vals:\r\n");
       
    adcV = adcMeanConvert(ANCH_TEMPER, opMode);
    temperNowF = 180 * adcV - 58;
    temperNowF *= temper1CalFactor;

    sysStat &= ~ST_TSEN1_OKAY;
    if (sigmaLastADC < T1_SEN_STDDEV_MAX && adcV > 0.05) sysStat |= ST_TSEN1_OKAY;

    if (opMode == ADC_LOQUACIOUS)
    {
        putStr("\r\n\tTemperature: ");
        sprintf(ioBfr, "%3.02f oF (%3.02fV)", temperNowF, adcV);
        putStr(ioBfr);
        putStr("\n\r");
    }
}

    // Microchip MCP9700A on a cable.
void adcTemperSecondary(byte opMode)
{
    extern double temperSecondaryF, temper2CalFactor;
    extern byte sysStat;
    double adcV;

    if (opMode == ADC_LOQUACIOUS) putStr("\tTemp2 sen hex vals:\r\n");

    adcV = adcMeanConvert(ANCH_TEMPER2, opMode);
    temperSecondaryF = 180 * adcV - 58;
   temperSecondaryF *= temper2CalFactor;

    sysStat &= ~ST_TSEN2_OKAY;
    if (sigmaLastADC < T2_SEN_STDDEV_MAX && adcV > 0.05) sysStat |= ST_TSEN2_OKAY;

    if (opMode == ADC_LOQUACIOUS)
    {
        putStr("\r\n\tSecondary temperature: ");
        sprintf(ioBfr, "%3.02f oF (%3.02fV)", temperSecondaryF, adcV);
        putStr(ioBfr);
        putStr("\n\r");
    }
}

    // Return 1 for flood threshold, 0 for dry, neg code on error
int adcFloodSensor(byte opMode)
{
    static unsigned timeIn = FLOOD_TIME_IN;
    double adcV;

    adcV = adcMeanConvert(ANCH_FLOOD, opMode);
    if (opMode == ADC_LOQUACIOUS) putStr("\r\n\tFlood sensor: ");
    if (timeIn)
    {
        timeIn--;     
        if (opMode == ADC_LOQUACIOUS) putStr("settling...\r\n");
        return(0);    // Hi Z sensor needs time to settle down
    }
    if (sigmaLastADC > FLOOD_SEN_STDDEV_MAX)
    {
        if (opMode == ADC_LOQUACIOUS) putStr("Fail: too noisy\r\n");
        return(-1);
    }

//    if (adcV < FLOOD_SEN_MIN_V)
//    {
//        if (opMode == ADC_LOQUACIOUS) putStr("Fail: low min V\r\n");
//        return(-2);
//    }

    if (opMode == ADC_LOQUACIOUS)
    {       
        sprintf(ioBfr, "%3.02f V\n\r", adcV);
        putStr(ioBfr);
    }

    if (adcV > FLOOD_THRESHOLD_V)
    {
        if (opMode == ADC_LOQUACIOUS) putStr("\r\n 'How high's the water, mama?'\n\r");
        return(1);
    }
    else return(0);
}

unsigned adcUrConvert(byte chan)
{
    AD1CON1bits.SAMP = 0;       // End sampling & start conver
    AD1CHSbits.CH0NA = 0;       // Sample A Ch 0 neg input is Vrefl
    AD1CHSbits.CH0SA = chan;    // A mux <- chan
    AD1CSSL = 0;                // Write to scan buff 0

    AD1CON1bits.SAMP = 1;       // Start sampling
    delay_us(10);
    AD1CON1bits.SAMP = 0;
    while(!(AD1CON1bits.DONE)) ;    // Wait
    return(ADC1BUF0);
}

double adcMeanConvert(byte chan, byte opMode)
{
    extern double vRef;
    byte sa;
    double retVal = 0.0, vMean = 0.0,  errSqr = 0.0;
    unsigned samp, minSa = 0xFFFF, maxSa = 0;
    unsigned uBfr[NUM_SA];

    if (chan == ANCH_FLOOD)
    {
        adcUrConvert(chan);         // Hi Z sensor is particularly bad
        delay_us(10);
        adcUrConvert(chan);            
        delay_us(10);
        adcUrConvert(chan);             
        delay_us(10);
    }

    adcUrConvert(chan);             // First is often worst
    delay_us(10);
    
    for (sa = 0; sa < NUM_SA; sa++)
    {
        delay_us(10);
        samp = adcUrConvert(chan);
        uBfr[sa] = samp;
        if (samp < minSa) minSa = samp;
        if (samp > maxSa) maxSa = samp;
    }

    for (sa = 0; sa < NUM_SA; sa++)
    {
        if (opMode == ADC_LOQUACIOUS)
        {
            sprintf(ioBfr, "\n\r  %d", (unsigned) uBfr[sa]); // DEB
            putStr(ioBfr);
        }
        vMean += uBfr[sa];
    }

    vMean -= minSa;
    vMean -= maxSa;
    vMean = vMean / (NUM_SA - 2);
    if (opMode == ADC_LOQUACIOUS)
    {
        sprintf(ioBfr, "\n\r\tADC mean: %3.02f\n\r", vMean);
        putStr(ioBfr);
    }

    for (sa = 0; sa < NUM_SA; sa++)     // This is a little off since I am leaving
    {                                   // outliers in.
        errSqr += (vMean - uBfr[sa]) * (vMean - uBfr[sa]);
    }
    errSqr /= (NUM_SA - 1);            // Bessel corrected
    sigmaLastADC = sqrtBabylon(errSqr);

    if (opMode == ADC_LOQUACIOUS)
    {
        sprintf(ioBfr, "\tsigma %5.04f, Sa = %d\n\r", sigmaLastADC, NUM_SA - 2);
        putStr(ioBfr);
    }
    //if (adcInternalCal < 0.5 || adcInternalCal > 1.5) { sprintf(ioBfr, "\t* aIC range err: %4.03f *\n\r", adcInternalCal); putStr(ioBfr); }
    //if (vRef < 2.8 || vRef > 3.3) { sprintf(ioBfr, "\t* vRef range err: %4.03f *\n\r", vRef); putStr(ioBfr); }
    retVal = vRef * vMean / 1023;

    return(retVal);
}

double sqrtBabylon(double S)
{
    byte i;
    double xn0, xn1;
    if (S <= 0.00001) return (0.0);
    xn0 = S;
    for(i = 0; i < 10; i++)
    {
        xn1 = 0.5 * (xn0 + (S / xn0));
        xn0 = xn1;
    }
    return(xn1);
}

//    // Return mean voltage normalized to 3.3 V
//    // Takes NUM_SA samples, throws out highest and lowest, compute mean.
//double adcMeanConvert(byte chan, byte opMode)
//{
//    extern double vRef;
//    byte sa;
//    double retVal = 0.0;
//    unsigned uBfr[NUM_SA], samp, minSa = 0xFFFF, maxSa = 0;
//
//    for (sa = 0; sa < NUM_SA; sa++)
//    {
//        delay_us(10);
//        samp = adcUrConvert(chan);
//        uBfr[sa] = samp;
//        if (samp < minSa) minSa = samp;
//        if (samp > maxSa) maxSa = samp;
//    }
//
//    for (sa = 0; sa < NUM_SA; sa++)
//    {
//        if (opMode == ADC_LOQUACIOUS)
//        {
//            putNib2Hex(uBfr[sa] >> 8);
//            putNib2Hex(uBfr[sa] >> 4);
//            putNib2Hex(uBfr[sa]);
//            putStr("\r\n");
//        }
//        retVal += uBfr[sa];
//    }
//
//    retVal -= minSa;
//    retVal -= maxSa;
//
//    retVal /= (NUM_SA - 2);
//    retVal = vRef * retVal / 1023;
//
//        // DEB
////    sprintf(ioBfr, " %4.03f\t", retVal);
////    putStr(ioBfr);
//
//    return(retVal);
//}
