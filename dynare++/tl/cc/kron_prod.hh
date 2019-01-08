// Copyright 2004, Ondra Kamenik

// Kronecker product.

/* Here we define an abstraction for a Kronecker product of a sequence of
   matrices. This is $A_1\otimes\ldots\otimes A_n$. Obviously we do not
   store the product in memory. First we need to represent a dimension
   of the Kronecker product. Then we represent the Kronecker product,
   simply it is the Kronecker product dimension with a vector of
   references to the matrices $A_1,\ldots, A_n$.

   The main task of this class is to calculate a matrix product
   $B\cdot(A_1\otimes A_2\otimes\ldots\otimes A_n)$ which in
   our application has much more moderate dimensions than $A_1\otimes
   A_2\otimes\ldots\otimes A_n$. We calculate it as
   $$B\cdot(A_1\otimes I)\cdot\ldots\cdot(I\otimes A_i\otimes
   I)\cdot\ldots\cdot (I\otimes A_n)$$
   where dimensions of identity matrices differ and are given by the
   chosen order. One can naturally ask, whether there is some optimal
   order minimizing maximum storage needed for intermediate
   results. The optimal ordering is implemented by class |KronProdAllOptim|.

   For this multiplication, we also need to represent products of type
   $A\otimes I$, $I\otimes A\otimes I$, and $I\otimes A$. */

#ifndef KRON_PROD_H
#define KRON_PROD_H

#include "twod_matrix.hh"
#include "permutation.hh"
#include "int_sequence.hh"

class KronProdAll;
class KronProdAllOptim;
class KronProdIA;
class KronProdIAI;
class KronProdAI;

/* |KronProdDimens| maintains a dimension of the Kronecker product. So,
   it maintains two sequences, one for rows, and one for columns. */

class KronProdDimens
{
  friend class KronProdAll;
  friend class KronProdAllOptim;
  friend class KronProdIA;
  friend class KronProdIAI;
  friend class KronProdAI;
private:
  IntSequence rows;
  IntSequence cols;
public:
  /* We define three constructors. First initializes to a given
     dimension, and all rows and cols are set to zeros. Second is a copy
     constructor. The third constructor takes dimensions of $A_1\otimes
     A_2\otimes\ldots\otimes A_n$, and makes dimensions of $I\otimes
     A_i\otimes I$, or $I\otimes A_n$, or $A_1\otimes I$ for a given
     $i$. The dimensions of identity matrices are such that
     $$A_1\otimes A_2\otimes\ldots\otimes A_n=
     (A_1\otimes I)\cdot\ldots\cdot(I\otimes A_i\otimes I)
     \cdot\ldots\cdot(I\otimes A_n)$$
     Note that the matrices on the right do not commute only because sizes
     of identity matrices which are then given by this ordering. */
  KronProdDimens(int dim)
    : rows(dim, 0), cols(dim, 0)
  {
  }
  KronProdDimens(const KronProdDimens &kd)
    : rows(kd.rows), cols(kd.cols)
  {
  }
  KronProdDimens(const KronProdDimens &kd, int i);

  const KronProdDimens &
  operator=(const KronProdDimens &kd)
  {
    rows = kd.rows; cols = kd.cols; return *this;
  }
  bool
  operator==(const KronProdDimens &kd) const
  {
    return rows == kd.rows && cols == kd.cols;
  }

  int
  dimen() const
  {
    return rows.size();
  }
  void
  setRC(int i, int r, int c)
  {
    rows[i] = r; cols[i] = c;
  }
  void
  getRC(int i, int &r, int &c) const
  {
    r = rows[i]; c = cols[i];
  }
  void
  getRC(int &r, int &c) const
  {
    r = rows.mult(); c = cols.mult();
  }
  int
  nrows() const
  {
    return rows.mult();
  }
  int
  ncols() const
  {
    return cols.mult();
  }
  int
  nrows(int i) const
  {
    return rows[i];
  }
  int
  ncols(int i) const
  {
    return cols[i];
  }
};

/* Here we define an abstract class for all Kronecker product classes,
   which are |KronProdAll| (the most general), |KronProdIA| (for
   $I\otimes A$), |KronProdAI| (for $A\otimes I$), and |KronProdIAI| (for
   $I\otimes A\otimes I$). The purpose of the super class is to only
   define some common methods and common member |kpd| for dimensions and
   declare pure virtual |mult| which is implemented by the subclasses.

   The class also contains a static method |kronMult|, which calculates a
   Kronecker product of two vectors and stores it in the provided
   vector. It is useful at a few points of the library. */

class KronProd
{
protected:
  KronProdDimens kpd;
public:
  KronProd(int dim)
    : kpd(dim)
  {
  }
  KronProd(const KronProdDimens &kd)
    : kpd(kd)
  {
  }
  KronProd(const KronProd &kp)
    : kpd(kp.kpd)
  {
  }
  virtual ~KronProd()
  {
  }

  int
  dimen() const
  {
    return kpd.dimen();
  }

  virtual void mult(const ConstTwoDMatrix &in, TwoDMatrix &out) const = 0;
  void
  mult(const TwoDMatrix &in, TwoDMatrix &out) const
  {
    mult(ConstTwoDMatrix(in), out);
  }

  void checkDimForMult(const ConstTwoDMatrix &in, const TwoDMatrix &out) const;
  void
  checkDimForMult(const TwoDMatrix &in, const TwoDMatrix &out) const
  {
    checkDimForMult(ConstTwoDMatrix(in), out);
  }

  static void kronMult(const ConstVector &v1, const ConstVector &v2,
                       Vector &res);

