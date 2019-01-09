// Copyright 2004, Ondra Kamenik

// Even more general symmetry tensor.

/* Here we define an abstraction for a tensor, which has a general
   symmetry, but the symmetry is not of what is modelled by
   |Symmetry|. This kind of tensor comes to existence when we evaluate
   something like:
   $$\left[B_{y^2u^3}\right]_{\alpha_1\alpha_2\beta_1\beta_2\beta_3}=
   \cdots+\left[g_{y^3}\right]_{\gamma_1\gamma_2\gamma_3}
   \left[g_{yu}\right]^{\gamma_1}_{\alpha_1\beta_3}
   \left[g_{yu}\right]^{\gamma_2}_{\alpha_2\beta_1}
   \left[g_u\right]^{\gamma_3}_{\beta_2}+\cdots
   $$
   If the tensors are unfolded, we obtain a tensor
   $$g_{y^3}\cdot\left(g_{yu}\otimes g_{yu}\otimes g_{u}\right)$$

   Obviously, this tensor can have a symmetry not compatible with
   ordering $\alpha_1\alpha_2\beta_1\beta_2\beta_3$, (in other words, not
   compatible with symmetry $y^2u^3$). In fact, the indices are permuted.

   This kind of tensor must be added to $\left[B_{y^2u^3}\right]$. Its
   dimensions are the same as of $\left[B_{y^2u^3}\right]$, but some
   coordinates are permuted. The addition is the only action we need to
   do with the tensor.

   Another application where this permuted symmetry tensor appears is a
   slice of a fully symmetric tensor. If the symmetric dimension of the
   tensor is partitioned to continuous parts, and we are interested only
   in data with a given symmetry (permuted) of the partitions, then we
   have the permuted symmetry tensor. For instance, if $x$ is partitioned
   $x=[a,b,c,d]$, and having tensor $\left[f_{x^3}\right]$, one can d a
   slice (subtensor) $\left[f_{aca}\right]$. The data of this tensor are
   permuted of $\left[f_{a^c}\right]$.

   Here we also define the folded version of permuted symmetry tensor. It
   has permuted symmetry and is partially folded. One can imagine it as a
   product of a few dimensions, each of them is folded and having a few
   variables. The underlying variables are permuted. The product of such
   dimensions is described by |PerTensorDimens2|. The tensor holding the
   underlying data is |FPSTensor|. */

#ifndef PS_TENSOR_H
#define PS_TENSOR_H

#include "tensor.hh"
#include "gs_tensor.hh"
#include "equivalence.hh"
#include "permutation.hh"
#include "kron_prod.hh"
#include "sparse_tensor.hh"

/* This is just a helper class for ordering a sequence on call stack. */

class SortIntSequence : public IntSequence
{
public:
  SortIntSequence(const IntSequence &s)
    : IntSequence(s)
  {
    sort();
  }
};

/* Here we declare a class describing dimensions of permuted symmetry
   tensor. It inherits from |TensorDimens| and adds a permutation which
   permutes |nvmax|. It has two constructors, each corresponds to a
   context where the tensor appears.

   The first constructor calculates the permutation from a given equivalence.

   The second constructor corresponds to dimensions of a slice. Let us
   take $\left[f_{aca}\right]$ as an example. First it calculates
   |TensorDimens| of $\left[f_{a^c}\right]$, then it calculates a
   permutation corresponding to ordering of $aca$ to $a^2c$, and applies
   this permutation on the dimensions as the first constructor. The
   constructor takes only stack sizes (lengths of $a$, $b$, $c$, and
   $d$), and coordinates of picked partitions.

   Note that inherited methods |calcUnfoldColumns| and |calcFoldColumns|
   work, since number of columns is independent on the permutation, and
   |calcFoldColumns| does not use changed |nvmax|, it uses |nvs|, so it
   is OK. */

