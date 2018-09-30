/*---------------------------------------------------------------------------*/
#ifndef IFWD_MemoryStreamH
#define IFWD_MemoryStreamH
/*---------------------------------------------------------------------------*/


class IFWD_MemoryStream
{
private:
  char *m_Memory;

public:
  int  Size;
  void *Memory;
  void SetSize(int size);
  void Clear(void);

  void LoadFromFile(char *FileName);
  bool SaveToFile(char *FileName);

  IFWD_MemoryStream();
  ~IFWD_MemoryStream();
};

/*---------------------------------------------------------------------------*/

#endif