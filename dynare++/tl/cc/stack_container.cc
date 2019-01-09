// Copyright 2004, Ondra Kamenik

#include "stack_container.hh"
#include "pyramid_prod2.hh"
#include "ps_tensor.hh"

double FoldedStackContainer::fill_threshold = 0.00005;
double UnfoldedStackContainer::fill_threshold = 0.00005;

// |FoldedStackContainer::multAndAdd| sparse code
/* Here we multiply the sparse tensor with the
   |FoldedStackContainer|. We have four implementations,
   |multAndAddSparse1|, |multAndAddSparse2|, |multAndAddSparse3|, and
   |multAndAddSparse4|.  The third is not threaded yet and I expect that
   it is certainly the slowest. The |multAndAddSparse4| exploits the
   sparsity, however, it seems to be still worse than |multAndAddSparse2|
   even for really sparse matrices. On the other hand, it can be more
   efficient than |multAndAddSparse2| for large problems, since it does
   not need that much of memory and can avoid much swapping. Very
   preliminary examination shows that |multAndAddSparse2| is the best in
   terms of time. */
void
FoldedStackContainer::multAndAdd(const FSSparseTensor &t,
                                 FGSTensor &out) const
{
  TL_RAISE_IF(t.nvar() != getAllSize(),
              "Wrong number of variables of tensor for FoldedStackContainer::multAndAdd");
  multAndAddSparse2(t, out);
}

// |FoldedStackContainer::multAndAdd| dense code
/* Here we perform the Faa Di Bruno step for a given dimension |dim|, and for
   the dense fully symmetric tensor which is scattered in the container
   of general symmetric tensors. The implementation is pretty the same as
   |@<|UnfoldedStackContainer::multAndAdd| dense code@>|. */
void
FoldedStackContainer::multAndAdd(int dim, const FGSContainer &c, FGSTensor &out) const
{
  TL_RAISE_IF(c.num() != numStacks(),
              "Wrong symmetry length of container for FoldedStackContainer::multAndAdd");

  THREAD_GROUP gr;
  SymmetrySet ss(dim, c.num());
  for (symiterator si(ss); !si.isEnd(); ++si)
    {
      if (c.check(*si))
        {
          THREAD *worker = new WorkerFoldMAADense(*this, *si, c, out);
          gr.insert(worker);
        }
    }
  gr.run();
}

/* This is analogous to |@<|WorkerUnfoldMAADense::operator()()|
   code@>|. */

void
WorkerFoldMAADense::operator()()
{
  Permutation iden(dense_cont.num());
  IntSequence coor(sym, iden.getMap());
  const FGSTensor *g = dense_cont.get(sym);
  cont.multAndAddStacks(coor, *g, out, &out);
}

WorkerFoldMAADense::WorkerFoldMAADense(const FoldedStackContainer &container,
                                       const Symmetry &s,
                                       const FGSContainer &dcontainer,
                                       FGSTensor &outten)
  : cont(container), sym(s), dense_cont(dcontainer), out(outten)
{
}

/* This is analogous to |@<|UnfoldedStackContainer::multAndAddSparse1|
   code@>|. */
void
FoldedStackContainer::multAndAddSparse1(const FSSparseTensor &t,
                                        FGSTensor &out) const
{
  THREAD_GROUP gr;
  UFSTensor dummy(0, numStacks(), t.dimen());
  for (Tensor::index ui = dummy.begin(); ui != dummy.end(); ++ui)
    {
      THREAD *worker = new WorkerFoldMAASparse1(*this, t, out, ui.getCoor());
      gr.insert(worker);
    }
  gr.run();
}

/* This is analogous to |@<|WorkerUnfoldMAASparse1::operator()()| code@>|.
   The only difference is that instead of |UPSTensor| as a
   result of multiplication of unfolded tensor and tensors from
   containers, we have |FPSTensor| with partially folded permuted
   symmetry.

   todo: make slice vertically narrowed according to the fill of t,
   vertically narrow out accordingly. */

