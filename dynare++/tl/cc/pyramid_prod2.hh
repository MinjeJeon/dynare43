// Copyright 2004, Ondra Kamenik

// Multiplying stacked tensor columns.

/* We need to calculate the following tensor product:
   $$\left[f_{s^j}\right]_{\alpha_1\ldots\alpha_j}=
   \sum_{l=1}^j\left[f_{z^l}\right]_{\beta_1\ldots\beta_l}
   \sum_{c\in M_{l,j}}\prod_{m=1}^l\left[z_{c_m}\right]^{\beta_m}_{c_m(\alpha)}
   $$
   where $s=[y,u,u',\sigma]$, and $z$ is a composition of four variables,
   say $[v,w,y,u]$. Note that $z$ ends with $y$ and $u$, and the only
   non-zero derivative of the trailing part of $z$ involving $y$ or $u$
   is the first derivative and is the unit matrix $y_y=[1]$ or
   $u_u=[1]$. Also, we suppose that the dependence of $v$, and $w$ on $s$
   is such that whenever derivative of $w$ is nonzero, then also of
   $v$. This means that there for any derivative and any index there is a
   continuous part of derivatives of $v$ and optionally of $w$ followed by
   column of zeros containing at most one $1$.

   This structure can be modelled and exploited with some costs at
   programming. For example, let us consider the following product:
   $$\left[B_{y^2u^3}\right]_{\alpha_1\alpha_2\beta_1\beta_2\beta_3}=
   \ldots
   \left[f_{z^3}\right]_{\gamma_1\gamma_2\gamma_3}
   \left[z_{yu}\right]^{\gamma_1}_{\alpha_1\beta_1}
   \left[z_{y}\right]^{\gamma_2}_{\alpha_2}
   \left[z_{uu}\right]^{\gamma_3}_{\beta_2\beta_3}
   \ldots$$
   The term corresponds to equivalence $\{\{0,2\},\{1\},\{3,4\}\}$. For
   the fixed index $\alpha_1\alpha_2\beta_1\beta_2\beta_3$ we have to
   make a Kronecker product of the columns
   $$
   \left[z_{yu}\right]_{\alpha_1\beta_1}\otimes
   \left[z_{y}\right]_{\alpha_2}\otimes
   \left[z_{uu}\right]_{\beta_2\beta_3}
   $$
   which can be written as
   $$
   \left[\matrix{\left[v_{yu}\right]_{\alpha_1\beta_1}\cr
   \left[w_{yu}\right]_{\alpha_1\beta_1}\cr 0\cr 0}\right]\otimes
   \left[\matrix{\left[v_y\right]_{\alpha_2\vphantom{(}}\cr
   \left[w_y\right]_{\alpha_2}\cr 1_{\alpha_2}\cr 0}\right]\otimes
   \left[\matrix{\left[v_{uu}\right]_{\beta_2\beta_3\vphantom{(}}\cr
   \left[w_{uu}\right]_{\beta_2\beta_3}\cr 0\cr 0}\right]
   $$
   where $1_{\alpha_2}$ is a column of zeros having the only $1$ at
   $\alpha_2$ index.

   This file develops the abstraction for this Kronecker product column
   without multiplication of the zeros at the top. Basically, it will be
   a column which is a Kronecker product of the columns without the
   zeros:
   $$
   \left[\matrix{\left[v_{yu}\right]_{\alpha_1\beta_1}\cr
   \left[w_{yu}\right]_{\alpha_1\beta_1}}\right]\otimes
   \left[\matrix{\left[v_y\right]_{\alpha_2}\cr
   \left[w_y\right]_{\alpha_2}\cr 1}\right]\otimes
   \left[\matrix{\left[v_{uu}\right]_{\beta_2\beta_3}\cr
   \left[w_{uu}\right]_{\beta_2\beta_3}}\right]
   $$
   The class will have a tensor infrastructure introducing |index| which
   iterates over all items in the column with $\gamma_1\gamma_2\gamma_3$
   as coordinates in $\left[f_{z^3}\right]$. The data of such a tensor is
   not suitable for any matrix operation and will have to be accessed
   only through the |index|. Note that this does not matter, since
   $\left[f_{z^l}\right]$ are sparse. */

#ifndef PYRAMID_PROD2_H
#define PYRAMID_PROD2_H

#include "permutation.hh"
#include "tensor.hh"
#include "tl_exception.hh"
#include "rfs_tensor.hh"
#include "stack_container.hh"

#include "Vector.hh"

/* First we declare a helper class for the tensor. Its purpose is to
   gather the columns which are going to be Kronecker multiplied. The
   input of this helper class is |StackProduct<FGSTensor>| and coordinate
   |c| of the column.

   It maintains |unit_flag| array which says for what columns we must
   stack 1 below $v$ and $w$. In this case, the value of |unit_flag| is
   an index of the $1$, otherwise the value of |unit_flag| is -1.

   Also we have storage for the stacked columns |cols|. The object is
   responsible for memory management associated to this storage. That is
   why we do not allow any copy constructor, since we need to be sure
   that no accidental copies take place. We declare the copy constructor
   as private and not implement it. */

class IrregTensor;
class IrregTensorHeader
{
  friend class IrregTensor;
  int nv;
  IntSequence unit_flag;
  Vector **const cols;
  IntSequence end_seq;
public:
  IrregTensorHeader(const StackProduct<FGSTensor> &sp, const IntSequence &c);
  IrregTensorHeader(const IrregTensorHeader &) = delete;
  ~IrregTensorHeader();
  int
  dimen() const
  {
    return unit_flag.size();
  }
  void increment(IntSequence &v) const;
  int calcMaxOffset() const;
};

/* Here we declare the irregular tensor. There is no special logic
   here. We inherit from |Tensor| and we must implement three methods,
   |increment|, |decrement| and |getOffset|. The last two are not
   implemented now, since they are not needed, and they raise an
   exception. The first just calls |increment| of the header. Also we
   declare a method |addTo| which adds this unfolded irregular single
   column tensor to folded (regular) single column tensor.

   The header |IrregTensorHeader| lives with an object by a
   reference. This is dangerous. However, we will use this class only in
   a simple loop and both |IrregTensor| and |IrregTensorHeader| will be
   destructed at the end of a block. Since the super class |Tensor| must
   be initialized before any member, we could do either a save copy of
   |IrregTensorHeader|, or relatively dangerous the reference member. For
   the reason above we chose the latter. */

class IrregTensor : public Tensor
{
  const IrregTensorHeader &header;
public:
  IrregTensor(const IrregTensorHeader &h);
  void addTo(FRSingleTensor &out) const;
  void
  increment(IntSequence &v) const
  {
    header.increment(v);
  }
  void
  decrement(IntSequence &v) const
  {
    TL_RAISE("Not implemented error in IrregTensor::decrement");
  }
  int
  getOffset(const IntSequence &v) const
  {
    TL_RAISE("Not implemented error in IrregTensor::getOffset"); return 0;
  }
};

#endif
