//---------------------------------------------------------------------------


#pragma hdrstop

#include "Sha_x.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

const unsigned char SHA1DER[15]=  {0x30,0x21,0x30,0x09,0x06,0x05,0x2b,0x0e,0x03,0x02,0x1a,0x05,0x00,0x04,0x14};
const unsigned char SHA256DER[19]={0x30,0x31,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,0x00,0x04,0x20};
unsigned long HashFunction1=1;

SHA_X::SHA_X()
{
	// The constructor does nothing
}                

SHA_X::~SHA_X()
{
	// The destructor does nothing
}

unsigned long SHA_X::GetDER(unsigned char** DER)
{
	unsigned long Size;
	switch(HashFunction1)
	{
		case 1:
			*DER = (unsigned char*)&SHA1DER;
			Size = sizeof(SHA1DER);
			break;
		case 2:
			*DER=(unsigned char*)&SHA256DER;
			Size = sizeof(SHA256DER);
			break;
			default:
			for(;;);
	}
	return Size;
}


void SHA_X::Reset(unsigned long FunctionId)
{
   HashFunction1=FunctionId;
    switch(HashFunction1)
    {
        case 1:
            Sha1.Reset();
          break;
        case 2:
            sha_init(&Sha256);
          break;
        default:
        for(;;);
    }
}

bool SHA_X::Result(unsigned *message_digest_array)
{
    switch(HashFunction1)
    {
        case 1:
            Sha1.Result(message_digest_array);
          break;
        case 2:
            sha_done(&Sha256, message_digest_array);
          break;
        default:
        for(;;);
    }
	return true;
}

void SHA_X::ScatterInput(const unsigned char *message_array,
						unsigned length,
						unsigned ScatterDistance)
{
}

void SHA_X::Input(const unsigned char *message_array,
					unsigned length)
{
    switch(HashFunction1)
    {
        case 1:
            Sha1.Input(message_array,length);
          break;
        case 2:
            sha_process(&Sha256, message_array, length);
          break;
        default:
        for(;;);
    }
}

void SHA_X::Input(const char *message_array,
				unsigned length)
{
	Input((unsigned char *) message_array, length);
}