class PerTensorDimens : public TensorDimens
{
protected:
  Permutation per;
public:
  PerTensorDimens(const Symmetry &s, const IntSequence &nvars,
                  const Equivalence &e)
    : TensorDimens(s, nvars), per(e)
  {
    per.apply(nvmax);
  }
  PerTensorDimens(const TensorDimens &td, const Equivalence &e)
    : TensorDimens(td), per(e)
  {
    per.apply(nvmax);
  }
  PerTensorDimens(const TensorDimens &td, const Permutation &p)
    : TensorDimens(td), per(p)
  {
    per.apply(nvmax);
  }
  PerTensorDimens(const IntSequence &ss, const IntSequence &coor)
    : TensorDimens(ss, SortIntSequence(coor)), per(coor)
  {
    per.apply(nvmax);
  }
  PerTensorDimens(const PerTensorDimens &td)
     
  = default;
  PerTensorDimens &
  operator=(const PerTensorDimens &td)
  = default;
  bool
  operator==(const PerTensorDimens &td)
  {
    return TensorDimens::operator==(td) && per == td.per;
  }
  int
  tailIdentity() const
  {
    return per.tailIdentity();
  }
  const Permutation &
  getPer() const
  {
    return per;
  }
};

/* Here we declare the permuted symmetry unfolded tensor. It has
   |PerTensorDimens| as a member. It inherits from |UTensor| which
   requires to implement |fold| method. There is no folded counterpart,
   so in our implementation we raise unconditional exception, and return
   some dummy object (just to make it compilable without warnings).

   The class has two sorts of constructors corresponding to a context where it
   appears. The first constructs object from a given matrix, and
   Kronecker product. Within the constructor, all the calculations are
   performed. Also we need to define dimensions, these are the same of
   the resulting matrix (in our example $\left[B_{y^2u^3}\right]$) but
   permuted. The permutation is done in |PerTensorDimens| constructor.

   The second type of constructor is slicing. It makes a slice from
   |FSSparseTensor|. The slice is given by stack sizes, and coordinates of
   picked stacks.

   There are two algorithms for filling a slice of a sparse tensor. The
   first |fillFromSparseOne| works well for more dense tensors, the
   second |fillFromSparseTwo| is better for very sparse tensors. We
   provide a static method, which decides what of the two algorithms is
   better. */

class UPSTensor : public UTensor
{
  const PerTensorDimens tdims;
public:
  // |UPSTensor| constructors from Kronecker product
  /* Here we have four constructors making an |UPSTensor| from a product
     of matrix and Kronecker product. The first constructs the tensor from
     equivalence classes of the given equivalence in an order given by the
     equivalence. The second does the same but with optimized
     |KronProdAllOptim|, which has a different order of matrices than given
     by the classes in the equivalence. This permutation is projected to
     the permutation of the |UPSTensor|. The third, is the same as the
     first, but the classes of the equivalence are permuted by the given
     permutation. Finally, the fourth is the most general combination. It
     allows for a permutation of equivalence classes, and for optimized
     |KronProdAllOptim|, which permutes the permuted equivalence classes. */
  UPSTensor(const TensorDimens &td, const Equivalence &e,
            const ConstTwoDMatrix &a, const KronProdAll &kp)
    : UTensor(along_col, PerTensorDimens(td, e).getNVX(),
              a.nrows(), kp.ncols(), td.dimen()), tdims(td, e)
  {
    kp.mult(a, *this);
  }
  UPSTensor(const TensorDimens &td, const Equivalence &e,
            const ConstTwoDMatrix &a, const KronProdAllOptim &kp)
    : UTensor(along_col, PerTensorDimens(td, Permutation(e, kp.getPer())).getNVX(),
              a.nrows(), kp.ncols(), td.dimen()), tdims(td, Permutation(e, kp.getPer()))
  {
    kp.mult(a, *this);
  }
  UPSTensor(const TensorDimens &td, const Equivalence &e, const Permutation &p,
            const ConstTwoDMatrix &a, const KronProdAll &kp)
    : UTensor(along_col, PerTensorDimens(td, Permutation(e, p)).getNVX(),
              a.nrows(), kp.ncols(), td.dimen()), tdims(td, Permutation(e, p))
  {
    kp.mult(a, *this);
  }
  UPSTensor(const TensorDimens &td, const Equivalence &e, const Permutation &p,
            const ConstTwoDMatrix &a, const KronProdAllOptim &kp)
    : UTensor(along_col, PerTensorDimens(td, Permutation(e, Permutation(p, kp.getPer()))).getNVX(),
              a.nrows(), kp.ncols(), td.dimen()), tdims(td, Permutation(e, Permutation(p, kp.getPer())))
  {
    kp.mult(a, *this);
  }
  UPSTensor(const FSSparseTensor &t, const IntSequence &ss,
            const IntSequence &coor, const PerTensorDimens &ptd);
  UPSTensor(const UPSTensor &ut)
     
