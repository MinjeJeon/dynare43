// Copyright 2004, Ondra Kamenik

#include "kron_prod.hh"
#include "tl_exception.hh"

#include <cstdio>

/* Here we construct Kronecker product dimensions from Kronecker
   product dimensions by picking a given matrix and all other set to
   identity. The constructor takes dimensions of $A_1\otimes
   A_2\otimes\ldots\otimes A_n$, and makes dimensions of $I\otimes
   A_i\otimes I$, or $I\otimes A_n$, or $A_1\otimes I$ for a given
   $i$. The identity matrices must fit into the described order. See
   header file.

   We first decide what is a length of the resulting dimensions. Possible
   length is three for $I\otimes A\otimes I$, and two for $I\otimes A$,
   or $A\otimes I$.

   Then we fork according to |i|. */

KronProdDimens::KronProdDimens(const KronProdDimens &kd, int i)
  : rows((i == 0 || i == kd.dimen()-1) ? (2) : (3)),
    cols((i == 0 || i == kd.dimen()-1) ? (2) : (3))
{
  TL_RAISE_IF(i < 0 || i >= kd.dimen(),
              "Wrong index for pickup in KronProdDimens constructor");

  int kdim = kd.dimen();
  if (i == 0)
    {
      // set AI dimensions
      /* The first rows and cols are taken from |kd|. The dimensions of
         identity matrix is a number of rows in $A_2\otimes\ldots\otimes A_n$
         since the matrix $A_1\otimes I$ is the first. */
      rows[0] = kd.rows[0];
      rows[1] = kd.rows.mult(1, kdim);
      cols[0] = kd.cols[0];
      cols[1] = rows[1];
    }
  else if (i == kdim-1)
    {
      // set IA dimensions
      /* The second dimension is taken from |kd|. The dimensions of identity
         matrix is a number of columns of $A_1\otimes\ldots A_{n-1}$, since the
         matrix $I\otimes A_n$ is the last. */
      rows[0] = kd.cols.mult(0, kdim-1);
      rows[1] = kd.rows[kdim-1];
      cols[0] = rows[0];
      cols[1] = kd.cols[kdim-1];
    }
  else
    {
      // set IAI dimensions
      /* The dimensions of the middle matrix are taken from |kd|. The
         dimensions of the first identity matrix are a number of columns of
         $A_1\otimes\ldots\otimes A_{i-1}$, and the dimensions of the last
         identity matrix are a number of rows of $A_{i+1}\otimes\ldots\otimes
         A_n$. */
      rows[0] = kd.cols.mult(0, i);
      cols[0] = rows[0];
      rows[1] = kd.rows[i];
      cols[1] = kd.cols[i];
      cols[2] = kd.rows.mult(i+1, kdim);
      rows[2] = cols[2];
    }
}

/* This raises an exception if dimensions are bad for multiplication
   |out = in*this|. */

void
KronProd::checkDimForMult(const ConstTwoDMatrix &in, const TwoDMatrix &out) const
{
  int my_rows;
  int my_cols;
  kpd.getRC(my_rows, my_cols);
  TL_RAISE_IF(in.nrows() != out.nrows() || in.ncols() != my_rows,
              "Wrong dimensions for KronProd in KronProd::checkDimForMult");
}

/* Here we Kronecker multiply two given vectors |v1| and |v2| and
   store the result in preallocated |res|. */

void
KronProd::kronMult(const ConstVector &v1, const ConstVector &v2,
                   Vector &res)
{
  TL_RAISE_IF(res.length() != v1.length()*v2.length(),
              "Wrong vector lengths in KronProd::kronMult");
  res.zeros();
  for (int i = 0; i < v1.length(); i++)
    {
      Vector sub(res, i *v2.length(), v2.length());
      sub.add(v1[i], v2);
    }
}

void
KronProdAll::setMat(int i, const TwoDMatrix &m)
{
  matlist[i] = &m;
  kpd.setRC(i, m.nrows(), m.ncols());
}

void
KronProdAll::setUnit(int i, int n)
{
  matlist[i] = NULL;
  kpd.setRC(i, n, n);
}

bool
KronProdAll::isUnit() const
{
  int i = 0;
  while (i < dimen() && matlist[i] == NULL)
    i++;
  return i == dimen();
}

/* Here we multiply $B\cdot(I\otimes A)$. If $m$ is a dimension of the
   identity matrix, then the product is equal to
   $B\cdot\hbox{diag}_m(A)$. If $B$ is partitioned accordingly, then the
   result is $[B_1A, B_2A,\ldots B_mA]$.

   Here, |outi| are partitions of |out|, |ini| are const partitions of
   |in|, and |id_cols| is $m$. We employ level-2 BLAS. */

void
KronProdIA::mult(const ConstTwoDMatrix &in, TwoDMatrix &out) const
{
  checkDimForMult(in, out);

  int id_cols = kpd.cols[0];
  ConstTwoDMatrix a(mat);

  for (int i = 0; i < id_cols; i++)
    {
      TwoDMatrix outi(out, i *a.ncols(), a.ncols());
      ConstTwoDMatrix ini(in, i *a.nrows(), a.nrows());
      outi.mult(ini, a);
    }
}

