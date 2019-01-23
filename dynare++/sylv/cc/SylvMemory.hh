/* $Header: /var/lib/cvs/dynare_cpp/sylv/cc/SylvMemory.h,v 1.1.1.1 2004/06/04 13:00:49 kamenik Exp $ */

/* Tag $Name:  $ */

#ifndef SYLV_MEMORY_H
#define SYLV_MEMORY_H

#include "SylvParams.hh"

#include <new>

class MallocAllocator
{
#ifdef USE_MEMORY_POOL
public:
  void *operator new(size_t size);
  void *operator new[](size_t size);
  void operator delete(void *p);
  void operator delete[](void *p);
#endif
};

#ifdef USE_MEMORY_POOL
void *operator new(size_t size);
void *operator new[](size_t size);
void operator delete(void *p);
void operator delete[](void *p);
#endif

class SylvMemoryPool
{
  char *base{nullptr};
  size_t length{0};
  size_t allocated{0};
  bool stack_mode{false};
public:
  SylvMemoryPool() = default;
  SylvMemoryPool(const SylvMemoryPool &) = delete;
  SylvMemoryPool &operator=(const SylvMemoryPool &) = delete;
  ~SylvMemoryPool();
  void init(size_t size);
  void *allocate(size_t size);
  void free(void *p);
  void reset();
  void setStackMode(bool);
};

class SylvMemoryDriver
{
public:
  SylvMemoryDriver(int num_d, int m, int n, int order);
  SylvMemoryDriver(const SylvParams &pars, int num_d, int m, int n, int order);
  SylvMemoryDriver(const SylvMemoryDriver &) = delete;
  SylvMemoryDriver &operator=(const SylvMemoryDriver &) = delete;
  static void setStackMode(bool);
  ~SylvMemoryDriver();
protected:
  void allocate(int num_d, int m, int n, int order);
};

#endif /* SYLV_MEMORY_H */