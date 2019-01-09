// Copyright 2005, Ondra Kamenik

// Product quadrature.

/* This file defines a product multidimensional quadrature. If $Q_k$
   denotes the one dimensional quadrature, then the product quadrature
   $Q$ of $k$ level and dimension $d$ takes the form
   $$Qf=\sum_{i_1=1}^{n_k}\ldots\sum_{i_d=1}^{n^k}w_{i_1}\cdot\ldots\cdot w_{i_d}
   f(x_{i_1},\ldots,x_{i_d})$$
   which can be written in terms of the one dimensional quadrature $Q_k$ as
   $$Qf=(Q_k\otimes\ldots\otimes Q_k)f$$

   Here we define the product quadrature iterator |prodpit| and plug it
   into |QuadratureImpl| to obtains |ProductQuadrature|. */

#ifndef PRODUCT_H
#define PRODUCT_H

#include "int_sequence.hh"
#include "vector_function.hh"
#include "quadrature.hh"

/* This defines a product point iterator. We have to maintain the
   following: a pointer to product quadrature in order to know the
   dimension and the underlying one dimensional quadrature, then level,
   number of points in the level, integer sequence of indices, signal,
   the coordinates of the point and the weight.

   The point indices, signal, and point coordinates are implmented as
   pointers in order to allow for empty constructor.

   The constructor |prodpit(const ProductQuadrature& q, int j0, int l)|
   constructs an iterator pointing to $(j0,0,\ldots,0)$, which is used by
   |begin| dictated by |QuadratureImpl|. */

class ProductQuadrature;

class prodpit
{
protected:
  const ProductQuadrature *prodq;
  int level{0};
  int npoints{0};
  IntSequence *jseq;
  bool end_flag{true};
  ParameterSignal *sig;
  Vector *p;
  double w;
public:
  prodpit();
  prodpit(const ProductQuadrature &q, int j0, int l);
  prodpit(const prodpit &ppit);
  ~prodpit();
  bool operator==(const prodpit &ppit) const;
  bool
  operator!=(const prodpit &ppit) const
  {
    return !operator==(ppit);
  }
  const prodpit &operator=(const prodpit &spit);
  prodpit &operator++();
  const ParameterSignal &
  signal() const
  {
    return *sig;
  }
  const Vector &
  point() const
  {
    return *p;
  }
  double
  weight() const
  {
    return w;
  }
  void print() const;
protected:
  void setPointAndWeight();
};

/* The product quadrature is just |QuadratureImpl| with the product
   iterator plugged in. The object is constructed by just giving the
   underlying one dimensional quadrature, and the dimension. The only
   extra method is |designLevelForEvals| which for the given maximum
   number of evaluations (and dimension and underlying quadrature from
   the object) returns a maximum level yeilding number of evaluations
   less than the given number. */

class ProductQuadrature : public QuadratureImpl<prodpit>
{
  friend class prodpit;
  const OneDQuadrature &uquad;
public:
  ProductQuadrature(int d, const OneDQuadrature &uq);
  virtual ~ProductQuadrature()
  = default;
  int
  numEvals(int l) const
  {
    int res = 1;
    for (int i = 0; i < dimen(); i++)
      res *= uquad.numPoints(l);
    return res;
  }
  void designLevelForEvals(int max_eval, int &lev, int &evals) const;
protected:
  prodpit begin(int ti, int tn, int level) const;
};

#endif
