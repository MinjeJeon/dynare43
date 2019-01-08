// Copyright 2004, Ondra Kamenik

// Multiplying tensor columns.

/* In here, we implement the Faa Di Bruno for folded
   tensors. Recall, that one step of the Faa Di Bruno is a formula:
   $$\left[B_{s^k}\right]_{\alpha_1\ldots\alpha_k}=
   [h_{y^l}]_{\gamma_1\ldots\gamma_l}
   \prod_{m=1}^l\left[g_{s^{\vert c_m\vert}}\right]^{\gamma_m}_{c_m(\alpha)}
   $$

   In contrast to unfolded implementation of |UGSContainer::multAndAdd|
   with help of |KronProdAll| and |UPSTensor|, we take a completely
   different strategy. We cannot afford full instantiation of
   $$\sum_{c\in M_{l,k}}
   \prod_{m=1}^l\left[g_{s^{\vert c_m\vert}}\right]^{\gamma_m}_{c_m(\alpha)}$$
   and therefore we do it per partes. We select some number of columns,
   for instance 10, calculate 10 continuous iterators of tensor $B$. Then we
   form unfolded tensor
   $$[G]_S^{\gamma_1\ldots\gamma_l}=\left[\sum_{c\in M_{l,k}}
   \prod_{m=1}^l\left[g_{s^{\vert c_m\vert}}\right]^{\gamma_m}_{c_m(\alpha)}
   \right]_S$$
   where $S$ is the selected set of 10 indices. This is done as Kronecker
   product of vectors corresponding to selected columns. Note that, in
   general, there is no symmetry in $G$, its type is special class for
   this purpose.

   If $g$ is folded, then we have to form folded version of $G$. There is
   no symmetry in $G$ data, so we sum all unfolded indices corresponding
   to folded index together. This is perfectly OK, since we multiply
   these groups of (equivalent) items with the same number in fully
   symmetric $g$.

   After this, we perform ordinary matrix multiplication to obtain a
   selected set of columns of $B$.

   In here, we define a class for forming and representing
   $[G]_S^{\gamma_1\ldots\gamma_l}$. Basically, this tensor is
   row-oriented (multidimensional index is along rows), and it is fully
   symmetric. So we inherit from |URTensor|. If we need its folded
   version, we simply use a suitable conversion. The new abstraction will
   have only a new constructor allowing a construction from the given set
   of indices $S$, and given set of tensors $g$. The rest of the process
   is implemented in |@<|FGSContainer::multAndAdd| unfolded code@>| or
   |@<|FGSContainer::multAndAdd| folded code@>|. */

#ifndef PYRAMID_PROD_H
#define PYRAMID_PROD_H

#include "int_sequence.hh"
#include "rfs_tensor.hh"
#include "gs_tensor.hh"
#include "t_container.hh"

#include <vector>

using namespace std;

/* Here we define the new tensor for representing
   $[G]_S^{\gamma_1\ldots\gamma_l}$. It allows a construction from
   container of folded general symmetry tensors |cont|, and set of
   indices |ts|. Also we have to supply dimensions of resulting tensor
   $B$, and dimensions of tensor $h$. */

class USubTensor : public URTensor
{
public:
  USubTensor(const TensorDimens &bdims, const TensorDimens &hdims,
             const FGSContainer &cont, const vector<IntSequence> &lst);
  void addKronColumn(int i, const vector<const FGSTensor *> &ts,
                     const IntSequence &pindex);
};

#endif
