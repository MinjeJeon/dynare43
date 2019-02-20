// Copyright 2004, Ondra Kamenik

// Tensor polynomial evaluation.

/* We need to evaluate a tensor polynomial of the form:
   $$
   \left[g_{x}\right]_{\alpha_1}[x]^{\alpha_1}+
   \left[g_{x^2}\right]_{\alpha_1\alpha_2}[x]^{\alpha_1}[x]^{\alpha_2}+
   \ldots+
   \left[g_{x^n}\right]_{\alpha_1\ldots\alpha_n}\prod_{i=1}^n[x]^{\alpha_i}
   $$
   where $x$ is a column vector.

   We have basically two options. The first is to use the formula above,
   the second is to use a Horner-like formula:
   $$
   \left[\cdots\left[\left[\left[g_{x^{n-1}}\right]+
   \left[g_{x^n}\right]_{\alpha_1\ldots\alpha_{n-1}\alpha_n}
   [x]^{\alpha_n}\right]_{\alpha_1\ldots\alpha_{n-2}\alpha_{n-1}}
   [x]^{\alpha_{n-1}}\right]\cdots\right]_{\alpha_1}
   [x]^{\alpha_1}
   $$

   Alternativelly, we can put the the polynomial into a more compact form
   $$\left[g_{x}\right]_{\alpha_1}[x]^{\alpha_1}+
   \left[g_{x^2}\right]_{\alpha_1\alpha_2}[x]^{\alpha_1}[x]^{\alpha_2}+
   \ldots+
   \left[g_{x^n}\right]_{\alpha_1\ldots\alpha_n}\prod_{i=1}^n[x]^{\alpha_i}
   = [G]_{\alpha_1\ldots\alpha_n}\prod_{i=1}^n\left[\matrix{1\cr x}\right]^{\alpha_i}
   $$
   Then the polynomial evaluation becomes just a matrix multiplication of the vector power.

   Here we define the tensor polynomial as a container of full symmetry
   tensors and add an evaluation methods. We have two sorts of
   containers, folded and unfolded. For each type we declare two methods
   implementing the above formulas. We define classes for the
   compactification of the polynomial. The class derives from the tensor
   and has a eval method. */

#include "t_container.hh"
#include "fs_tensor.hh"
#include "rfs_tensor.hh"
#include "tl_static.hh"
#include "pascal_triangle.hh"

/* Just to make the code nicer, we implement a Kronecker power of a
   vector encapsulated in the following class. It has |getNext| method
   which returns either folded or unfolded row-oriented single column
   Kronecker power of the vector according to the type of a dummy
   argument. This allows us to use the type dependent code in templates
   below.

   The implementation of the Kronecker power is that we maintain the last
   unfolded power. If unfolded |getNext| is called, we Kronecker multiply
   the last power with a vector and return it. If folded |getNext| is
   called, we do the same plus we fold it.

   |getNext| returns the vector for the first call (first power), the
   second power is returned on the second call, and so on. */

class PowerProvider
{
  Vector origv;
  URSingleTensor *ut;
  FRSingleTensor *ft;
  int nv;
public:
  PowerProvider(const ConstVector &v)
    : origv(v), ut(nullptr), ft(nullptr), nv(v.length())
  {
  }
  ~PowerProvider();
  const URSingleTensor&getNext(const URSingleTensor *dummy);
  const FRSingleTensor&getNext(const FRSingleTensor *dummy);
};

/* The tensor polynomial is basically a tensor container which is more
   strict on insertions. It maintains number of rows and number of
   variables and allows insertions only of those tensors, which yield
   these properties. The maximum dimension is maintained by |insert|
   method.

   So we re-implement |insert| method and implement |evalTrad|
   (traditional polynomial evaluation) and horner-like evaluation
   |evalHorner|.

   In addition, we implement derivatives of the polynomial and its
   evaluation. The evaluation of a derivative is different from the
   evaluation of the whole polynomial, simply because the evaluation of
   the derivatives is a tensor, and the evaluation of the polynomial is a
   vector (zero dimensional tensor). See documentation to
   |@<|TensorPolynomial::derivative| code@>| and
   |@<|TensorPolynomial::evalPartially| code@>| for details. */

