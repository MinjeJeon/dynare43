// Copyright 2004, Ondra Kamenik

#include "tensor.hh"
#include "tl_exception.hh"
#include "tl_static.hh"
#include "pascal_triangle.hh"

/* Here we increment a given sequence within full symmetry given by
   |nv|, which is number of variables in each dimension. The underlying
   tensor is unfolded, so we increase the rightmost by one, and if it is
   |nv| we zero it and increase the next one to the left. */

void
UTensor::increment(IntSequence &v, int nv)
{
  if (v.size() == 0)
    return;
  int i = v.size()-1;
  v[i]++;
  while (i > 0 && v[i] == nv)
    {
      v[i] = 0;
      v[--i]++;
    }
}

/* This is dual to |UTensor::increment(IntSequence& v, int nv)|. */

void
UTensor::decrement(IntSequence &v, int nv)
{
  if (v.size() == 0)
    return;
  int i = v.size()-1;
  v[i]--;
  while (i > 0 && v[i] == -1)
    {
      v[i] = nv -1;
      v[--i]--;
    }
}

/* Here we increment index for general symmetry for unfolded
   storage. The sequence |nvmx| assigns for each coordinate a number of
   variables. Since the storage is unfolded, we do not need information
   about what variables are symmetric, everything necessary is given by
   |nvmx|. */

void
UTensor::increment(IntSequence &v, const IntSequence &nvmx)
{
  if (v.size() == 0)
    return;
  int i = v.size()-1;
  v[i]++;
  while (i > 0 && v[i] == nvmx[i])
    {
      v[i] = 0;
      v[--i]++;
    }
}

/* This is a dual code to |UTensor::increment(IntSequence& v, const
   IntSequence& nvmx)|. */

void
UTensor::decrement(IntSequence &v, const IntSequence &nvmx)
{
  if (v.size() == 0)
    return;
  int i = v.size()-1;
  v[i]--;
  while (i > 0 && v[i] == -1)
    {
      v[i] = nvmx[i] -1;
      v[--i]--;
    }
}

/* Here we return an offset for a given coordinates of unfolded full
   symmetry tensor. This is easy. */

int
UTensor::getOffset(const IntSequence &v, int nv)
{
  int pow = 1;
  int res = 0;
  for (int i = v.size()-1; i >= 0; i--)
    {
      res += v[i]*pow;
      pow *= nv;
    }
  return res;
}

/* Also easy. */

int
UTensor::getOffset(const IntSequence &v, const IntSequence &nvmx)
{
  int pow = 1;
  int res = 0;
  for (int i = v.size()-1; i >= 0; i--)
    {
      res += v[i]*pow;
      pow *= nvmx[i];
    }
  return res;
}

/* Decrementing of coordinates of folded index is not that easy. Note
   that if a trailing part of coordinates is $(b, a, a, a)$ (for
   instance) with $b<a$, then a preceding coordinates are $(b, a-1, n-1,
   n-1)$, where $n$ is a number of variables |nv|. So we find the left
   most element which is equal to the last element, decrease it by one,
   and then set all elements to the right to $n-1$. */

void
FTensor::decrement(IntSequence &v, int nv)
{
  int i = v.size()-1;
  while (i > 0 && v[i-1] == v[i])
    i--;
  v[i]--;
  for (int j = i+1; j < v.size(); j++)
    v[j] = nv-1;
}

/* This calculates order of the given index of our ordering of
   indices. In order to understand how it works, let us take number of
   variables $n$ and dimension $k$, and write down all the possible
   combinations of indices in our ordering. For example for $n=4$ and
   $k=3$, the sequence looks as:

   \def\tr#1#2#3{\hbox{\rlap{#1}\hskip 0.7em\rlap{#2}\hskip 0.7em\rlap{#3}\hskip 0.7em}}
   \halign{\tabskip=3em \hskip2cm #&#&#&#\cr
   \tr 000 &\tr 111 &\tr 222 &\tr 333\cr
   \tr 001 &\tr 112 &\tr 223 \cr
   \tr 002 &\tr 113 &\tr 233 \cr
   \tr 003 &\tr 122 \cr
   \tr 011 &\tr 123\cr
   \tr 012 &\tr 133\cr
   \tr 013\cr
   \tr 022\cr
   \tr 023\cr
   \tr 033\cr
   }

   Now observe, that a number of sequences starting with zero is the same
   as total number of sequences with the same number of variables but
   with dimension minus one. More generally, if $S_{n,k}$ denotes number
   of indices of $n$ variables and dimension $k$, then the number of
   indices beginning with $m$ is exactly $S_{n-m,k-1}$. This is because $m$
   can be subtracted from all items, and we obtain sequence of indices of
   $n-m$ variables. So we have formula:
   $$S_{n,k}=S_{n,k-1}+S_{n-1,k-1}+\ldots+S_{1,k-1}$$

   Now it is easy to calculate offset of index of the form
   $(m,\ldots,m)$. It is a sum of all above it, this is
   $S_{n,k-1}+\ldots+S_{n-m,k-1}$. We know that $S_{n,k}=\pmatrix{n+k-1\cr
   k}$. Using above formula, we can calculate offset of $(m,\ldots,m)$ as
   $$\pmatrix{n+k-1\cr k}-\pmatrix{n-m+k-1\cr k}$$

   The offset of general index $(m_1,m_2,\ldots,m_k)$ is calculated
   recursively, since it is offset of $(m_1,\ldots,m_1)$ for $n$
   variables plus offset of $(m_2-m_1,m_3-m_1,\ldots,m_k-m_1)$ for
   $n-m_1$ variables. */

int
FTensor::getOffsetRecurse(IntSequence &v, int nv)
{
  if (v.size() == 0)
    return 0;
  int prefix = v.getPrefixLength();
  int m = v[0];
  int k = v.size();
  int s1 = PascalTriangle::noverk(nv+k-1, k) - PascalTriangle::noverk(nv-m+k-1, k);
  IntSequence subv(v, prefix, k);
  subv.add(-m);
  int s2 = getOffsetRecurse(subv, nv-m);
  return s1+s2;
}