void
WorkerFoldMAASparse1::operator()()
{
  const EquivalenceSet &eset = ebundle.get(out.dimen());
  const PermutationSet &pset = tls.pbundle->get(t.dimen());
  Permutation iden(t.dimen());

  UPSTensor slice(t, cont.getStackSizes(), coor,
                  PerTensorDimens(cont.getStackSizes(), coor));
  for (int iper = 0; iper < pset.getNum(); iper++)
    {
      const Permutation &per = pset.get(iper);
      IntSequence percoor(coor.size());
      per.apply(coor, percoor);
      for (const auto & it : eset)
        {
          if (it.numClasses() == t.dimen())
            {
              StackProduct<FGSTensor> sp(cont, it, out.getSym());
              if (!sp.isZero(percoor))
                {
                  KronProdStack<FGSTensor> kp(sp, percoor);
                  kp.optimizeOrder();
                  const Permutation &oper = kp.getPer();
                  if (Permutation(oper, per) == iden)
                    {
                      FPSTensor fps(out.getDims(), it, slice, kp);
                      {
                        SYNCHRO syn(&out, "WorkerUnfoldMAASparse1");
                        fps.addTo(out);
                      }
                    }
                }
            }
        }
    }
}

WorkerFoldMAASparse1::WorkerFoldMAASparse1(const FoldedStackContainer &container,
                                           const FSSparseTensor &ten,
                                           FGSTensor &outten, const IntSequence &c)
  : cont(container), t(ten), out(outten), coor(c), ebundle(*(tls.ebundle))
{
}

/* Here is the second implementation of sparse folded |multAndAdd|. It
   is pretty similar to implementation of
   |@<|UnfoldedStackContainer::multAndAddSparse2| code@>|. We make a
   dense folded |slice|, and then call folded |multAndAddStacks|, which
   multiplies all the combinations compatible with the slice. */

void
FoldedStackContainer::multAndAddSparse2(const FSSparseTensor &t,
                                        FGSTensor &out) const
{
  THREAD_GROUP gr;
  FFSTensor dummy_f(0, numStacks(), t.dimen());
  for (Tensor::index fi = dummy_f.begin(); fi != dummy_f.end(); ++fi)
    {
      THREAD *worker = new WorkerFoldMAASparse2(*this, t, out, fi.getCoor());
      gr.insert(worker);
    }
  gr.run();
}

/* Here we make a sparse slice first and then call |multAndAddStacks|
   if the slice is not empty. If the slice is really sparse, we call
   sparse version of |multAndAddStacks|. What means ``really sparse'' is
   given by |fill_threshold|. It is not tuned yet, a practice shows that
   it must be a really low number, since sparse |multAndAddStacks| is
   much slower than the dense version.

   Further, we take only nonzero rows of the slice, and accordingly of
   the out tensor. We jump over zero initial rows and drop zero tailing
   rows. */

void
WorkerFoldMAASparse2::operator()()
{
  GSSparseTensor slice(t, cont.getStackSizes(), coor,
                       TensorDimens(cont.getStackSizes(), coor));
  if (slice.getNumNonZero())
    {
      if (slice.getUnfoldIndexFillFactor() > FoldedStackContainer::fill_threshold)
        {
          FGSTensor dense_slice(slice);
          int r1 = slice.getFirstNonZeroRow();
          int r2 = slice.getLastNonZeroRow();
          FGSTensor dense_slice1(r1, r2-r1+1, dense_slice);
          FGSTensor out1(r1, r2-r1+1, out);
          cont.multAndAddStacks(coor, dense_slice1, out1, &out);
        }
      else
        cont.multAndAddStacks(coor, slice, out, &out);
    }
}

WorkerFoldMAASparse2::WorkerFoldMAASparse2(const FoldedStackContainer &container,
                                           const FSSparseTensor &ten,
                                           FGSTensor &outten, const IntSequence &c)
  : cont(container), t(ten), out(outten), coor(c)
{
}

/* Here is the third implementation of the sparse folded
   |multAndAdd|. It is column-wise implementation, and thus is not a good
   candidate for the best performer.

   We go through all columns from the output. For each column we
   calculate folded |sumcol| which is a sum of all appropriate columns
   for all suitable equivalences. So we go through all suitable
   equivalences, for each we construct a |StackProduct| object and
   construct |IrregTensor| for a corresponding column of $z$. The
   |IrregTensor| is an abstraction for Kronecker multiplication of
   stacked columns of the two containers without zeros. Then the column
   is added to |sumcol|. Finally, the |sumcol| is multiplied by the
   sparse tensor. */

