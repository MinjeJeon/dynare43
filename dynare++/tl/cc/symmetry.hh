// Copyright 2004, Ondra Kamenik

// Symmetry.

/* Symmetry is an abstraction for a term of the form $y^3u^2$. It manages
   only indices, not the variable names. So if one uses this
   abstraction, he must keep in mind that $y$ is the first, and $u$ is
   the second.

   In fact, the symmetry is a special case of equivalence, but its
   implementation is much simpler. We do not need an abstraction for the
   term $yyuyu$ but due to Green theorem we can have term $y^3u^2$. That
   is why the equivalence is too general for our purposes.

   One of a main purposes of the tensor library is to calculate something like:
   $$\left[B_{y^2u^3}\right]_{\alpha_1\alpha_2\beta_1\beta_2\beta_3}
   =\left[g_{y^l}\right]_{\gamma_1\ldots\gamma_l}
   \left(\sum_{c\in M_{l,5}}
   \prod_{m=1}^l\left[g_{c_m}\right]^{\gamma_m}_{c_m(\alpha,\beta)}\right)$$
   If, for instance, $l=3$, and $c=\{\{0,4\},\{1,2\},\{3\}\}$, then we
   have to calculate
   $$\left[g_{y^3}\right]_{\gamma_1\gamma_2\gamma_3}
   \left[g_{yu}\right]^{\gamma_1}_{\alpha_1\beta_3}
   \left[g_{yu}\right]^{\gamma_2}_{\alpha_2\beta_1}
   \left[g_u\right]^{\gamma_3}_{\beta_2}
   $$

   We must be able to calculate a symmetry induced by symmetry $y^2u^3$
   and by an equivalence class from equivalence $c$. For equivalence
   class $\{0,4\}$ the induced symmetry is $yu$, since we pick first and
   fifth variable from $y^2u^3$. For a given outer symmetry, the class
   |InducedSymmetries| does this for all classes of a given equivalence.

   We need also to cycle through all possible symmetries yielding the
   given dimension. For this purpose we define classes |SymmetrySet| and
   |symiterator|.

   The symmetry is implemented as |IntSequence|, in fact, it inherits
   from it. */

#ifndef SYMMETRY_H
#define SYMMETRY_H

#include "equivalence.hh"
#include "int_sequence.hh"

#include <list>
#include <vector>

/* Clear. The method |isFull| returns true if and only if the symmetry
   allows for any permutation of indices. */

class Symmetry : public IntSequence
{
public:
  /* We provide three constructors for symmetries of the form $y^n$,
     $y^nu^m$, $y^nu^m\sigma^k$. Also a copy constructor, and finally a
     constructor of implied symmetry for a symmetry and an equivalence
     class. It is already implemented in |IntSequence| so we only call
     appropriate constructor of |IntSequence|. We also provide the
     subsymmetry, which takes the given length of symmetry from the end.

     The last constructor constructs a symmetry from an integer sequence
     (supposed to be ordered) as a symmetry counting successively equal
     items. For instance the sequence $(a,a,a,b,c,c,d,d,d,d)$ produces
     symmetry $(3,1,2,4)$. */
  Symmetry(int len, const char *dummy)
    : IntSequence(len, 0)
  {
  }
  Symmetry(int i1)
    : IntSequence(1, i1)
  {
  }
  Symmetry(int i1, int i2)
    : IntSequence(2)
  {
    operator[](0) = i1; operator[](1) = i2;
  }
  Symmetry(int i1, int i2, int i3)
    : IntSequence(3)
  {
    operator[](0) = i1;
    operator[](1) = i2;
    operator[](2) = i3;
  }
  Symmetry(int i1, int i2, int i3, int i4)
    : IntSequence(4)
  {
    operator[](0) = i1;
    operator[](1) = i2;
    operator[](2) = i3;
    operator[](3) = i4;
  }
  Symmetry(const Symmetry &s)
     
  = default;
  Symmetry(const Symmetry &s, const OrdSequence &cl)
    : IntSequence(s, cl.getData())
  {
  }
  Symmetry(Symmetry &s, int len)
    : IntSequence(s, s.size()-len, s.size())
  {
  }
  Symmetry(const IntSequence &s);

  int
  num() const
  {
    return size();
  }
  int
  dimen() const
  {
    return sum();
  }
  int findClass(int i) const;
  bool isFull() const;
};

/* The class |SymmetrySet| defines a set of symmetries of the given
   length having given dimension. It does not store all the symmetries,
   rather it provides a storage for one symmetry, which is changed as an
   adjoint iterator moves.

   The iterator class is |symiterator|. It is implemented
   recursively. The iterator object, when created, creates subordinal
   iterator, which iterates over a symmetry set whose length is one less,
   and dimension is the former dimension. When the subordinal iterator
   goes to its end, the superordinal iterator increases left most index in
   the symmetry, resets the subordinal symmetry set with different
   dimension, and iterates through the subordinal symmetry set until its
   end, and so on. That's why we provide also |SymmetrySet| constructor
   for construction of a subordinal symmetry set.

   The typical usage of the abstractions for |SymmetrySet| and
   |symiterator| is as follows:

   \kern0.3cm
   \centerline{|for (symiterator si(SymmetrySet(6, 4)); !si.isEnd(); ++si) {body}|}
   \kern0.3cm

   \noindent It goes through all symmetries of size 4 having dimension
   6. One can use |*si| as the symmetry in the body. */

class SymmetrySet
{
  Symmetry run;
  int dim;
public:
  SymmetrySet(int d, int length)
    : run(length, ""), dim(d)
  {
  }
  SymmetrySet(SymmetrySet &s, int d)
    : run(s.run, s.size()-1), dim(d)
  {
  }
  int
  dimen() const
  {
    return dim;
  }
  const Symmetry &
  sym() const
  {
    return run;
  }
  Symmetry &
  sym()
  {
    return run;
  }
  int
  size() const
  {
    return run.size();
  }
};

/* The logic of |symiterator| was described in |@<|SymmetrySet| class
   declaration@>|. Here we only comment that: the class has a reference
   to the |SymmetrySet| only to know dimension and for access of its
   symmetry storage. Further we have pointers to subordinal |symiterator|
   and its |SymmetrySet|. These are pointers, since the recursion ends at
   length equal to 2, in which case these pointers are |NULL|.

   The constructor creates the iterator which initializes to the first
   symmetry (beginning). */

class symiterator
{
  SymmetrySet &s;
  symiterator *subit;
  SymmetrySet *subs;
  bool end_flag;
public:
  symiterator(SymmetrySet &ss);
  ~symiterator();
  symiterator &operator++();
  bool
  isEnd() const
  {
    return end_flag;
  }
  const Symmetry &
  operator*() const
  {
    return s.sym();
  }
};

/* This simple abstraction just constructs a vector of induced
   symmetries from the given equivalence and outer symmetry. A
   permutation might optionally permute the classes of the equivalence. */

class InducedSymmetries : public vector<Symmetry>
{
public:
  InducedSymmetries(const Equivalence &e, const Symmetry &s);
  InducedSymmetries(const Equivalence &e, const Permutation &p, const Symmetry &s);
  void print() const;
};

#endif
