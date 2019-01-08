// Copyright 2004, Ondra Kamenik

// Integer sequence.

/* Here we define an auxiliary abstraction for a sequence of integers. The
   basic functionality is to hold an ordered sequence of integers with
   constant length. We prefer using this simple class before STL
   |vector<int>| since it is more efficient for our purposes.

   The class is used in index of a tensor, in symmetry definition, in
   Kronecker product dimensions, or as a class of an equivalence. The
   latter case is not ordered, but we always order equivalence classes in
   order to ensure unique representativeness. For almost all cases we
   need the integer sequence to be ordered (sort), or monotonize (indices
   of folded tensors), or partially monotonize (indices of folded tensors
   not fully symmetric), or calculate a product of all members or only of
   a part (used in Kronecker product dimensions). When we calculate
   offsets in folded tensors, we need to obtain a number of the same
   items in the front (|getPrefixLength|), and also to add some integer
   number to all items.

   Also, we need to construct a subsequence of a sequence, so
   some instances do destroy the underlying data, and some not. */

#ifndef INT_SEQUENCE_H
#define INT_SEQUENCE_H

#include <cstring>
#include <vector>

using namespace std;

/* The implementation of |IntSequence| is straightforward. It has a
   pointer |data|, a |length| of the data, and a flag |destroy|, whether
   the instance must destroy the underlying data. */

class Symmetry;
class IntSequence
{
  int *data;
  int length;
  bool destroy;
public:
  /* We have a constructor allocating a given length of data, constructor
     allocating and then initializing all members to a given number, a copy
     constructor, a conversion from |vector<int>|, a subsequence
     constructor, a constructor used for calculating implied symmetry from
     a more general symmetry and one equivalence class (see |Symmetry|
     class). Finally we have a constructor which unfolds a sequence with
     respect to a given symmetry and constructor which inserts a given
     number to the ordered sequence or given number to a given position. */

  IntSequence(int l)
    : data(new int[l]), length(l), destroy(true)
  {
  }
  IntSequence(int l, int n)
    :  data(new int[l]), length(l), destroy(true)
  {
    for (int i = 0; i < length; i++)
      data[i] = n;
  }
  IntSequence(const IntSequence &s)
    : data(new int[s.length]), length(s.length), destroy(true)
  {
    memcpy(data, s.data, length*sizeof(int));
  }
  IntSequence(IntSequence &s, int i1, int i2)
    : data(s.data+i1), length(i2-i1), destroy(false)
  {
  }
  IntSequence(const IntSequence &s, int i1, int i2)
    : data(new int[i2-i1]), length(i2-i1), destroy(true)
  {
    memcpy(data, s.data+i1, sizeof(int)*length);
  }
  IntSequence(const Symmetry &sy, const vector<int> &se);
  IntSequence(const Symmetry &sy, const IntSequence &se);
  IntSequence(int i, const IntSequence &s);
  IntSequence(int i, const IntSequence &s, int pos);
  IntSequence(int l, const int *d)
    : data(new int[l]), length(l), destroy(true)
  {
    memcpy(data, d, sizeof(int)*length);
  }

  const IntSequence &operator=(const IntSequence &s);
  virtual ~IntSequence()
  {
    if (destroy)
      delete [] data;
  }
  bool operator==(const IntSequence &s) const;
  bool
  operator!=(const IntSequence &s) const
  {
    return !operator==(s);
  }
  int &
  operator[](int i)
  {
    return data[i];
  }
  int
  operator[](int i) const
  {
    return data[i];
  }
  int
  size() const
  {
    return length;
  }

  /* We provide two orderings. The first |operator<| is the linear
     lexicographic ordering, the second |less| is the non-linear Cartesian
     ordering. */
  bool operator<(const IntSequence &s) const;
  bool
  operator<=(const IntSequence &s) const
  {
    return (operator==(s) || operator<(s));
  }
  bool lessEq(const IntSequence &s) const;
  bool less(const IntSequence &s) const;

  void sort();
  void monotone();
  void pmonotone(const Symmetry &s);
  int sum() const;
  int mult(int i1, int i2) const;
  int
  mult() const
  {
    return mult(0, length);
  }
  void add(int i);
  void add(int f, const IntSequence &s);
  int getPrefixLength() const;
  int getNumDistinct() const;
  int getMax() const;
  bool isPositive() const;
  bool isConstant() const;
  bool isSorted() const;
  void print() const;
};

#endif
