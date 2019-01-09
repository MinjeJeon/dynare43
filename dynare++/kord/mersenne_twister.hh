// Copyright 2007, Ondra Kamenik

// Mersenne Twister PRNG

/* This file provides a class for generating random numbers with
   encapsulated state. It is based on the work of Makoto Matsumoto and
   Takuji Nishimura, implementation inspired by code of Richard Wagner
   and Geoff Kuenning. */

#ifndef MERSENNE_TWISTER_H
#define MERSENNE_TWISTER_H

#include "random.hh"
#include <cstring>

class MersenneTwister : public RandomGenerator
{
protected:
  typedef unsigned int uint32;
  enum {STATE_SIZE = 624};
  enum {RECUR_OFFSET = 397};
  uint32 statevec[STATE_SIZE];
  int stateptr;
public:
  MersenneTwister(uint32 iseed);
  MersenneTwister(const MersenneTwister &mt);
  virtual ~MersenneTwister()
  = default;
  uint32 lrand();
  double drand();
  double
  uniform() override
  {
    return drand();
  }
protected:
  void seed(uint32 iseed);
  void refresh();
private:
  static uint32
  hibit(uint32 u)
  {
    return u & 0x80000000UL;
  }
  static uint32
  lobit(uint32 u)
  {
    return u & 0x00000001UL;
  }
  static uint32
  lobits(uint32 u)
  {
    return u & 0x7fffffffUL;
  }
  static uint32
  mixbits(uint32 u, uint32 v)
  {
    return hibit(u) | lobits(v);
  }
  static uint32
  twist(uint32 m, uint32 s0, uint32 s1)
  {
    return m ^ (mixbits(s0, s1)>>1) ^ (-lobit(s1) & 0x9908b0dfUL);
  }
};

inline MersenneTwister::MersenneTwister(uint32 iseed)
{
  seed(iseed);
}

inline MersenneTwister::MersenneTwister(const MersenneTwister &mt)
  : stateptr(mt.stateptr)
{
  memcpy(statevec, mt.statevec, sizeof(uint32)*STATE_SIZE);
}

inline MersenneTwister::uint32
MersenneTwister::lrand()
{
  if (stateptr >= STATE_SIZE)
    refresh();

  register uint32 v = statevec[stateptr++];
  v ^= v >> 11;
  v ^= (v << 7) & 0x9d2c5680;
  v ^= (v << 15) & 0xefc60000;
  return (v ^ (v >> 18));
}

inline double
MersenneTwister::drand()
{
  uint32 a = lrand() >> 5;
  uint32 b = lrand() >> 6;
  return (a*67108864.0+b) * (1.0/9007199254740992.0);
}

// PRNG of D. Knuth
inline void
MersenneTwister::seed(uint32 iseed)
{
  statevec[0] = iseed & 0xffffffffUL;
  for (int i = 1; i < STATE_SIZE; i++)
    {
      register uint32 val = statevec[i-1] >> 30;
      val ^= statevec[i-1];
      val *= 1812433253ul;
      val += i;
      statevec[i] = val & 0xffffffffUL;
    }

  refresh();
}

inline void
MersenneTwister::refresh()
{
  register uint32 *p = statevec;
  for (int i = STATE_SIZE-RECUR_OFFSET; i--; ++p)
    *p = twist(p[RECUR_OFFSET], p[0], p[1]);
  for (int i = RECUR_OFFSET; --i; ++p)
    *p = twist(p[RECUR_OFFSET-STATE_SIZE], p[0], p[1]);
  *p = twist(p[RECUR_OFFSET-STATE_SIZE], p[0], statevec[0]);

  stateptr = 0;
}

#endif