  = default;

  void increment(IntSequence &v) const override;
  void decrement(IntSequence &v) const override;
  FTensor&fold() const override;

  int getOffset(const IntSequence &v) const override;
  void addTo(FGSTensor &out) const;
  void addTo(UGSTensor &out) const;

  enum fill_method {first, second};
  static fill_method decideFillMethod(const FSSparseTensor &t);
private:
  int tailIdentitySize() const;
  void fillFromSparseOne(const FSSparseTensor &t, const IntSequence &ss,
                         const IntSequence &coor);
  void fillFromSparseTwo(const FSSparseTensor &t, const IntSequence &ss,
                         const IntSequence &coor);
};

/* Here we define an abstraction for the tensor dimension with the
   symmetry like $xuv\vert uv\vert xu\vert y\vert y\vert x\vert x\vert
   y$. These symmetries come as induces symmetries of equivalence and
   some outer symmetry. Thus the underlying variables are permuted. One
   can imagine the dimensions as an unfolded product of dimensions which
   consist of folded products of variables.

   We inherit from |PerTensorDimens| since we need the permutation
   implied by the equivalence. The new member are the induced symmetries
   (symmetries of each folded dimensions) and |ds| which are sizes of the
   dimensions. The number of folded dimensions is return by |numSyms|.

   The object is constructed from outer tensor dimensions and from
   equivalence with optionally permuted classes. */

class PerTensorDimens2 : public PerTensorDimens
{
  InducedSymmetries syms;
  IntSequence ds;
public:
  PerTensorDimens2(const TensorDimens &td, const Equivalence &e,
                   const Permutation &p)
    : PerTensorDimens(td, Permutation(e, p)),
      syms(e, p, td.getSym()),
      ds(syms.size())
  {
    setDimensionSizes();
  }
  PerTensorDimens2(const TensorDimens &td, const Equivalence &e)
    : PerTensorDimens(td, e),
      syms(e, td.getSym()),
      ds(syms.size())
  {
    setDimensionSizes();
  }
  int
  numSyms() const
  {
    return (int) syms.size();
  }
  const Symmetry &
  getSym(int i) const
  {
    return syms[i];
  }
  int
  calcMaxOffset() const
  {
    return ds.mult();
  }
  int calcOffset(const IntSequence &coor) const;
  void print() const;
protected:
  void setDimensionSizes();
};