template <class _Ttype, class _TGStype, class _Stype>
class TensorPolynomial : public TensorContainer<_Ttype>
{
  int nr;
  int nv;
  int maxdim;
  using _Tparent = TensorContainer<_Ttype>;
public:
  TensorPolynomial(int rows, int vars)
    : TensorContainer<_Ttype>(1),
    nr(rows), nv(vars), maxdim(0)
  {
  }
  TensorPolynomial(const TensorPolynomial<_Ttype, _TGStype, _Stype> &tp, int k)
    : TensorContainer<_Ttype>(tp),
    nr(tp.nr), nv(tp.nv), maxdim(0)
  {
    derivative(k);
  }
  TensorPolynomial(int first_row, int num, TensorPolynomial<_Ttype, _TGStype, _Stype> &tp)
    : TensorContainer<_Ttype>(first_row, num, tp),
    nr(num), nv(tp.nv), maxdim(tp.maxdim)
  {
  }

  // |TensorPolynomial| contract constructor code@
  /* This constructor takes a tensor polynomial
     $$P(x,y)=\sum^m_{k=0}[g_{(xy)^k}]_{\alpha_1\ldots\alpha_k}
     \left[\matrix{x\cr y}\right]^{\alpha_1\ldots\alpha_k}$$
     and for a given $x$ it makes a polynomial
     $$Q(y)=P(x,y).$$

     The algorithm for each full symmetry $(xy)^k$ works with subtensors (slices) of
     symmetry $x^iy^j$ (with $i+j=k$), and contracts these subtensors with respect to
     $x^i$ to obtain a tensor of full symmetry $y^j$. Since the column
     $x^i$ is calculated by |PowerProvider| we cycle for $i=1,...,m$. Then
     we have to add everything for $i=0$.

     The code works as follows: For slicing purposes we need stack sizes
     |ss| corresponing to lengths of $x$ and $y$, and then identity |pp|
     for unfolding a symmetry of the slice to obtain stack coordinates of
     the slice. Then we do the calculations for $i=1,\ldots,m$ and then for
     $i=0$. */

  TensorPolynomial(const TensorPolynomial<_Ttype, _TGStype, _Stype> &tp, const Vector &xval)
    : TensorContainer<_Ttype>(1),
    nr(tp.nrows()), nv(tp.nvars() - xval.length()), maxdim(0)
  {
    TL_RAISE_IF(nvars() < 0,
                "Length of xval too big in TensorPolynomial contract constructor");
    IntSequence ss{xval.length(), nvars()};
    IntSequence pp{0, 1};

    // do contraction for all $i>0$
    /* Here we setup the |PowerProvider|, and cycle through
       $i=1,\ldots,m$. Within the loop we cycle through $j=0,\ldots,m-i$. If
       there is a tensor with symmetry $(xy)^{i+j}$ in the original
       polynomial, we make its slice with symmetry $x^iy^j$, and
       |contractAndAdd| it to the tensor |ten| in the |this| polynomial with
       a symmetry $y^j$.

       Note three things: First, the tensor |ten| is either created and put
       to |this| container or just got from the container, this is done in
       |@<initialize |ten| of dimension |j|@>|. Second, the contribution to
       the |ten| tensor must be multiplied by $\left(\matrix{i+j\cr
       j}\right)$, since there are exactly that number of slices of
       $(xy)^{i+j}$ of the symmetry $x^iy^j$ and all must be added. Third,
       the tensor |ten| is fully symmetric and |_TGStype::contractAndAdd|
       works with general symmetry, that is why we have to in-place convert
       fully syummetric |ten| to a general symmetry tensor. */
    PowerProvider pwp(xval);
    for (int i = 1; i <= tp.maxdim; i++)
      {
        const _Stype &xpow = pwp.getNext((const _Stype *) nullptr);
        for (int j = 0; j <= tp.maxdim-i; j++)
          {
            if (tp.check(Symmetry{i+j}))
              {
                // initialize |ten| of dimension |j|
                /* The pointer |ten| is either a new tensor or got from |this| container. */
                _Ttype *ten;
                if (_Tparent::check(Symmetry{j}))
                  ten = &_Tparent::get(Symmetry{j});
                else
                  {
                    auto ten_smart = std::make_unique<_Ttype>(nrows(), nvars(), j);
                    ten_smart->zeros();
                    ten = ten_smart.get();
                    insert(std::move(ten_smart));
                  }

                Symmetry sym{i, j};
                IntSequence coor(sym, pp);
                _TGStype slice(tp.get(Symmetry{i+j}), ss, coor, TensorDimens(sym, ss));
                slice.mult(PascalTriangle::noverk(i+j, j));
                _TGStype tmp(*ten);
                slice.contractAndAdd(0, tmp, xpow);
              }
          }
      }

    // do contraction for $i=0$
    /* This is easy. The code is equivalent to code |@<do contraction for
       all $i>0$@>| as for $i=0$. The contraction here takes a form of a
       simple addition. */
    for (int j = 0; j <= tp.maxdim; j++)
      {
        if (tp.check(Symmetry{j}))
          {

            // initialize |ten| of dimension |j|
            /* Same code as above */
            _Ttype *ten;
            if (_Tparent::check(Symmetry{j}))
              ten = &_Tparent::get(Symmetry{j});
            else
              {
                auto ten_smart = std::make_unique<_Ttype>(nrows(), nvars(), j);
                ten_smart->zeros();
                ten = ten_smart.get();
                insert(std::move(ten_smart));
              }

            Symmetry sym{0, j};
            IntSequence coor(sym, pp);
            _TGStype slice(tp.get(Symmetry{j}), ss, coor, TensorDimens(sym, ss));
            ten->add(1.0, slice);
          }
      }
  }

