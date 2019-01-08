// Copyright 2004, Ondra Kamenik

// Exception.

/* Within the code we often check some state of variables, typically
   preconditions or postconditions. If the state is not as required, it
   is worthless to continue, since this means some fatal error in
   algorithms. In this case we raise an exception which can be caught at
   some higher level. This header file defines a simple infrastructure
   for this. */

#ifndef TL_EXCEPTION_H
#define TL_EXCEPTION_H

#include <cstring>
#include <cstdio>

/* The basic idea of raising an exception if some condition fails is
   that the conditions is checked only if required. We define global
   |TL_DEBUG| macro which is integer and says, how many debug messages
   the programm has to emit. We also define |TL_DEBUG_EXCEPTION| which
   says, for what values of |TL_DEBUG| we will check for conditions of
   the exceptions. If the |TL_DEBUG| is equal or higher than
   |TL_DEBUG_EXCEPTION|, the exception conditions are checked.

   We define |TL_RAISE|, and |TL_RAISE_IF| macros which throw an instance
   of |TLException| if |TL_DEBUG >= TL_DEBUG_EXCEPTION|. The first is
   unconditional throw, the second is conditioned by a given
   expression. Note that if |TL_DEBUG < TL_DEBUG_EXCEPTION| then the code
   is compiled but evaluation of the condition is passed. If code is
   optimized, the optimizer also passes evaluation of |TL_DEBUG| and
   |TL_DEBUG_EXCEPTION| comparison (I hope).

   We provide default values for |TL_DEBUG| and |TL_DEBUG_EXCEPTION|. */

#ifndef TL_DEBUG_EXCEPTION
# define TL_DEBUG_EXCEPTION 1
#endif

#ifndef TL_DEBUG
# define TL_DEBUG 0
#endif

#define TL_RAISE(mes)                                                   \
  if (TL_DEBUG >= TL_DEBUG_EXCEPTION) throw TLException(__FILE__, __LINE__, mes);

#define TL_RAISE_IF(expr, mes)                                          \
  if (TL_DEBUG >= TL_DEBUG_EXCEPTION && (expr)) throw TLException(__FILE__, __LINE__, mes);

/* Primitive exception class containing file name, line number and message. */

class TLException
{
  char fname[50];
  int lnum;
  char message[500];
public:
  TLException(const char *f, int l, const char *mes)
  {
    strncpy(fname, f, 50); fname[49] = '\0';
    strncpy(message, mes, 500); message[499] = '\0';
    lnum = l;
  }
  virtual ~TLException()
  {
  }
  virtual void
  print() const
  {
    printf("At %s:%d:%s\n", fname, lnum, message);
  }
};

#endif
