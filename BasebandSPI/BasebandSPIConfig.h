#ifndef _BASEBANDSPICONFIG_H_
#define _BASEBANDSPICONFIG_H_

#define SPI_GM_TASK					1

#define SPI_HANDLE_DMA_NOTIFY		0
#define SPI_USE_REAL_ISR			0
#define SPI_USE_REAL_ISR_FOR_DEBUG	SPI_GM_TASK

#define SPI_PEDANTIC_ASSERT			0

#define SPI_SRDY_DEBUG				1

#include <IOKit/IOLib.h>

typedef struct BasebandSPIConfig {
	UInt32 spiChipSelect;
	UInt32 spiClockPeriod;
	
	UInt8 spiClockPolarity;
	UInt8 spiClockPhase;
	UInt8 spiBitFirst;
	UInt8 spiWordSize;
	
	UInt32 spiWordDelay;
	UInt32 spiCSDelay;
	
	UInt8 spiBigEndian;
	UInt8 reservedByte1;
	UInt8 reservedByte2;
	UInt8 reservedByte3;
	
	UInt32 reserved[2];
};

#endif