  TensorPolynomial(const TensorPolynomial &tp)
    : TensorContainer<_Ttype>(tp), nr(tp.nr), nv(tp.nv), maxdim(tp.maxdim)
  {
  }
  int
  nrows() const
  {
    return nr;
  }
  int
  nvars() const
  {
    return nv;
  }

  /* Here we cycle up to the maximum dimension, and if a tensor exists in
     the container, then we multiply it with the Kronecker power of the
     vector supplied by |PowerProvider|. */

  void
  evalTrad(Vector &out, const ConstVector &v) const
  {
    if (_Tparent::check(Symmetry{0}))
      out = _Tparent::get(Symmetry{0}).getData();
    else
      out.zeros();

    PowerProvider pp(v);
    for (int d = 1; d <= maxdim; d++)
      {
        const _Stype &p = pp.getNext((const _Stype *) nullptr);
        Symmetry cs{d};
        if (_Tparent::check(cs))
          {
            const _Ttype &t = _Tparent::get(cs);
            t.multaVec(out, p.getData());
          }
      }
  }

  /* Here we construct by contraction |maxdim-1| tensor first, and then
     cycle. The code is clear, the only messy thing is |new| and |delete|. */

  void
  evalHorner(Vector &out, const ConstVector &v) const
  {
    if (_Tparent::check(Symmetry{0}))
      out = _Tparent::get(Symmetry{0}).getData();
    else
      out.zeros();

    if (maxdim == 0)
      return;

    _Ttype *last;
    if (maxdim == 1)
      last = new _Ttype(_Tparent::get(Symmetry{1}));
    else
      last = new _Ttype(_Tparent::get(Symmetry{maxdim}), v);
    for (int d = maxdim-1; d >= 1; d--)
      {
        Symmetry cs{d};
        if (_Tparent::check(cs))
          {
            const _Ttype &nt = _Tparent::get(cs);
            last->add(1.0, ConstTwoDMatrix(nt));
          }
        if (d > 1)
          {
            auto *new_last = new _Ttype(*last, v);
            delete last;
            last = new_last;
          }
      }
    last->multaVec(out, v);
    delete last;
  }

  /* Before a tensor is inserted, we check for the number of rows, and
     number of variables. Then we insert and update the |maxdim|. */

  void
  insert(std::unique_ptr<_Ttype> t) override
  {
    TL_RAISE_IF(t->nrows() != nr,
                "Wrong number of rows in TensorPolynomial::insert");
    TL_RAISE_IF(t->nvar() != nv,
                "Wrong number of variables in TensorPolynomial::insert");
    if (maxdim < t->dimen())
      maxdim = t->dimen();
    TensorContainer<_Ttype>::insert(std::move(t));
  }

