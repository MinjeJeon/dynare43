// Copyright 2004, Ondra Kamenik

// General symmetry tensor.

/* Here we define tensors for general symmetry. All tensors from here are
   identifying the multidimensional index with columns. Thus all
   symmetries regard to columns. The general symmetry here is not the most
   general. It captures all symmetries of indices which are given by
   continuous partitioning of indices. Two items are symmetric if they
   belong to the same group. The continuity implies that if two items
   belong to one group, then all items between them belong to that
   group. This continuous partitioning of indices is described by
   |Symmetry| class.

   The dimension of the tensors here are described (besides the symmetry)
   also by number of variables for each group. This is dealt in the class
   for tensor dimensions defined also here. */

#ifndef GS_TENSOR_H
#define GS_TENSOR_H

#include "tensor.hh"
#include "fs_tensor.hh"
#include "symmetry.hh"
#include "rfs_tensor.hh"

class FGSTensor;
class UGSTensor;
class FSSparseTensor;

/* This class encapsulates symmetry information for the general
   symmetry tensor. It maintains a vector of variable numbers |nvs|, and
   symmetry |sym|. For example, let the symmetry be $y^2u^3$, and
   variable numbers be 10 for $y$, and 5 for $u$. Then the |nvs| is
   $(10,5)$, and |sym| is $(2,3)$. Also it maintains |nvmax| unfolded |nvs| with
   respect to the symmetry, this is $(10,10,5,5,5)$.

   The constructors of |TensorDimens| are clear and pretty intuitive but
   the constructor which is used for slicing fully symmetric tensor. It
   constructs the dimensions from the partitioning of variables of fully
   symmetric tensor. Let the partitioning be, for instance, $(a,b,c,d)$,
   where $(n_a,n_b,n_c,n_d)$ are lengths of the partitions. Let one want
   to get a slice only of the part of the fully symmetric tensor
   corresponding to indices of the form $b^2d^3$. This corresponds to the
   symmetry $a^0b^2c^0d^3$. So, the dimension of the slice would be also
   $(n_a,n_b,n_c,n_d)$ for number of variables and $(0,2,0,3)$ for the
   symmetry. So we provide the constructor which takes sizes of
   partitions $(n_a,n_b,n_c,n_d)$ as |IntSequence|, and indices of picked
   partitions, in our case $(1,1,3,3,3)$, as |IntSequence|.

   The class is able to calculate number of offsets (columns or rows depending
   what matrix coordinate we describe) in unfolded and folded tensors
   with the given symmetry. */

class TensorDimens
{
protected:
  IntSequence nvs;
  Symmetry sym;
  IntSequence nvmax;
public:
  TensorDimens(const Symmetry &s, const IntSequence &nvars)
    : nvs(nvars), sym(s), nvmax(sym, nvs)
  {
  }
  TensorDimens(int nvar, int dimen)
    : nvs(1), sym{dimen}, nvmax(dimen, nvar)
  {
    nvs[0] = nvar;
  }
  TensorDimens(const TensorDimens &td)
     
  = default;
  virtual ~TensorDimens()
  = default;
  TensorDimens(const IntSequence &ss, const IntSequence &coor);
  TensorDimens &
  operator=(const TensorDimens &td)
  = default;
  bool
  operator==(const TensorDimens &td) const
  {
    return nvs == td.nvs && sym == td.sym;
  }
  bool
  operator!=(const TensorDimens &td) const
  {
    return !operator==(td);
  }

  int
  dimen() const
  {
    return sym.dimen();
  }
  int
  getNVX(int i) const
  {
    return nvmax[i];
  }
  const IntSequence &
  getNVS() const
  {
    return nvs;
  }
  const IntSequence &
  getNVX() const
  {
    return nvmax;
  }
  const Symmetry &
  getSym() const
  {
    return sym;
  }

  int calcUnfoldMaxOffset() const;
  int calcFoldMaxOffset() const;
  int calcFoldOffset(const IntSequence &v) const;
  void decrement(IntSequence &v) const;
};

/* Here is a class for folded general symmetry tensor. It only contains
   tensor dimensions, it defines types for indices, implement virtual
   methods of super class |FTensor|.

   We add a method |contractAndAdd| which performs a contraction of one
   variable in the tensor. This is, for instance
   $$\left[r_{x^iz^k}\right]_{\alpha_1\ldots\alpha_i\gamma_1\ldots\gamma_k}=
   \left[t_{x^iy^jz^k}\right]_{\alpha_1\ldots\alpha_i\beta_1\ldots\beta_j\gamma_1\ldots\gamma_k}
   \left[c\right]^{\beta_1\ldots\beta_j}
   $$

   Also we add |getOffset| which should be used with care. */

