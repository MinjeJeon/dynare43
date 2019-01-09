// Copyright 2004, Ondra Kamenik

// Permutations.

/* The permutation class is useful when describing a permutation of
   indices in permuted symmetry tensor. This tensor comes to existence,
   for instance, as a result of the following tensor multiplication:
   $$\left[g_{y^3}\right]_{\gamma_1\gamma_2\gamma_3}
   \left[g_{yu}\right]^{\gamma_1}_{\alpha_1\beta_3}
   \left[g_{yu}\right]^{\gamma_2}_{\alpha_2\beta_1}
   \left[g_u\right]^{\gamma_3}_{\beta_2}
   $$
   If this operation is done by a Kronecker product of unfolded tensors,
   the resulting tensor has permuted indices. So, in this case the
   permutation is implied by the equivalence:
   $\{\{0,4\},\{1,3\},\{2\}\}$. This results in a permutation which maps
   indices $(0,1,2,3,4)\mapsto(0,2,4,3,1)$.

   The other application of |Permutation| class is to permute indices
   with the same permutation as done during sorting.

   Here we only define an abstraction for the permutation defined by an
   equivalence. Its basic operation is to apply the permutation to the
   integer sequence. The application is right (or inner), in sense that
   it works on indices of the sequence not items of the sequence. More
   formally $s\circ m \not=m\circ s$. In here, the application of the
   permutation defined by map $m$ is $s\circ m$.

   Also, we need |PermutationSet| class which contains all permutations
   of $n$ element set, and a bundle of permutations |PermutationBundle|
   which contains all permutation sets up to a given number. */

#ifndef PERMUTATION_H
#define PERMUTATION_H

#include "int_sequence.hh"
#include "equivalence.hh"

#include <vector>

/* The permutation object will have a map, which defines mapping of
   indices $(0,1,\ldots,n-1)\mapsto(m_0,m_1,\ldots, m_{n-1})$. The map is
   the sequence $(m_0,m_1,\ldots, m_{n-1}$. When the permutation with the
   map $m$ is applied on sequence $s$, it permutes its indices:
   $s\circ\hbox{id}\mapsto s\circ m$.

   So we have one constructor from equivalence, then a method |apply|,
   and finally a method |tailIdentity| which returns a number of trailing
   indices which yield identity. Also we have a constructor calculating
   map, which corresponds to permutation in sort. This is, we want
   $(\hbox{sorted }s)\circ m = s$. */

class Permutation
{
protected:
  IntSequence permap;
public:
  Permutation(int len)
    : permap(len)
  {
    for (int i = 0; i < len; i++)
      permap[i] = i;
  }
  Permutation(const Equivalence &e)
    : permap(e.getN())
  {
    e.trace(permap);
  }
  Permutation(const Equivalence &e, const Permutation &per)
    : permap(e.getN())
  {
    e.trace(permap, per);
  }
  Permutation(const IntSequence &s)
    : permap(s.size())
  {
    computeSortingMap(s);
  };
  Permutation(const Permutation &p)
     
  = default;
  Permutation(const Permutation &p1, const Permutation &p2)
    : permap(p2.permap)
  {
    p1.apply(permap);
  }
  Permutation(const Permutation &p, int i)
    : permap(p.size(), p.permap, i)
  {
  }
  Permutation &
  operator=(const Permutation &p)
  = default;
  bool
  operator==(const Permutation &p)
  {
    return permap == p.permap;
  }
  int
  size() const
  {
    return permap.size();
  }
  void
  print() const
  {
    permap.print();
  }
  void apply(const IntSequence &src, IntSequence &tar) const;
  void apply(IntSequence &tar) const;
  void inverse();
  int tailIdentity() const;
  const IntSequence &
  getMap() const
  {
    return permap;
  }
  IntSequence &
  getMap()
  {
    return permap;
  }
protected:
  void computeSortingMap(const IntSequence &s);
};

/* The |PermutationSet| maintains an array of of all permutations. The
   default constructor constructs one element permutation set of one
   element sets. The second constructor constructs a new permutation set
   over $n$ from all permutations over $n-1$. The parameter $n$ need not
   to be provided, but it serves to distinguish the constructor from copy
   constructor, which is not provided.

   The method |getPreserving| returns a factor subgroup of permutations,
   which are invariants with respect to the given sequence. This are all
   permutations $p$ yielding $p\circ s = s$, where $s$ is the given
   sequence. */

class PermutationSet
{
  int order{1};
  int size{1};
  const Permutation **const pers;
public:
  PermutationSet();
  PermutationSet(const PermutationSet &ps, int n);
  ~PermutationSet();
  int
  getNum() const
  {
    return size;
  }
  const Permutation &
  get(int i) const
  {
    return *(pers[i]);
  }
  vector<const Permutation *> getPreserving(const IntSequence &s) const;
};

/* The permutation bundle encapsulates all permutations sets up to some
   given dimension. */

class PermutationBundle
{
  vector<PermutationSet *> bundle;
public:
  PermutationBundle(int nmax);
  ~PermutationBundle();
  const PermutationSet&get(int n) const;
  void generateUpTo(int nmax);
};

#endif