  /* The polynomial takes the form
     $$\sum_{i=0}^n{1\over i!}\left[g_{y^i}\right]_{\alpha_1\ldots\alpha_i}
     \left[y\right]^{\alpha_1}\ldots\left[y\right]^{\alpha_i},$$ where
     $\left[g_{y^i}\right]$ are $i$-order derivatives of the polynomial. We
     assume that ${1\over i!}\left[g_{y^i}\right]$ are items in the tensor
     container.  This method differentiates the polynomial by one order to
     yield:
     $$\sum_{i=1}^n{1\over i!}\left[i\cdot g_{y^i}\right]_{\alpha_1\ldots\alpha_i}
     \left[y\right]^{\alpha_1}\ldots\left[y\right]^{\alpha_{i-1}},$$
     where $\left[i\cdot{1\over i!}\cdot g_{y^i}\right]$ are put to the container.

     A polynomial can be derivative of some order, and the order cannot be
     recognized from the object. That is why we need to input the order. */

  void
  derivative(int k)
  {
    for (int d = 1; d <= maxdim; d++)
      {
        if (_Tparent::check(Symmetry{d}))
          {
            _Ttype &ten = _Tparent::get(Symmetry{d});
            ten.mult((double) std::max((d-k), 0));
          }
      }
  }

  /* Now let us suppose that we have an |s| order derivative of a
     polynomial whose $i$ order derivatives are $\left[g_{y^i}\right]$, so
     we have
     $$\sum_{i=s}^n{1\over i!}\left[g_{y^i}\right]_{\alpha_1\ldots\alpha_i}
     \prod_{k=1}^{i-s}\left[y\right]^{\alpha_k},$$
     where ${1\over i!}\left[g_{y^i}\right]$ are tensors in the container.

     This methods performs this evaluation. The result is an |s| dimensional
     tensor. Note that when combined with the method |derivative|, they
     evaluate a derivative of some order. For example a sequence of calls
     |g.derivative(0)|, |g.derivative(1)| and |der=g.evalPartially(2, v)|
     calculates $2!$ multiple of the second derivative of |g| at |v|. */

  std::unique_ptr<_Ttype>
  evalPartially(int s, const ConstVector &v)
  {
    TL_RAISE_IF(v.length() != nvars(),
                "Wrong length of vector for TensorPolynomial::evalPartially");

    auto res = std::make_unique<_Ttype>(nrows(), nvars(), s);
    res->zeros();

    if (_Tparent::check(Symmetry{s}))
      res->add(1.0, _Tparent::get(Symmetry{s}));

    for (int d = s+1; d <= maxdim; d++)
      {
        if (_Tparent::check(Symmetry{d}))
          {
            const _Ttype &ltmp = _Tparent::get(Symmetry{d});
            auto last = std::make_unique<_Ttype>(ltmp);
            for (int j = 0; j < d - s; j++)
              {
                auto newlast = std::make_unique<_Ttype>(*last, v);
                last = std::move(newlast);
              }
            res->add(1.0, *last);
          }
      }

    return res;
  }
};

/* This just gives a name to unfolded tensor polynomial. */

class FTensorPolynomial;
class UTensorPolynomial : public TensorPolynomial<UFSTensor, UGSTensor, URSingleTensor>
{
public:
  UTensorPolynomial(int rows, int vars)
    : TensorPolynomial<UFSTensor, UGSTensor, URSingleTensor>(rows, vars)
  {
  }
  UTensorPolynomial(const UTensorPolynomial &up, int k)
    : TensorPolynomial<UFSTensor, UGSTensor, URSingleTensor>(up, k)
  {
  }
  UTensorPolynomial(const FTensorPolynomial &fp);
  UTensorPolynomial(const UTensorPolynomial &tp, const Vector &xval)
    : TensorPolynomial<UFSTensor, UGSTensor, URSingleTensor>(tp, xval)
  {
  }
  UTensorPolynomial(int first_row, int num, UTensorPolynomial &tp)
    : TensorPolynomial<UFSTensor, UGSTensor, URSingleTensor>(first_row, num, tp)
  {
  }
};

/* This just gives a name to folded tensor polynomial. */

