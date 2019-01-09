// Copyright 2004, Ondra Kamenik

#include "int_sequence.hh"
#include "symmetry.hh"
#include "tl_exception.hh"

#include <cstdio>
#include <climits>

/* This unfolds a given integer sequence with respect to the given
   symmetry. If for example the symmetry is $(2,3)$, and the sequence is
   $(a,b)$, then the result is $(a,a,b,b,b)$. */

IntSequence::IntSequence(const Symmetry &sy, const IntSequence &se)
  : data(new int[sy.dimen()]), length(sy.dimen()), destroy(true)
{
  int k = 0;
  for (int i = 0; i < sy.num(); i++)
    for (int j = 0; j < sy[i]; j++, k++)
      operator[](k) = se[i];
}

/* This constructs an implied symmetry (implemented as |IntSequence|
   from a more general symmetry and equivalence class (implemented as
   |vector<int>|). For example, let the general symmetry be $y^3u^2$ and
   the equivalence class is $\{0,4\}$ picking up first and fifth
   variable, we calculate symmetry (at this point only |IntSequence|)
   corresponding to the picked variables. These are $yu$. Thus the
   constructed sequence must be $(1,1)$, meaning that we picked one $y$
   and one $u$. */

IntSequence::IntSequence(const Symmetry &sy, const vector<int> &se)
  : data(new int[sy.num()]), length(sy.num()), destroy(true)
{
  TL_RAISE_IF(sy.dimen() <= se[se.size()-1],
              "Sequence is not reachable by symmetry in IntSequence()");
  for (int i = 0; i < length; i++)
    operator[](i) = 0;

  for (int i : se)
    operator[](sy.findClass(i))++;
}

/* This constructs an ordered integer sequence from the given ordered
   sequence inserting the given number to the sequence. */

IntSequence::IntSequence(int i, const IntSequence &s)
  : data(new int[s.size()+1]), length(s.size()+1), destroy(true)
{
  int j = 0;
  while (j < s.size() && s[j] < i)
    j++;
  for (int jj = 0; jj < j; jj++)
    operator[](jj) = s[jj];
  operator[](j) = i;
  for (int jj = j; jj < s.size(); jj++)
    operator[](jj+1) = s[jj];
}

IntSequence::IntSequence(int i, const IntSequence &s, int pos)
  : data(new int[s.size()+1]), length(s.size()+1), destroy(true)
{
  TL_RAISE_IF(pos < 0 || pos > s.size(),
              "Wrong position for insertion IntSequence constructor");
  for (int jj = 0; jj < pos; jj++)
    operator[](jj) = s[jj];
  operator[](pos) = i;
  for (int jj = pos; jj < s.size(); jj++)
    operator[](jj+1) = s[jj];
}

const IntSequence &
IntSequence::operator=(const IntSequence &s)
{
  TL_RAISE_IF(!destroy && length != s.length,
              "Wrong length for in-place IntSequence::operator=");
  if (destroy && length != s.length)
    {
      delete [] data;
      data = new int[s.length];
      destroy = true;
      length = s.length;
    }
  memcpy(data, s.data, sizeof(int)*length);
  return *this;
}

bool
IntSequence::operator==(const IntSequence &s) const
{
  if (size() != s.size())
    return false;

  int i = 0;
  while (i < size() && operator[](i) == s[i])
    i++;
  return i == size();
}

/* We need some linear irreflexive ordering, we implement it as
   lexicographic ordering without identity. */
bool
IntSequence::operator<(const IntSequence &s) const
{
  int len = min(size(), s.size());

  int i = 0;
  while (i < len && operator[](i) == s[i])
    i++;
  return (i < s.size() && (i == size() || operator[](i) < s[i]));
}

bool
IntSequence::lessEq(const IntSequence &s) const
{
  TL_RAISE_IF(size() != s.size(),
              "Sequence with different lengths in IntSequence::lessEq");

  int i = 0;
  while (i < size() && operator[](i) <= s[i])
    i++;
  return (i == size());
}

