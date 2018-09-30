//---------------------------------------------------------------------------

#ifndef _SHAX_H_
#define _SHAX_H_
#include "sha1.h"
#include "sha256.h"

class SHA_X
{

	public:

		SHA_X();
		virtual ~SHA_X();
      
		/*
		 *	Get pointer to DER coding of algorithm (and length)
		 */
      unsigned long GetDER(unsigned char** DER);

		/*
		 *	Re-initialize the class
		 */
		void Reset(unsigned long FunctionId);

		/*
		 *	Returns the message digest
		 */
		bool Result(unsigned *message_digest_array);

		/*
		 *	Provide input to SHA_X
		 */
		void ScatterInput(	const unsigned char	*message_array,
                  unsigned			length,
                  unsigned       ScatterDistance);
       
		void Input(	const unsigned char	*message_array,
					unsigned			length);
		void Input(	const char	*message_array,
					unsigned	length);

	private:
      SHA1 Sha1;
      sha_state Sha256;
	
};

#endif