class FTensorPolynomial : public TensorPolynomial<FFSTensor, FGSTensor, FRSingleTensor>
{
public:
  FTensorPolynomial(int rows, int vars)
    : TensorPolynomial<FFSTensor, FGSTensor, FRSingleTensor>(rows, vars)
  {
  }
  FTensorPolynomial(const FTensorPolynomial &fp, int k)
    : TensorPolynomial<FFSTensor, FGSTensor, FRSingleTensor>(fp, k)
  {
  }
  FTensorPolynomial(const UTensorPolynomial &up);
  FTensorPolynomial(const FTensorPolynomial &tp, const Vector &xval)
    : TensorPolynomial<FFSTensor, FGSTensor, FRSingleTensor>(tp, xval)
  {
  }
  FTensorPolynomial(int first_row, int num, FTensorPolynomial &tp)
    : TensorPolynomial<FFSTensor, FGSTensor, FRSingleTensor>(first_row, num, tp)
  {
  }
};

/* The compact form of |TensorPolynomial| is in fact a full symmetry
   tensor, with the number of variables equal to the number of variables
   of the polynomial plus 1 for $1$. */

template <class _Ttype, class _TGStype, class _Stype>
class CompactPolynomial : public _Ttype
{
public:
  /* This constructor copies matrices from the given tensor polynomial to
     the appropriate location in this matrix. It creates a dummy tensor
     |dum| with two variables (one corresponds to $1$, the other to
     $x$). The index goes through this dummy tensor and the number of
     columns of the folded/unfolded general symmetry tensor corresponding
     to the selections of $1$ or $x$ given by the index. Length of $1$ is
     one, and length of $x$ is |pol.nvars()|. This nvs information is
     stored in |dumnvs|. The symmetry of this general symmetry dummy tensor
     |dumgs| is given by a number of ones and x's in the index. We then
     copy the matrix, if it exists in the polynomial and increase |offset|
     for the following cycle. */

  CompactPolynomial(const TensorPolynomial<_Ttype, _TGStype, _Stype> &pol)
    : _Ttype(pol.nrows(), pol.nvars()+1, pol.getMaxDim())
  {
    _Ttype::zeros();

    IntSequence dumnvs{1, pol.nvars()};

    int offset = 0;
    _Ttype dum(0, 2, _Ttype::dimen());
    for (Tensor::index i = dum.begin(); i != dum.end(); ++i)
      {
        int d = i.getCoor().sum();
        Symmetry symrun{_Ttype::dimen()-d, d};
        _TGStype dumgs(0, TensorDimens(symrun, dumnvs));
        if (pol.check(Symmetry{d}))
          {
            TwoDMatrix subt(*this, offset, dumgs.ncols());
            subt.add(1.0, pol.get(Symmetry{d}));
          }
        offset += dumgs.ncols();
      }
  }

  /* We create |x1| to be a concatenation of $1$ and $x$, and then create
     |PowerProvider| to make a corresponding power |xpow| of |x1|, and
     finally multiply this matrix with the power. */

  void
  eval(Vector &out, const ConstVector &v) const
  {
    TL_RAISE_IF(v.length()+1 != _Ttype::nvar(),
                "Wrong input vector length in CompactPolynomial::eval");
    TL_RAISE_IF(out.length() != _Ttype::nrows(),
                "Wrong output vector length in CompactPolynomial::eval");

    Vector x1(v.length()+1);
    Vector x1p(x1, 1, v.length());
    x1p = v;
    x1[0] = 1.0;

    if (_Ttype::dimen() == 0)
      out = ConstVector(*this, 0);
    else
      {
        PowerProvider pp(x1);
        const _Stype &xpow = pp.getNext((const _Stype *) NULL);
        for (int i = 1; i < _Ttype::dimen(); i++)
          xpow = pp.getNext((const _Stype *) NULL);
        multVec(0.0, out, 1.0, xpow);
      }
  }
};

/* Specialization of the |CompactPolynomial| for unfolded tensor. */
class UCompactPolynomial : public CompactPolynomial<UFSTensor, UGSTensor, URSingleTensor>
{
public:
  UCompactPolynomial(const UTensorPolynomial &upol)
    : CompactPolynomial<UFSTensor, UGSTensor, URSingleTensor>(upol)
  {
  }
};

/* Specialization of the |CompactPolynomial| for folded tensor. */
class FCompactPolynomial : public CompactPolynomial<FFSTensor, FGSTensor, FRSingleTensor>
{
public:
  FCompactPolynomial(const FTensorPolynomial &fpol)
    : CompactPolynomial<FFSTensor, FGSTensor, FRSingleTensor>(fpol)
  {
  }
};
