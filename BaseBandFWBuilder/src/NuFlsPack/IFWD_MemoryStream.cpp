/*---------------------------------------------------------------------------*/
/* Borland C++ TMemoryStream Class substitution                              */
/*---------------------------------------------------------------------------*/

#include "IFWD_MemoryStream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
IFWD_MemoryStream::IFWD_MemoryStream()
{
  Size = 0;
  m_Memory = NULL;
  Memory = NULL;
}
/*---------------------------------------------------------------------------*/

IFWD_MemoryStream::~IFWD_MemoryStream()
{
  Clear();
}
/*---------------------------------------------------------------------------*/
void IFWD_MemoryStream::Clear(void)
{
  if(m_Memory)
  {
    delete []m_Memory;
    m_Memory = NULL;
    Memory = NULL;
    Size = 0;         
  }
}
/*---------------------------------------------------------------------------*/

void IFWD_MemoryStream::SetSize(int size)
{
  if(size <= 0)
    Clear();
  else
  {
    if(size != Size) // any difference?
    {
      char *NewMemory = new char[size + 1]; // Allocate memory for new size
      if(m_Memory) // any old memory data to copy?
      {
        int copyBytes;
        if(size > Size) // new size bigger than old?
        {
          copyBytes = Size;
          memset(NewMemory+Size,0,size-Size); // clear all of new memory that we are not copying from old memory block
        }
        else
          copyBytes = size;
        memcpy(NewMemory,m_Memory,copyBytes); // copy as much of the old data as possible
      }
      else
        memset(NewMemory,0,size);
      delete []m_Memory;
      Size = size;
      m_Memory = NewMemory;
      Memory = (void*)m_Memory;
    }
  }
}
/*---------------------------------------------------------------------------*/

void IFWD_MemoryStream::LoadFromFile(char *FileName)
{
  FILE *in;
  Clear();
  if((in = fopen(FileName,"rb")) == NULL)
    return;
  fseek(in, 0L, SEEK_END);
  Size = ftell(in);  // get file length
  fseek(in, 0L, SEEK_SET); // go to beginning of file again
  m_Memory = new char[Size+1]; // Allocate memory for file
  fread(m_Memory,1,Size,in); // read it
  Memory = (void*)m_Memory;
  fclose(in);
}

/*---------------------------------------------------------------------------*/
bool IFWD_MemoryStream::SaveToFile(char *FileName)
{
  FILE *out;
  if((out = fopen(FileName,"wb")) == NULL)
    return false;
  if(m_Memory)
    fwrite(m_Memory,1,Size,out); // write to file
  fclose(out);

  return true;
}
/*---------------------------------------------------------------------------*/