/* Here we define an abstraction of the permuted symmetry folded
   tensor. It is needed in context of the Faa Di Bruno formula for folded
   stack container multiplied with container of dense folded tensors, or
   multiplied by one full symmetry sparse tensor.

   For example, if we perform the Faa Di Bruno for $F=f(z)$, where
   $z=[g(x,y,u,v), h(x,y,u), x, y]^T$, we get for one concrete
   equivalence:
   $$
   \left[F_{x^4y^3u^3v^2}\right]=\ldots+
   \left[f_{g^2h^2x^2y}\right]\left(
   [g]_{xv}\otimes[g]_{u^2v}\otimes
   [h]_{xu}\otimes[h]_{y^2}\otimes
   \left[\vphantom{\sum}[I]_x\otimes[I]_x\right]\otimes
   \left[\vphantom{\sum}[I]_y\right]
   \right)
   +\ldots
   $$

   The class |FPSTensor| represents the tensor at the right. Its
   dimension corresponds to a product of 7 dimensions with the following
   symmetries: $xv\vert u^v\vert xu\vert y^2\vert x\vert x\vert y$. Such
   the dimension is described by |PerTensorDimens2|.

   The tensor is constructed in a context of stack container
   multiplication, so, it is constructed from dimensions |td| (dimensions
   of the output tensor), stack product |sp| (implied symmetries picking
   tensors from a stack container, here it is $z$), then a sorted integer
   sequence of the picked stacks of the stack product (it is always
   sorted, here it is $(0,0,1,1,2,2,3)$), then the tensor
   $\left[f_{g^2h^2x^2y}\right]$ (its symmetry must be the same as
   symmetry given by the |istacks|), and finally from the equivalence
   with permuted classes.

   We implement |increment| and |getOffset| methods, |decrement| and
   |unfold| raise an exception. Also, we implement |addTo| method, which
   adds the tensor data (partially unfolded) to folded general symmetry
   tensor. */

template<typename _Ttype>
class StackProduct;

class FPSTensor : public FTensor
{
  const PerTensorDimens2 tdims;
public:
  /* As for |UPSTensor|, we provide four constructors allowing for
     combinations of permuting equivalence classes, and optimization of
     |KronProdAllOptim|. These constructors multiply with dense general
     symmetry tensor (coming from the dense container, or as a dense slice
     of the full symmetry sparse tensor). In addition to these 4
     constructors, we have one constructor multiplying with general
     symmetry sparse tensor (coming as a sparse slice of the full symmetry
     sparse tensor). */
  FPSTensor(const TensorDimens &td, const Equivalence &e,
            const ConstTwoDMatrix &a, const KronProdAll &kp)
    : FTensor(along_col, PerTensorDimens(td, e).getNVX(),
              a.nrows(), kp.ncols(), td.dimen()), tdims(td, e)
  {
    kp.mult(a, *this);
  }
  FPSTensor(const TensorDimens &td, const Equivalence &e,
            const ConstTwoDMatrix &a, const KronProdAllOptim &kp)
    : FTensor(along_col, PerTensorDimens(td, Permutation(e, kp.getPer())).getNVX(),
              a.nrows(), kp.ncols(), td.dimen()), tdims(td, e, kp.getPer())
  {
    kp.mult(a, *this);
  }
  FPSTensor(const TensorDimens &td, const Equivalence &e, const Permutation &p,
            const ConstTwoDMatrix &a, const KronProdAll &kp)
    : FTensor(along_col, PerTensorDimens(td, Permutation(e, p)).getNVX(),
              a.nrows(), kp.ncols(), td.dimen()), tdims(td, e, p)
  {
    kp.mult(a, *this);
  }
  FPSTensor(const TensorDimens &td, const Equivalence &e, const Permutation &p,
            const ConstTwoDMatrix &a, const KronProdAllOptim &kp)
    : FTensor(along_col, PerTensorDimens(td, Permutation(e, Permutation(p, kp.getPer()))).getNVX(),
              a.nrows(), kp.ncols(), td.dimen()), tdims(td, e, Permutation(p, kp.getPer()))
  {
    kp.mult(a, *this);
  }

  FPSTensor(const TensorDimens &td, const Equivalence &e, const Permutation &p,
            const GSSparseTensor &t, const KronProdAll &kp);

  FPSTensor(const FPSTensor &ft)
     
  = default;

  void increment(IntSequence &v) const override;
  void decrement(IntSequence &v) const override;
  UTensor&unfold() const override;

  int getOffset(const IntSequence &v) const override;
  void addTo(FGSTensor &out) const;
};

#endif
