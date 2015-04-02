/* File:   spi_iox.h  SPI based functions for MCP23SO8 IO Expander
 * Author: Michael L Anderson
 * Contact: MichaelLAndersonEE@gmail.com
 * Platform: PIC32MX130F016H on BW-CCS-Child   board.
 * Created: 18 Aug 14
 */

#ifndef SPI_IOX_H
#define	SPI_IOX_H

#ifdef	__cplusplus
extern "C" {
#endif

void spiIoxConfig(void);
byte spiIoxRead(void);


#ifdef	__cplusplus
}
#endif

#endif	/* SPI_IOX_H */