/* Here we construct |KronProdAI| from |KronProdIAI|. It is clear. */
KronProdAI::KronProdAI(const KronProdIAI &kpiai)
  : KronProd(KronProdDimens(2)), mat(kpiai.mat)
{
  kpd.rows[0] = mat.nrows();
  kpd.cols[0] = mat.ncols();
  kpd.rows[1] = kpiai.kpd.rows[2];
  kpd.cols[1] = kpiai.kpd.cols[2];
}

/* Here we multiply $B\cdot(A\otimes I)$. Let the dimension of the
   matrix $A$ be $m\times n$, the dimension of $I$ be $p$, and a number
   of rows of $B$ be $q$. We use the fact that $B\cdot(A\otimes
   I)=\hbox{reshape}(\hbox{reshape}(B, q, mp)\cdot A, q, np)$. This works
   only for matrix $B$, whose storage has leading dimension equal to
   number of rows.

   For cases where the leading dimension is not equal to the number of
   rows, we partition the matrix $A\otimes I$ to $m\times n$ square
   partitions $a_{ij}I$. Therefore, we partition $B$ to $m$ partitions
   $[B_1, B_2,\ldots,B_m]$. Each partition of $B$ has the same number of
   columns as the identity matrix. If $R$ denotes the resulting matrix,
   then it can be partitioned to $n$ partitions
   $[R_1,R_2,\ldots,R_n]$. Each partition of $R$ has the same number of
   columns as the identity matrix. Then we have $R_i=\sum a_{ji}B_j$.

   In code, |outi| is $R_i$, |ini| is $B_j$, and |id_cols| is a dimension
   of the identity matrix */

void
KronProdAI::mult(const ConstTwoDMatrix &in, TwoDMatrix &out) const
{
  checkDimForMult(in, out);

  int id_cols = kpd.cols[1];
  ConstTwoDMatrix a(mat);

  if (in.getLD() == in.nrows())
    {
      ConstTwoDMatrix in_resh(in.nrows()*id_cols, a.nrows(), in.getData().base());
      TwoDMatrix out_resh(in.nrows()*id_cols, a.ncols(), out.getData().base());
      out_resh.mult(in_resh, a);
    }
  else
    {
      out.zeros();
      for (int i = 0; i < a.ncols(); i++)
        {
          TwoDMatrix outi(out, i *id_cols, id_cols);
          for (int j = 0; j < a.nrows(); j++)
            {
              ConstTwoDMatrix ini(in, j *id_cols, id_cols);
              outi.add(a.get(j, i), ini);
            }
        }
    }
}

/* Here we multiply $B\cdot(I\otimes A\otimes I)$. If $n$ is a
   dimension of the first identity matrix, then we multiply
   $B\cdot\hbox{diag}_n(A\otimes I)$. So we partition $B$ and result $R$
   accordingly, and multiply $B_i\cdot(A\otimes I)$, which is in fact
   |KronProdAI::mult|. Note that number of columns of partitions of $B$
   are number of rows of $A\otimes I$, and number of columns of $R$ are
   number of columns of $A\otimes I$.

   In code, |id_cols| is $n$, |akronid| is a Kronecker product object of
   $A\otimes I$, and |in_bl_width|, and |out_bl_width| are rows and cols of
   $A\otimes I$. */

void
KronProdIAI::mult(const ConstTwoDMatrix &in, TwoDMatrix &out) const
{
  checkDimForMult(in, out);

  int id_cols = kpd.cols[0];

  KronProdAI akronid(*this);
  int in_bl_width;
  int out_bl_width;
  akronid.kpd.getRC(in_bl_width, out_bl_width);

  for (int i = 0; i < id_cols; i++)
    {
      TwoDMatrix outi(out, i *out_bl_width, out_bl_width);
      ConstTwoDMatrix ini(in, i *in_bl_width, in_bl_width);
      akronid.mult(ini, outi);
    }
}

/* Here we multiply $B\cdot(A_1\otimes\ldots\otimes A_n)$. First we
   multiply $B\cdot(A_1\otimes)$, then this is multiplied by all
   $I\otimes A_i\otimes I$, and finally by $I\otimes A_n$.

   If the dimension of the Kronecker product is only 1, then we multiply
   two matrices in straight way and return.

   The intermediate results are stored on heap pointed by |last|. A new
   result is allocated, and then the former storage is deallocated.

   We have to be careful in cases when last or first matrix is unit and
   no calculations are performed in corresponding codes. The codes should
   handle |last| safely also if no calcs are done. */

