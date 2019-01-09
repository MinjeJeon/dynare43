// Copyright (C) 2004-2011, Ondra Kamenik

#include "symmetry.hh"
#include "permutation.hh"

#include <cstdio>

/* Construct symmetry as numbers of successively equal items in the sequence. */

Symmetry::Symmetry(const IntSequence &s)
  : IntSequence(s.getNumDistinct(), 0)
{
  int p = 0;
  if (s.size() > 0)
    operator[](p) = 1;
  for (int i = 1; i < s.size(); i++)
    {
      if (s[i] != s[i-1])
        p++;
      operator[](p)++;
    }
}

/* Find a class of the symmetry containing a given index. */

int
Symmetry::findClass(int i) const
{
  int j = 0;
  int sum = 0;
  do
    {
      sum += operator[](j);
      j++;
    }
  while (j < size() && sum <= i);

  return j-1;
}

/* The symmetry is full if it allows for any permutation of indices. It
   means, that there is at most one non-zero index. */

bool
Symmetry::isFull() const
{
  int count = 0;
  for (int i = 0; i < num(); i++)
    if (operator[](i) != 0)
      count++;
  return count <= 1;
}

/* Here we construct the beginning of the |symiterator|. The first
   symmetry index is 0. If length is 2, the second index is the
   dimension, otherwise we create the subordinal symmetry set and its
   beginning as subordinal |symiterator|. */

symiterator::symiterator(SymmetrySet &ss)
  : s(ss), subit(NULL), subs(NULL), end_flag(false)
{
  s.sym()[0] = 0;
  if (s.size() == 2)
    {
      s.sym()[1] = s.dimen();
    }
  else
    {
      subs = new SymmetrySet(s, s.dimen());
      subit = new symiterator(*subs);
    }
}

symiterator::~symiterator()
{
  if (subit)
    delete subit;
  if (subs)
    delete subs;
}

/* Here we move to the next symmetry. We do so only, if we are not at
   the end. If length is 2, we increase lower index and decrease upper
   index, otherwise we increase the subordinal symmetry. If we got to the
   end, we recreate the subordinal symmetry set and set the subordinal
   iterator to the beginning. At the end we test, if we are not at the
   end. This is recognized if the lowest index exceeded the dimension. */

symiterator &
symiterator::operator++()
{
  if (!end_flag)
    {
      if (s.size() == 2)
        {
          s.sym()[0]++;
          s.sym()[1]--;
        }
      else
        {
          ++(*subit);
          if (subit->isEnd())
            {
              delete subit;
              delete subs;
              s.sym()[0]++;
              subs = new SymmetrySet(s, s.dimen()-s.sym()[0]);
              subit = new symiterator(*subs);
            }
        }
      if (s.sym()[0] == s.dimen()+1)
        end_flag = true;
    }
  return *this;
}

InducedSymmetries::InducedSymmetries(const Equivalence &e, const Symmetry &s)
{
  for (const auto & i : e)
    {
      push_back(Symmetry(s, i));
    }
}

// |InducedSymmetries| permuted constructor code
InducedSymmetries::InducedSymmetries(const Equivalence &e, const Permutation &p,
                                     const Symmetry &s)
{
  for (int i = 0; i < e.numClasses(); i++)
    {
      Equivalence::const_seqit it = e.find(p.getMap()[i]);
      push_back(Symmetry(s, *it));
    }
}

/* Debug print. */

void
InducedSymmetries::print() const
{
  printf("Induced symmetries: %lu\n", (unsigned long) size());
  for (unsigned int i = 0; i < size(); i++)
    operator[](i).print();
}