void
FoldedStackContainer::multAndAddSparse3(const FSSparseTensor &t,
                                        FGSTensor &out) const
{
  const EquivalenceSet &eset = ebundle.get(out.dimen());
  for (Tensor::index run = out.begin(); run != out.end(); ++run)
    {
      Vector outcol(out, *run);
      FRSingleTensor sumcol(t.nvar(), t.dimen());
      sumcol.zeros();
      for (const auto & it : eset)
        {
          if (it.numClasses() == t.dimen())
            {
              StackProduct<FGSTensor> sp(*this, it, out.getSym());
              IrregTensorHeader header(sp, run.getCoor());
              IrregTensor irten(header);
              irten.addTo(sumcol);
            }
        }
      t.multColumnAndAdd(sumcol, outcol);
    }
}

/* Here is the fourth implementation of sparse
   |FoldedStackContainer::multAndAdd|. It is almost equivalent to
   |multAndAddSparse2| with the exception that the |FPSTensor| as a
   result of a product of a slice and Kronecker product of the stack
   derivatives is calculated in the sparse fashion. For further details, see
   |@<|FoldedStackContainer::multAndAddStacks| sparse code@>| and
   |@<|FPSTensor| sparse constructor@>|. */

void
FoldedStackContainer::multAndAddSparse4(const FSSparseTensor &t, FGSTensor &out) const
{
  THREAD_GROUP gr;
  FFSTensor dummy_f(0, numStacks(), t.dimen());
  for (Tensor::index fi = dummy_f.begin(); fi != dummy_f.end(); ++fi)
    {
      THREAD *worker = new WorkerFoldMAASparse4(*this, t, out, fi.getCoor());
      gr.insert(worker);
    }
  gr.run();
}

/* The |WorkerFoldMAASparse4| is the same as |WorkerFoldMAASparse2|
   with the exception that we call a sparse version of
   |multAndAddStacks|. */

void
WorkerFoldMAASparse4::operator()()
{
  GSSparseTensor slice(t, cont.getStackSizes(), coor,
                       TensorDimens(cont.getStackSizes(), coor));
  if (slice.getNumNonZero())
    cont.multAndAddStacks(coor, slice, out, &out);
}

WorkerFoldMAASparse4::WorkerFoldMAASparse4(const FoldedStackContainer &container,
                                           const FSSparseTensor &ten,
                                           FGSTensor &outten, const IntSequence &c)
  : cont(container), t(ten), out(outten), coor(c)
{
}

// |FoldedStackContainer::multAndAddStacks| dense code
/* This is almost the same as
   |@<|UnfoldedStackContainer::multAndAddStacks| code@>|. The only
   difference is that we do not construct a |UPSTensor| from
   |KronProdStack|, but we construct partially folded permuted
   symmetry |FPSTensor|. Note that the tensor |g| must be unfolded
   in order to be able to multiply with unfolded rows of Kronecker
   product. However, columns of such a product are partially
   folded giving a rise to the |FPSTensor|. */
void
FoldedStackContainer::multAndAddStacks(const IntSequence &coor,
                                       const FGSTensor &g,
                                       FGSTensor &out, const void *ad) const
{
  const EquivalenceSet &eset = ebundle.get(out.dimen());

  UGSTensor ug(g);
  UFSTensor dummy_u(0, numStacks(), g.dimen());
  for (Tensor::index ui = dummy_u.begin(); ui != dummy_u.end(); ++ui)
    {
      IntSequence tmp(ui.getCoor());
      tmp.sort();
      if (tmp == coor)
        {
          Permutation sort_per(ui.getCoor());
          sort_per.inverse();
          for (const auto & it : eset)
            {
              if (it.numClasses() == g.dimen())
                {
                  StackProduct<FGSTensor> sp(*this, it, sort_per, out.getSym());
                  if (!sp.isZero(coor))
                    {
                      KronProdStack<FGSTensor> kp(sp, coor);
                      if (ug.getSym().isFull())
                        kp.optimizeOrder();
                      FPSTensor fps(out.getDims(), it, sort_per, ug, kp);
                      {
                        SYNCHRO syn(ad, "multAndAddStacks");
                        fps.addTo(out);
                      }
                    }
                }
            }
        }
    }
}

// |FoldedStackContainer::multAndAddStacks| sparse code
/* This is almost the same as
   |@<|FoldedStackContainer::multAndAddStacks| dense code@>|. The only
   difference is that the Kronecker product of the stacks is multiplied
   with sparse slice |GSSparseTensor| (not dense slice |FGSTensor|). The
   multiplication is done in |@<|FPSTensor| sparse constructor@>|. */
