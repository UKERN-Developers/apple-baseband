/*---------------------------------------------------------------------------*/
/* ANSI C++ TStringList Class substitution                                   */
/*---------------------------------------------------------------------------*/

#include "IFWD_StringList.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*---------------------------------------------------------------------------*/

#define CLIST_CHUNK_SIZE 100 // don't allocate a new list every time a new item is added
#define MAX_LINE_LENGTH 4000

/*---------------------------------------------------------------------------*/
/* For some reason they left out "strcmpi()" out of the ANSI standard */
static int ANSI_strcmpi(char *s1, char *s2)
{
  int diff = 0;
  while(diff == 0 && *s1 && *s2)
    diff = toupper(*s1++) - toupper(*s2++);
  return (diff == 0) ? (*s1 - *s2) : diff;
}
/*---------------------------------------------------------------------------*/
IFWD_StringList::IFWD_StringList()
{
  Count = 0;
  NofAllocated = 0;
  CaseSensitive = 1;
  OnDelete = NULL;
  m_Sorted = 0;
  m_update = 0;
  Items = NULL;
  CRLF = 1; // default MS-DOS lines on save
}
/*---------------------------------------------------------------------------*/
IFWD_StringList::~IFWD_StringList()
{
  Clear();
}
/*---------------------------------------------------------------------------*/

#ifdef __BORLANDC__
#define BSEARCH_CALLING_CONVENTION  _USERENTRY 
#else
#define BSEARCH_CALLING_CONVENTION 
#endif




static int BSEARCH_CALLING_CONVENTION SListItem_compare( const void *a, const void *b)
{
  typedef struct
  {
    char *text;
    void *object;
  } SListItem;

  char *p1 = ((SListItem*)a)->text, *p2 = ((SListItem*)b)->text;
  if(p1 && p2)  /* do we have to valid pointers ? */
    return strcmp(p1,p2);
  if(p1 == p2) /* Okay one of them is NULL, so compare on pointer addresses */
    return 0;
  return (p1 > p2) ? 1 : -1;
}
/*---------------------------------------------------------------------------*/
static int BSEARCH_CALLING_CONVENTION SListItem_comparei( const void *a, const void *b)
{
  typedef struct
  {
    char *text;
    void *object;
  } SListItem;

  char *p1 = ((SListItem*)a)->text, *p2 = ((SListItem*)b)->text;
  if(p1 && p2) /* do we have to valid pointers ? */
    return ANSI_strcmpi(p1,p2);
  if(p1 == p2)  /* Okay one of them is NULL, so compare on pointer addresses */
    return 0;
  return (p1 > p2) ? 1 : -1;
}
/*---------------------------------------------------------------------------*/

int IFWD_StringList::AddObject(char *text, void *object)
{
  char *p = NULL;
  int new_pos;
  if(Count + 1 >= NofAllocated) /* not enough room one more? */
  {
    SListItem *tmp_Items;
    NofAllocated = Count + CLIST_CHUNK_SIZE;

    tmp_Items = new SListItem[NofAllocated];
    if(Items && Count)
    {
      memcpy(tmp_Items, Items, Count * sizeof( SListItem)); /* copy old items over to new */
      delete []Items;
    }
    Items = tmp_Items;
    Strings.Items = Items;
    Objects.Items = Items;
  }
  /* add new: */
  Items[Count].object = object;
  if(text)
  {
    int n = strlen(text);
    p = new char[n + 1];
    strcpy(p,text);
  }
  Items[Count].text = p;
  new_pos = Count;
  Count++;
  Strings.Count = Count;
  Objects.Count = Count;
  if(m_Sorted && m_update == 0)
  {
    Sort();
    // go look for the newly added object, so we can return the correct index of it - slow, but
    // needed to be compatable!
    SListItem *curr = Items;
    if(curr)
    {
      int n;
      for(n = 0 ; n < Count ; n++)
      {
        if(curr->text == p && curr->object == object) // check the pointer, not contents and object (this may give the wrong result if (NULL,NULL) is added)
          return n;
        p++;
      }
    }
  }
  return new_pos;
}
/*---------------------------------------------------------------------------*/
int IFWD_StringList::Add(char *text)
{
  return AddObject(text, NULL);
}
/*---------------------------------------------------------------------------*/
void IFWD_StringList::BeginUpdate(void)
{
  m_update++;
}
/*---------------------------------------------------------------------------*/
void IFWD_StringList::EndUpdate(void)
{
  m_update--;
  if(m_update <= 0)
  {
    m_update = 0;
    if(m_Sorted)
      Sort();
  }
}