class GSSparseTensor;
class FGSTensor : public FTensor
{
  friend class UGSTensor;

  const TensorDimens tdims;
public:
  /* These are standard constructors followed by two slicing. The first
     constructs a slice from the sparse, the second from the dense (both
     fully symmetric). Next constructor is just a conversion from
     |GSSParseTensor|. The last constructor allows for in-place conversion
     from |FFSTensor| to |FGSTensor|. */

  FGSTensor(int r, const TensorDimens &td)
    : FTensor(along_col, td.getNVX(), r,
              td.calcFoldMaxOffset(), td.dimen()), tdims(td)
  {
  }
  FGSTensor(const FGSTensor &ft)
     
  = default;
  FGSTensor(const UGSTensor &ut);
  FGSTensor(int first_row, int num, FGSTensor &t)
    : FTensor(first_row, num, t), tdims(t.tdims)
  {
  }
  FGSTensor(const FSSparseTensor &t, const IntSequence &ss,
            const IntSequence &coor, const TensorDimens &td);
  FGSTensor(const FFSTensor &t, const IntSequence &ss,
            const IntSequence &coor, const TensorDimens &td);
  FGSTensor(const GSSparseTensor &sp);
  FGSTensor(FFSTensor &t)
    : FTensor(0, t.nrows(), t), tdims(t.nvar(), t.dimen())
  {
  }

  ~FGSTensor()
  override = default;

  void increment(IntSequence &v) const override;
  void
  decrement(IntSequence &v) const override
  {
    tdims.decrement(v);
  }
  UTensor&unfold() const override;
  const TensorDimens &
  getDims() const
  {
    return tdims;
  }
  const Symmetry &
  getSym() const
  {
    return getDims().getSym();
  }

  void contractAndAdd(int i, FGSTensor &out,
                      const FRSingleTensor &col) const;
  int
  getOffset(const IntSequence &v) const override
  {
    return tdims.calcFoldOffset(v);
  }
};

/* Besides similar things that has |FGSTensor|, we have here also
   method |unfoldData|, and helper method |getFirstIndexOf|
   which corresponds to sorting coordinates in fully symmetric case (here
   the action is more complicated, so we put it to the method). */

class UGSTensor : public UTensor
{
  friend class FGSTensor;

  const TensorDimens tdims;
public:
  /* These are standard constructors. The last two constructors are
     slicing. The first makes a slice from fully symmetric sparse, the
     second from fully symmetric dense unfolded tensor. The last
     constructor allows for in-place conversion from |UFSTensor| to
     |UGSTensor|. */
  UGSTensor(int r, const TensorDimens &td)
    : UTensor(along_col, td.getNVX(), r,
              td.calcUnfoldMaxOffset(), td.dimen()), tdims(td)
  {
  }
  UGSTensor(const UGSTensor &ut)
     
  = default;
  UGSTensor(const FGSTensor &ft);

  UGSTensor(int first_row, int num, UGSTensor &t)
    : UTensor(first_row,  num, t), tdims(t.tdims)
  {
  }
  UGSTensor(const FSSparseTensor &t, const IntSequence &ss,
            const IntSequence &coor, const TensorDimens &td);
  UGSTensor(const UFSTensor &t, const IntSequence &ss,
            const IntSequence &coor, const TensorDimens &td);
  UGSTensor(UFSTensor &t)
    : UTensor(0, t.nrows(), t), tdims(t.nvar(), t.dimen())
  {
  }
  ~UGSTensor()
  override = default;

  void increment(IntSequence &v) const override;
  void decrement(IntSequence &v) const override;
  FTensor&fold() const override;
  const TensorDimens &
  getDims() const
  {
    return tdims;
  }
  const Symmetry &
  getSym() const
  {
    return getDims().getSym();
  }

  void contractAndAdd(int i, UGSTensor &out,
                      const URSingleTensor &col) const;
  int getOffset(const IntSequence &v) const override;
private:
  void unfoldData();
public:
  index getFirstIndexOf(const index &in) const;
};

#endif