void
FoldedStackContainer::multAndAddStacks(const IntSequence &coor,
                                       const GSSparseTensor &g,
                                       FGSTensor &out, const void *ad) const
{
  const EquivalenceSet &eset = ebundle.get(out.dimen());
  UFSTensor dummy_u(0, numStacks(), g.dimen());
  for (Tensor::index ui = dummy_u.begin(); ui != dummy_u.end(); ++ui)
    {
      IntSequence tmp(ui.getCoor());
      tmp.sort();
      if (tmp == coor)
        {
          Permutation sort_per(ui.getCoor());
          sort_per.inverse();
          for (const auto & it : eset)
            {
              if (it.numClasses() == g.dimen())
                {
                  StackProduct<FGSTensor> sp(*this, it, sort_per, out.getSym());
                  if (!sp.isZero(coor))
                    {
                      KronProdStack<FGSTensor> kp(sp, coor);
                      FPSTensor fps(out.getDims(), it, sort_per, g, kp);
                      {
                        SYNCHRO syn(ad, "multAndAddStacks");
                        fps.addTo(out);
                      }
                    }
                }
            }
        }
    }
}

// |UnfoldedStackContainer::multAndAdd| sparse code
/*  Here we simply call either |multAndAddSparse1| or
    |multAndAddSparse2|. The first one allows for optimization of
    Kronecker products, so it seems to be more efficient. */
void
UnfoldedStackContainer::multAndAdd(const FSSparseTensor &t,
                                   UGSTensor &out) const
{
  TL_RAISE_IF(t.nvar() != getAllSize(),
              "Wrong number of variables of tensor for UnfoldedStackContainer::multAndAdd");
  multAndAddSparse2(t, out);
}

// |UnfoldedStackContainer::multAndAdd| dense code
/* Here we implement the formula for stacks for fully symmetric tensor
   scattered in a number of general symmetry tensors contained in a given
   container. The implementations is pretty the same as in
   |multAndAddSparse2| but we do not do the slices of sparse tensor, but
   only a lookup to the container.

   This means that we do not iterate through a dummy folded tensor to
   obtain folded coordinates of stacks, rather we iterate through all
   symmetries contained in the container and the coordinates of stacks
   are obtained as unfolded identity sequence via the symmetry. The
   reason of doing this is that we are unable to calculate symmetry from
   stack coordinates as easily as stack coordinates from the symmetry. */
void
UnfoldedStackContainer::multAndAdd(int dim, const UGSContainer &c,
                                   UGSTensor &out) const
{
  TL_RAISE_IF(c.num() != numStacks(),
              "Wrong symmetry length of container for UnfoldedStackContainer::multAndAdd");

  THREAD_GROUP gr;
  SymmetrySet ss(dim, c.num());
  for (symiterator si(ss); !si.isEnd(); ++si)
    {
      if (c.check(*si))
        {
          THREAD *worker = new WorkerUnfoldMAADense(*this, *si, c, out);
          gr.insert(worker);
        }
    }
  gr.run();
}

void
WorkerUnfoldMAADense::operator()()
{
  Permutation iden(dense_cont.num());
  IntSequence coor(sym, iden.getMap());
  const UGSTensor *g = dense_cont.get(sym);
  cont.multAndAddStacks(coor, *g, out, &out);
}

WorkerUnfoldMAADense::WorkerUnfoldMAADense(const UnfoldedStackContainer &container,
                                           const Symmetry &s,
                                           const UGSContainer &dcontainer,
                                           UGSTensor &outten)
  : cont(container), sym(s), dense_cont(dcontainer), out(outten)
{
}

/* Here we implement the formula for unfolded tensors. If, for instance,
   a coordinate $z$ of a tensor $\left[f_{z^2}\right]$ is partitioned as
   $z=[a, b]$, then we perform the following:
   $$
   \eqalign{
   \left[f_{z^2}\right]\left(\sum_c\left[\matrix{a_{c(x)}\cr b_{c(y)}}\right]
   \otimes\left[\matrix{a_{c(y)}\cr b_{c(y)}}\right]\right)=&
   \left[f_{aa}\right]\left(\sum_ca_{c(x)}\otimes a_{c(y)}\right)+
   \left[f_{ab}\right]\left(\sum_ca_{c(x)}\otimes b_{c(y)}\right)+\cr
   &\left[f_{ba}\right]\left(\sum_cb_{c(x)}\otimes a_{c(y)}\right)+
   \left[f_{bb}\right]\left(\sum_cb_{c(x)}\otimes b_{c(y)}\right)\cr
   }
   $$
   This is exactly what happens here. The code is clear. It goes through
   all combinations of stacks, and each thread is responsible for
   operation for the slice corresponding to the combination of the stacks. */

