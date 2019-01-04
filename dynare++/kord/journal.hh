// Copyright 2004, Ondra Kamenik

// Resource usage journal

#ifndef JOURNAL_H
#define JOURNAL_H

#include "int_sequence.h"

#include <sys/time.h>
#include <cstdio>
#include <iostream>
#include <fstream>

class SystemResources
{
  timeval start;
public:
  SystemResources();
  static long int pageSize();
  static long int physicalPages();
  static long int onlineProcessors();
  static long int availableMemory();
  void getRUS(double &load_avg, long int &pg_avail, double &utime,
              double &stime, double &elapsed, long int &idrss,
              long int &majflt);
};

struct SystemResourcesFlash
{
  double load_avg;
  long int pg_avail;
  double utime;
  double stime;
  double elapsed;
  long int idrss;
  long int majflt;
  SystemResourcesFlash();
  void diff(const SystemResourcesFlash &pre);
};

class Journal : public ofstream
{
  int ord;
  int depth;
public:
  Journal(const char *fname)
    : ofstream(fname), ord(0), depth(0)
  {
    printHeader();
  }
  ~Journal()
  {
    flush();
  }
  void printHeader();
  void
  incrementOrd()
  {
    ord++;
  }
  int
  getOrd() const
  {
    return ord;
  }
  void
  incrementDepth()
  {
    depth++;
  }
  void
  decrementDepth()
  {
    depth--;
  }
  int
  getDepth() const
  {
    return depth;
  }
};

#define MAXLEN 1000

class JournalRecord;
JournalRecord&endrec(JournalRecord &);

class JournalRecord
{
protected:
  char recChar;
  int ord;
public:
  Journal &journal;
  char prefix[MAXLEN];
  char mes[MAXLEN];
  SystemResourcesFlash flash;
  typedef JournalRecord & (*_Tfunc)(JournalRecord &);

  JournalRecord(Journal &jr, char rc = 'M')
    : recChar(rc), ord(jr.getOrd()), journal(jr)
  {
    prefix[0] = '\0'; mes[0] = '\0'; writePrefix(flash);
  }
  virtual ~JournalRecord()
  {
  }
  JournalRecord &operator<<(const IntSequence &s);
  JournalRecord &
  operator<<(_Tfunc f)
  {
    (*f)(*this); return *this;
  }
  JournalRecord &
  operator<<(const char *s)
  {
    strcat(mes, s); return *this;
  }
  JournalRecord &
  operator<<(int i)
  {
    sprintf(mes+strlen(mes), "%d", i); return *this;
  }
  JournalRecord &
  operator<<(double d)
  {
    sprintf(mes+strlen(mes), "%f", d); return *this;
  }
protected:
  void writePrefix(const SystemResourcesFlash &f);
};

class JournalRecordPair : public JournalRecord
{
  char prefix_end[MAXLEN];
public:
  JournalRecordPair(Journal &jr)
    : JournalRecord(jr, 'S')
  {
    prefix_end[0] = '\0'; journal.incrementDepth();
  }
  ~JournalRecordPair();
private:
  void writePrefixForEnd(const SystemResourcesFlash &f);
};

#endif