bool
IntSequence::less(const IntSequence &s) const
{
  TL_RAISE_IF(size() != s.size(),
              "Sequence with different lengths in IntSequence::less");

  int i = 0;
  while (i < size() && operator[](i) < s[i])
    i++;
  return (i == size());
}

/* This is a bubble sort, all sequences are usually very short, so this
   sin might be forgiven. */

void
IntSequence::sort()
{
  for (int i = 0; i < length; i++)
    {
      int swaps = 0;
      for (int j = 0; j < length-1; j++)
        {
          if (data[j] > data[j+1])
            {
              int s = data[j+1];
              data[j+1] = data[j];
              data[j] = s;
              swaps++;
            }
        }
      if (swaps == 0)
        return;
    }
}

/* Here we monotonize the sequence. If an item is less then its
   predecessor, it is equalized. */

void
IntSequence::monotone()
{
  for (int i = 1; i < length; i++)
    if (data[i-1] > data[i])
      data[i] = data[i-1];
}

/* This partially monotones the sequence. The partitioning is done by a
   symmetry. So the subsequence given by the symmetry classes are
   monotonized. For example, if the symmetry is $y^2u^3$, and the
   |IntSequence| is $(5,3,1,6,4)$, the result is $(5,5,1,6,6)$. */

void
IntSequence::pmonotone(const Symmetry &s)
{
  int cum = 0;
  for (int i = 0; i < s.num(); i++)
    {
      for (int j = cum + 1; j < cum + s[i]; j++)
        if (data[j-1] > data[j])
          data[j] = data[j-1];
      cum += s[i];
    }
}

/* This returns sum of all elements. Useful for symmetries. */

int
IntSequence::sum() const
{
  int res = 0;
  for (int i = 0; i < length; i++)
    res += operator[](i);
  return res;
}

/* This returns product of subsequent items. Useful for Kronecker product
   dimensions. */

int
IntSequence::mult(int i1, int i2) const
{
  int res = 1;
  for (int i = i1; i < i2; i++)
    res *= operator[](i);
  return res;
}

/* Return a number of the same items in the beginning of the sequence. */

int
IntSequence::getPrefixLength() const
{
  int i = 0;
  while (i+1 < size() && operator[](i+1) == operator[](0))
    i++;
  return i+1;
}

/* This returns a number of distinct items in the sequence. It supposes
   that the sequence is ordered. For the empty sequence it returns zero. */

int
IntSequence::getNumDistinct() const
{
  int res = 0;
  if (size() > 0)
    res++;
  for (int i = 1; i < size(); i++)
    if (operator[](i) != operator[](i-1))
      res++;
  return res;
}

/* This returns a maximum of the sequence. If the sequence is empty, it
   returns the least possible |int| value. */

int
IntSequence::getMax() const
{
  int res = INT_MIN;
  for (int i = 0; i < size(); i++)
    if (operator[](i) > res)
      res = operator[](i);
  return res;
}

void
IntSequence::add(int i)
{
  for (int j = 0; j < size(); j++)
    operator[](j) += i;
}

void
IntSequence::add(int f, const IntSequence &s)
{
  TL_RAISE_IF(size() != s.size(),
              "Wrong sequence length in IntSequence::add");
  for (int j = 0; j < size(); j++)
    operator[](j) += f*s[j];
}

bool
IntSequence::isPositive() const
{
  int i = 0;
  while (i < size() && operator[](i) >= 0)
    i++;
  return (i == size());
}

bool
IntSequence::isConstant() const
{
  bool res = true;
  int i = 1;
  while (res && i < size())
    {
      res = res && operator[](0) == operator[](i);
      i++;
    }
  return res;
}

bool
IntSequence::isSorted() const
{
  bool res = true;
  int i = 1;
  while (res && i < size())
    {
      res = res && operator[](i-1) <= operator[](i);
      i++;
    }
  return res;
}

/* Debug print. */

void
IntSequence::print() const
{
  printf("[");
  for (int i = 0; i < size(); i++)
    printf("%2d ", operator[](i));
  printf("]\n");
}
