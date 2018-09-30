/*
 *  RegisterAccessorLib.h
 *  RegisterAccessor
 *
 *  Created by Arjuna Sivasithambaresan on 2/15/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */

#include "RegisterAccessorIO.h"

#include <string>
#include <sys/ioctl.h>

 
class RAInterface
{
	public:
		class Exception: public std::exception 
		{
		private:
			char		fMessage[256];
		
		public:
			Exception( const char * fmt, ... );
			Exception( const Exception & e );
			
		public:
			virtual const char * what( void ) const throw();
		};

	private:
		static const std::string  kDevicePath;
		
	private:
		int			fFD;
		
	private:
		void ioctl( unsigned what, void * data ) const;
		
	public:
		RAInterface();
		~RAInterface();
	
		void connect( void );
		void disconnect( void );

	public:
		inline unsigned read( unsigned reg ) const
		{
			ioctl( RAREAD, &reg );
			return reg;
		}
	
		inline void write( unsigned reg, unsigned value ) const
		{
			RAWriteInfo info;
			info.addr = reg;
			info.value = value;
			ioctl( RAWRITE, &info );
		}
		
		inline void writeMasked( unsigned reg, unsigned value, unsigned mask ) const
		{
			RAMaskedWriteInfo info;
			info.addr = reg;
			info.value = value;
			info.mask = mask;

			ioctl( RAMASKEDWRITE, &info );
		}
 
};