void
KronProdAll::mult(const ConstTwoDMatrix &in, TwoDMatrix &out) const
{
  // quick copy if product is unit
  if (isUnit())
    {
      out.zeros();
      out.add(1.0, in);
      return;
    }

  // quick zero if one of the matrices is zero
  /* If one of the matrices is exactly zero or the |in| matrix is zero,
     set out to zero and return */
  bool is_zero = false;
  for (int i = 0; i < dimen() && !is_zero; i++)
    is_zero = matlist[i] && matlist[i]->isZero();
  if (is_zero || in.isZero())
    {
      out.zeros();
      return;
    }

  // quick multiplication if dimension is 1
  if (dimen() == 1)
    {
      if (matlist[0]) // always true
        out.mult(in, ConstTwoDMatrix(*(matlist[0])));
      return;
    }

  int c;
  TwoDMatrix *last = NULL;

  // perform first multiplication AI
  /* Here we have to construct $A_1\otimes I$, allocate intermediate
     result |last|, and perform the multiplication. */
  if (matlist[0])
    {
      KronProdAI akronid(*this);
      c = akronid.kpd.ncols();
      last = new TwoDMatrix(in.nrows(), c);
      akronid.mult(in, *last);
    }
  else
    {
      last = new TwoDMatrix(in.nrows(), in.ncols(), in.getData().base());
    }

  // perform intermediate multiplications IAI
  /* Here we go through all $I\otimes A_i\otimes I$, construct the
     product, allocate new storage for result |newlast|, perform the
     multiplication, deallocate old |last|, and set |last| to |newlast|. */
  for (int i = 1; i < dimen()-1; i++)
    {
      if (matlist[i])
        {
          KronProdIAI interkron(*this, i);
          c = interkron.kpd.ncols();
          TwoDMatrix *newlast = new TwoDMatrix(in.nrows(), c);
          interkron.mult(*last, *newlast);
          delete last;
          last = newlast;
        }
    }

  // perform last multiplication IA
  /* Here just construct $I\otimes A_n$ and perform multiplication and
     deallocate |last|. */
  if (matlist[dimen()-1])
    {
      KronProdIA idkrona(*this);
      idkrona.mult(*last, out);
    }
  else
    {
      out = *last;
    }
  delete last;
}

/* This calculates a Kornecker product of rows of matrices, the row
   indices are given by the integer sequence. The result is allocated and
   returned. The caller is repsonsible for its deallocation. */

Vector *
KronProdAll::multRows(const IntSequence &irows) const
{
  TL_RAISE_IF(irows.size() != dimen(),
              "Wrong length of row indices in KronProdAll::multRows");

  Vector *last = NULL;
  ConstVector *row;
  vector<Vector *> to_delete;
  for (int i = 0; i < dimen(); i++)
    {
      int j = dimen()-1-i;

      // set |row| to the row of |j|-th matrix
      /* If the |j|-th matrix is real matrix, then the row is constructed
         from the matrix. It the matrix is unit, we construct a new vector,
         fill it with zeros, than set the unit to appropriate place, and make
         the |row| as ConstVector of this vector, which sheduled for
         deallocation. */
      if (matlist[j])
        row = new ConstVector(irows[j], *(matlist[j]));
      else
        {
          Vector *aux = new Vector(ncols(j));
          aux->zeros();
          (*aux)[irows[j]] = 1.0;
          to_delete.push_back(aux);
          row = new ConstVector(*aux);
        }

      // set |last| to product of |row| and |last|
      /* If the |last| is exists, we allocate new storage, Kronecker
         multiply, deallocate the old storage. If the |last| does not exist,
         then we only make |last| equal to |row|. */
      if (last)
        {
          Vector *newlast;
          newlast = new Vector(last->length()*row->length());
          kronMult(*row, ConstVector(*last), *newlast);
          delete last;
          last = newlast;
        }
      else
        {
          last = new Vector(*row);
        }

      delete row;
    }

  for (auto & i : to_delete)
    delete i;

  return last;
}

/* This permutes the matrices so that the new ordering would minimize
   memory consumption. As shown in |@<|KronProdAllOptim| class declaration@>|,
   we want ${m_k\over n_k}\leq{m_{k-1}\over n_{k-1}}\ldots\leq{m_1\over n_1}$,
   where $(m_i,n_i)$ is the dimension of $A_i$. So we implement the bubble
   sort. */

void
KronProdAllOptim::optimizeOrder()
{
  for (int i = 0; i < dimen(); i++)
    {
      int swaps = 0;
      for (int j = 0; j < dimen()-1; j++)
        {
          if (((double) kpd.rows[j])/kpd.cols[j] < ((double) kpd.rows[j+1])/kpd.cols[j+1])
            {
              // swap dimensions and matrices at |j| and |j+1|
              int s = kpd.rows[j+1];
              kpd.rows[j+1] = kpd.rows[j];
              kpd.rows[j] = s;
              s = kpd.cols[j+1];
              kpd.cols[j+1] = kpd.cols[j];
              kpd.cols[j] = s;
              const TwoDMatrix *m = matlist[j+1];
              matlist[j+1] = matlist[j];
              matlist[j] = m;

              // project the swap to the permutation |oper|
              s = oper.getMap()[j+1];
              oper.getMap()[j+1] = oper.getMap()[j];
              oper.getMap()[j] = s;
              swaps++;
            }
        }
      if (swaps == 0)
        {
          return;
        }
    }
}