void
UnfoldedStackContainer::multAndAddSparse1(const FSSparseTensor &t,
                                          UGSTensor &out) const
{
  THREAD_GROUP gr;
  UFSTensor dummy(0, numStacks(), t.dimen());
  for (Tensor::index ui = dummy.begin(); ui != dummy.end(); ++ui)
    {
      THREAD *worker = new WorkerUnfoldMAASparse1(*this, t, out, ui.getCoor());
      gr.insert(worker);
    }
  gr.run();
}

/* This does a step of |@<|UnfoldedStackContainer::multAndAddSparse1| code@>| for
   a given coordinates. First it makes the slice of the given stack coordinates.
   Then it multiplies everything what should be multiplied with the slice.
   That is it goes through all equivalences, creates |StackProduct|, then
   |KronProdStack|, which is added to |out|. So far everything is clear.

   However, we want to use optimized |KronProdAllOptim| to minimize
   a number of flops and memory needed in the Kronecker product. So we go
   through all permutations |per|, permute the coordinates to get
   |percoor|, go through all equivalences, and make |KronProdStack| and
   optimize it. The result of optimization is a permutation |oper|. Now,
   we multiply the Kronecker product with the slice, only if the slice
   has the same ordering of coordinates as the Kronecker product
   |KronProdStack|. However, it is not perfectly true. Since we go
   through {\bf all} permutations |per|, there might be two different
   permutations leading to the same ordering in |KronProdStack| and thus
   the same ordering in the optimized |KronProdStack|. The two cases
   would be counted twice, which is wrong. That is why we do not
   condition on $\hbox{coor}\circ\hbox{oper}\circ\hbox{per} =
   \hbox{coor}$, but we condition on
   $\hbox{oper}\circ\hbox{per}=\hbox{id}$. In this way, we rule out
   permutations |per| leading to the same ordering of stacks when
   applied on |coor|.

   todo: vertically narrow slice and out according to the fill in t. */

void
WorkerUnfoldMAASparse1::operator()()
{
  const EquivalenceSet &eset = ebundle.get(out.dimen());
  const PermutationSet &pset = tls.pbundle->get(t.dimen());
  Permutation iden(t.dimen());

  UPSTensor slice(t, cont.getStackSizes(), coor,
                  PerTensorDimens(cont.getStackSizes(), coor));
  for (int iper = 0; iper < pset.getNum(); iper++)
    {
      const Permutation &per = pset.get(iper);
      IntSequence percoor(coor.size());
      per.apply(coor, percoor);
      for (const auto & it : eset)
        {
          if (it.numClasses() == t.dimen())
            {
              StackProduct<UGSTensor> sp(cont, it, out.getSym());
              if (!sp.isZero(percoor))
                {
                  KronProdStack<UGSTensor> kp(sp, percoor);
                  kp.optimizeOrder();
                  const Permutation &oper = kp.getPer();
                  if (Permutation(oper, per) == iden)
                    {
                      UPSTensor ups(out.getDims(), it, slice, kp);
                      {
                        SYNCHRO syn(&out, "WorkerUnfoldMAASparse1");
                        ups.addTo(out);
                      }
                    }
                }
            }
        }
    }
}

WorkerUnfoldMAASparse1::WorkerUnfoldMAASparse1(const UnfoldedStackContainer &container,
                                               const FSSparseTensor &ten,
                                               UGSTensor &outten, const IntSequence &c)
  : cont(container), t(ten), out(outten), coor(c), ebundle(*(tls.ebundle))
{
}

/* In here we implement the formula by a bit different way. We use the
   fact, using notation of |@<|UnfoldedStackContainer::multAndAddSparse2|
   code@>|, that
   $$
   \left[f_{ba}\right]\left(\sum_cb_{c(x)}\otimes a_{c(y)}\right)=
   \left[f_{ab}\right]\left(\sum_ca_{c(y)}\otimes b_{c(b)}\right)\cdot P
   $$
   where $P$ is a suitable permutation of columns. The permutation
   corresponds to (in this example) a swap of $a$ and $b$. An advantage
   of this approach is that we do not need |UPSTensor| for $f_{ba}$, and
   thus we decrease the number of needed slices.

   So we go through all folded indices of stack coordinates, then for
   each such index |fi| we make a slice and call |multAndAddStacks|. This
   goes through all corresponding unfolded indices to perform the
   formula. Each unsorted (unfold) index implies a sorting permutation
   |sort_per| which must be used to permute stacks in |StackProduct|, and
   permute equivalence classes when |UPSTensor| is formed. In this way
   the column permutation $P$ from the formula is factored to the
   permutation of |UPSTensor|. */

