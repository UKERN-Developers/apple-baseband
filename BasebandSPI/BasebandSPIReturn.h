#include <IOKit/IOReturn.h>

#ifndef _BASEBANDSPIRETURN_H_
#define _BASEBANDSPIRETURN_H_

#define kBasebandSPIReturnTransferActive		iokit_vendor_specific_err(0x1)
#define kBasebandSPIReturnAlreadyQueued			iokit_vendor_specific_err(0x2)
#define kBasebandSPIReturnHardwareIncompatible	iokit_vendor_specific_err(0x3)

#endif
