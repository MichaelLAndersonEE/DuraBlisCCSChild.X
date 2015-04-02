/* File:   adc.h  Analog functions 
 * Author: Michael L Anderson
 * Contact: MichaelLAndersonEE@gmail.com
 * Platform: PIC32MX130F064B on DuraBlis Child board, ver3.
 * Created: 11 Jul 14
 */

#ifndef ADC_H
#define	ADC_H

#ifdef	__cplusplus
extern "C" {
#endif

#define ADC_SILENT      0x01
#define ADC_LOQUACIOUS  0x02

void adcRHumid(byte);
void adcTemper(byte);
void adcTemperSecondary(byte);
int  adcFloodSensor(byte opMode);

#ifdef	__cplusplus
}
#endif

#endif	/* ADC_H */