void
UnfoldedStackContainer::multAndAddSparse2(const FSSparseTensor &t,
                                          UGSTensor &out) const
{
  THREAD_GROUP gr;
  FFSTensor dummy_f(0, numStacks(), t.dimen());
  for (Tensor::index fi = dummy_f.begin(); fi != dummy_f.end(); ++fi)
    {
      THREAD *worker = new WorkerUnfoldMAASparse2(*this, t, out, fi.getCoor());
      gr.insert(worker);
    }
  gr.run();
}

/* This does a step of |@<|UnfoldedStackContainer::multAndAddSparse2| code@>| for
   a given coordinates.

   todo: implement |multAndAddStacks| for sparse slice as
   |@<|FoldedStackContainer::multAndAddStacks| sparse code@>| and do this method as
   |@<|WorkerFoldMAASparse2::operator()()| code@>|. */

void
WorkerUnfoldMAASparse2::operator()()
{
  GSSparseTensor slice(t, cont.getStackSizes(), coor,
                       TensorDimens(cont.getStackSizes(), coor));
  if (slice.getNumNonZero())
    {
      FGSTensor fslice(slice);
      UGSTensor dense_slice(fslice);
      int r1 = slice.getFirstNonZeroRow();
      int r2 = slice.getLastNonZeroRow();
      UGSTensor dense_slice1(r1, r2-r1+1, dense_slice);
      UGSTensor out1(r1, r2-r1+1, out);

      cont.multAndAddStacks(coor, dense_slice1, out1, &out);
    }
}

WorkerUnfoldMAASparse2::WorkerUnfoldMAASparse2(const UnfoldedStackContainer &container,
                                               const FSSparseTensor &ten,
                                               UGSTensor &outten, const IntSequence &c)
  : cont(container), t(ten), out(outten), coor(c)
{
}

/* For a given unfolded coordinates of stacks |fi|, and appropriate
   tensor $g$, whose symmetry is a symmetry of |fi|, the method
   contributes to |out| all tensors in unfolded stack formula involving
   stacks chosen by |fi|.

   We go through all |ui| coordinates which yield |fi| after sorting. We
   construct a permutation |sort_per| which sorts |ui| to |fi|. We go
   through all appropriate equivalences, and construct |StackProduct|
   from equivalence classes permuted by |sort_per|, then |UPSTensor| with
   implied permutation of columns by the permuted equivalence by
   |sort_per|. The |UPSTensor| is then added to |out|.

   We cannot use here the optimized |KronProdStack|, since the symmetry
   of |UGSTensor& g| prescribes the ordering of the stacks. However, if
   |g| is fully symmetric, we can do the optimization harmlessly. */

void
UnfoldedStackContainer::multAndAddStacks(const IntSequence &fi,
                                         const UGSTensor &g,
                                         UGSTensor &out, const void *ad) const
{
  const EquivalenceSet &eset = ebundle.get(out.dimen());

  UFSTensor dummy_u(0, numStacks(), g.dimen());
  for (Tensor::index ui = dummy_u.begin(); ui != dummy_u.end(); ++ui)
    {
      IntSequence tmp(ui.getCoor());
      tmp.sort();
      if (tmp == fi)
        {
          Permutation sort_per(ui.getCoor());
          sort_per.inverse();
          for (const auto & it : eset)
            {
              if (it.numClasses() == g.dimen())
                {
                  StackProduct<UGSTensor> sp(*this, it, sort_per, out.getSym());
                  if (!sp.isZero(fi))
                    {
                      KronProdStack<UGSTensor> kp(sp, fi);
                      if (g.getSym().isFull())
                        kp.optimizeOrder();
                      UPSTensor ups(out.getDims(), it, sort_per, g, kp);
                      {
                        SYNCHRO syn(ad, "multAndAddStacks");
                        ups.addTo(out);
                      }
                    }
                }
            }
        }
    }
}
