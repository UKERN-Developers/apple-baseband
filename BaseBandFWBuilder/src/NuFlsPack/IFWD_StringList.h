/*---------------------------------------------------------------------------*/
#ifndef IFWD_StringListH
#define IFWD_StringListH
/*---------------------------------------------------------------------------*/


class IFWD_StringList
{
private:

  typedef struct
  {
    char *text;
    void *object;
  } SListItem;

  class TLocalStrings
  {
    public:
    SListItem *Items;
    int  Count;
    char * operator[](int index);
    TLocalStrings();
  };

  class TLocalObjects
  {
    public:
    SListItem *Items;
    int  Count;
    void * operator[](int index);
    TLocalObjects();
  };

  int NofAllocated;
  char m_Sorted;
  int m_update;
  SListItem *Items;

/*---------------------------------------------------------------------------*/


public:
  int  Count;
  char CaseSensitive;
  char CRLF; /* Set to 1, if lines should end on "\r\n" (MS-DOS). 0 means the lines will be ended with "\n" (used by "SaveToFile()") */

  void BeginUpdate(void); /* When sorting it on: having a begin/end to encapsolate the "Add()" calls will increase the speed greatly */
  void EndUpdate(void);  

  void Clear(void);
  void SetSort(char sorted);
  void Sort(void); /* sorts the strings, but does not turn sorting on */
  int  Add(char *text);
  int  AddObject(char *text, void *object);
  int  IndexOf(char *text);
  char Find(char *S, int &Index);
  void Delete(int index);
  char LoadFromFile(char *FileName);
  void SaveToFile(char *FileName);

  TLocalStrings Strings;
  TLocalObjects Objects;

  void (*OnDelete)(void *object);  /* External callback to handle object deletion (in case memory needs deallocation) */
  IFWD_StringList();
  ~IFWD_StringList();
};

/*---------------------------------------------------------------------------*/

#endif

// KeymapKey* __fastcall operator[](int index) {return m_keys[index];};