/*---------------------------------------------------------------------------*/
char IFWD_StringList::Find(char *S, int &Index)
{
  if(m_Sorted == 0) /* Not sorted? Then do linear search: */
  {
    int result = IndexOf(S);
    if(result >= 0)
    {
      Index = result;
      return 1;
    }
  }
  else
  {
    SListItem *p = Items;
    if(p)
    {
      SListItem key, *found;
      key.text = S;
      key.object = NULL;
      if(CaseSensitive)
        found = (SListItem*)bsearch(&key, p, Count, sizeof(SListItem), SListItem_compare);
      else
        found = (SListItem*)bsearch(&key, p, Count, sizeof(SListItem), SListItem_comparei);
      if(found)
      {
        Index = ((unsigned long)found - (unsigned long)p) / sizeof(SListItem); /* calculate index from pointer to found item */
        return 1;
      }
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/

int IFWD_StringList::IndexOf(char *text)
{
  if(text)
  {
    SListItem *p = Items;
    if(p)
    {
      int n;
      if(CaseSensitive)
      {
        for(n = 0 ; n < Count ; n++)
        {
          if(p->text)
          {
            if(strcmp(p->text, text) == 0) /* found? */
              return n;
          }
          p++;
        }
      }
      else
      {
        for(n = 0 ; n < Count ; n++)
        {
          if(p->text)
          {
            if(ANSI_strcmpi(p->text, text) == 0) /* found? */
              return n;
          }
          p++;
        }
      }
    }
  }
  return -1; /* indicate not found */
}
/*---------------------------------------------------------------------------*/

void IFWD_StringList::Clear(void)
{
  if(NofAllocated)
  {
    SListItem *p = Items;
    if(p)
    {
      int n = Count;
      while(n-- > 0)
      {
        if(OnDelete && p->object != NULL)
          OnDelete(p->object);
        if(p->text)
        {
          delete []p->text;
          p->text = NULL;
        }
        p++;
      }
      delete []Items;
      Items = NULL;
      Strings.Items = NULL;
      Objects.Items = NULL;
    }
    NofAllocated = 0;
    Count = 0;
    Strings.Count = 0;
    Objects.Count = 0;
  }
}

/*---------------------------------------------------------------------------*/
void IFWD_StringList::Delete(int index)
{
  if(index >= 0 && index < Count)
  {
    SListItem *p = &Items[index];
    if(OnDelete && p->object != NULL)
    {
      OnDelete(p->object);
      p->object = NULL;
    }
    if(p->text)
    {
      delete []p->text;
      p->text = NULL;
    }
    if(index < Count - 1) /* not deleting the last? ...then move list */
      memmove(p,p+1,sizeof(SListItem)*(Count - index - 1));
    Count--; /* we have one less valid item */
    Strings.Count = Count;
    Objects.Count = Count;
  }
}
/*---------------------------------------------------------------------------*/
void IFWD_StringList::SetSort(char sorted)
{
  if(m_Sorted != sorted)
  {
    m_Sorted = sorted;
    if(m_Sorted)
      Sort();
  }
}

/*---------------------------------------------------------------------------*/

char IFWD_StringList::LoadFromFile(char *FileName)
{
  FILE *in;
  char done = 0;

  Clear();
  if((in = fopen(FileName, "rb")) != NULL)
  {
    char *line_buf = new char[MAX_LINE_LENGTH + 2];
    int len;

    BeginUpdate();
    while(!done)
    {
      line_buf[0]=0;
      if(fgets(line_buf, MAX_LINE_LENGTH, in))
      {
        line_buf[MAX_LINE_LENGTH] = 0; /* Make sure it's null-terminated */
        len = strlen(line_buf);
        if(line_buf[len-1] == '\n')
        {
          line_buf[len-1] = 0; /* Remove LF character  */
          if(line_buf[len-2] == '\r')
            line_buf[len-2] = 0; /* Remove CR character  */
        }
        Add(line_buf);
      }
      else
        done  = 1;
    }
    fclose(in);
    delete []line_buf;
    EndUpdate();
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/

void IFWD_StringList::SaveToFile(char *FileName)
{
  FILE *out;
  if((out = fopen(FileName, "wb")) != NULL)
  {
    SListItem *p = Items;
    if(p)
    {
      int n = Count;
      if(CRLF)
      {
        while(n-- > 0)
        {
          fprintf(out,"%s\r\n",p->text);
          p++;
        }
      }
      else
      {
        while(n-- > 0)
        {
          fprintf(out,"%s\n",p->text);
          p++;
        }
      }
    }
    fclose(out);
  }
}
/*---------------------------------------------------------------------------*/

void IFWD_StringList::Sort(void)
{
  if(CaseSensitive)
    qsort((void *)Items, Count, sizeof(SListItem), SListItem_compare);
  else
    qsort((void *)Items, Count, sizeof(SListItem), SListItem_comparei);
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
IFWD_StringList::TLocalObjects::TLocalObjects()
{
  Count = 0;
}
/*---------------------------------------------------------------------------*/

void *IFWD_StringList::TLocalObjects::operator[](int index)
{
  if(index >= 0 && index < Count)
    return Items[index].object;
  return NULL;
}
/*---------------------------------------------------------------------------*/

IFWD_StringList::TLocalStrings::TLocalStrings()
{
  Count = 0;
}
/*---------------------------------------------------------------------------*/

char *IFWD_StringList::TLocalStrings::operator[](int index)
{
  if(index >= 0 && index < Count)
    return Items[index].text;
  return NULL;
}
/*---------------------------------------------------------------------------*/