  int
  nrows() const
  {
    return kpd.nrows();
  }
  int
  ncols() const
  {
    return kpd.ncols();
  }
  int
  nrows(int i) const
  {
    return kpd.nrows(i);
  }
  int
  ncols(int i) const
  {
    return kpd.ncols(i);
  }
};

/* |KronProdAll| is a main class of this file. It represents the
   Kronecker product $A_1\otimes A_2\otimes\ldots\otimes A_n$. Besides
   dimensions, it stores pointers to matrices in |matlist| array. If a
   pointer is null, then the matrix is considered to be unit. The array
   is set by calls to |setMat| method (for real matrices) or |setUnit|
   method (for unit matrices).

   The object is constructed by a constructor, which allocates the
   |matlist| and initializes dimensions to zeros. Then a caller must feed
   the object with matrices by calling |setMat| and |setUnit| repeatedly
   for different indices.

   We implement the |mult| method of |KronProd|, and a new method
   |multRows|, which creates a vector of kronecker product of all rows of
   matrices in the object. The rows are given by the |IntSequence|. */

class KronProdAll : public KronProd
{
  friend class KronProdIA;
  friend class KronProdIAI;
  friend class KronProdAI;
protected:
  const TwoDMatrix **const matlist;
public:
  KronProdAll(int dim)
    : KronProd(dim), matlist(new const TwoDMatrix *[dim])
  {
  }
  virtual ~KronProdAll()
  {
    delete [] matlist;
  }
  void setMat(int i, const TwoDMatrix &m);
  void setUnit(int i, int n);
  const TwoDMatrix &
  getMat(int i) const
  {
    return *(matlist[i]);
  }

  void mult(const ConstTwoDMatrix &in, TwoDMatrix &out) const;
  Vector *multRows(const IntSequence &irows) const;
private:
  bool isUnit() const;
};

/* The class |KronProdAllOptim| minimizes memory consumption of the
   product $B\cdot(A_1\otimes A_2\otimes\ldots\otimes A_k)$. The
   optimization is done by reordering of the matrices $A_1,\ldots,A_k$,
   in order to minimize a sum of all storages needed for intermediate
   results. The optimal ordering is also nearly optimal with respect to
   number of flops.

   Let $(m_i,n_i)$ be dimensions of $A_i$. It is easy to observe, that
   for $i$-th step we need storage of $r\cdot n_1\cdot\ldots\cdot
   n_i\cdot m_{i+1}\cdot\ldots\cdot m_k$, where $r$ is a number of rows
   of $B$. To minimize the sum through all $i$ over all permutations of
   matrices, it is equivalent to minimize the sum
   $\sum_{i=1}^k{m_{i+1}\cdot\ldots\cdot m_k\over n_{i+1}\cdot\ldots\cdot
   n_k}$. The optimal ordering will yield ${m_k\over
   n_k}\leq{m_{k-1}\over n_{k-1}}\ldots\leq{m_1\over n_1}$.

   Now observe, that the number of flops for $i$-th step is $r\cdot
   n_1\cdot\ldots\cdot n_i\cdot m_i\cdot\ldots\cdot m_k$. In order to
   minimize a number of flops, it is equivalent to minimize
   $\sum_{i=1}^km_i{m_{i+1}\cdot\ldots\cdot m_k\over
   n_{i+1}\cdot\ldots\cdot n_k}$. Note that, normally, the $m_i$ does not
   change as much as $n_{j+1},\ldots,n_k$, so the ordering minimizing the
   memory will be nearly optimal with respect to number of flops.

   The class |KronProdAllOptim| inherits from |KronProdAll|. A public
   method |optimizeOrder| does the reordering. The permutation is stored
   in |oper|. So, as long as |optimizeOrder| is not called, the class is
   equivalent to |KronProdAll|. */

class KronProdAllOptim : public KronProdAll
{
protected:
  Permutation oper;
public:
  KronProdAllOptim(int dim)
    : KronProdAll(dim), oper(dim)
  {
  }
  void optimizeOrder();
  const Permutation &
  getPer() const
  {
    return oper;
  }
};

/* This class represents $I\otimes A$. We have only one reference to
   the matrix, which is set by constructor. */

class KronProdIA : public KronProd
{
  friend class KronProdAll;
  const TwoDMatrix &mat;
public:
  KronProdIA(const KronProdAll &kpa)
    : KronProd(KronProdDimens(kpa.kpd, kpa.dimen()-1)),
      mat(kpa.getMat(kpa.dimen()-1))
  {
  }
  void mult(const ConstTwoDMatrix &in, TwoDMatrix &out) const;
};

/* This class represents $A\otimes I$. We have only one reference to
   the matrix, which is set by constructor. */

class KronProdAI : public KronProd
{
  friend class KronProdIAI;
  friend class KronProdAll;
  const TwoDMatrix &mat;
public:
  KronProdAI(const KronProdAll &kpa)
    : KronProd(KronProdDimens(kpa.kpd, 0)),
      mat(kpa.getMat(0))
  {
  }
  KronProdAI(const KronProdIAI &kpiai);

  void mult(const ConstTwoDMatrix &in, TwoDMatrix &out) const;
};

/* This class represents $I\otimes A\otimes I$. We have only one reference to
   the matrix, which is set by constructor. */

class KronProdIAI : public KronProd
{
  friend class KronProdAI;
  friend class KronProdAll;
  const TwoDMatrix &mat;
public:
  KronProdIAI(const KronProdAll &kpa, int i)
    : KronProd(KronProdDimens(kpa.kpd, i)),
      mat(kpa.getMat(i))
  {
  }
  void mult(const ConstTwoDMatrix &in, TwoDMatrix &out) const;
};

#endif
