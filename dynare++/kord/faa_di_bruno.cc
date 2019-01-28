// Copyright 2005, Ondra Kamenik

#include "faa_di_bruno.hh"
#include "fine_container.hh"

#include <cmath>

double FaaDiBruno::magic_mult = 1.5;

// |FaaDiBruno::calculate| folded sparse code
/* We take an opportunity to refine the stack container to avoid
   allocation of more memory than available. */
void
FaaDiBruno::calculate(const StackContainer<FGSTensor> &cont,
                      const TensorContainer<FSSparseTensor> &f,
                      FGSTensor &out)
{
  out.zeros();
  for (int l = 1; l <= out.dimen(); l++)
    {
      int mem_mb, p_size_mb;
      int max = estimRefinment(out.getDims(), out.nrows(), l, mem_mb, p_size_mb);
      FoldedFineContainer fine_cont(cont, max);
      fine_cont.multAndAdd(l, f, out);
      JournalRecord recc(journal);
      recc << "dim=" << l << " avmem=" << mem_mb << " tmpmem=" << p_size_mb << " max=" << max
           << " stacks=" << cont.numStacks() << "->" << fine_cont.numStacks() << endrec;
    }
}

// |FaaDiBruno::calculate| folded dense code
/* Here we just simply evaluate |multAndAdd| for the dense
   container. There is no opportunity for tuning. */
void
FaaDiBruno::calculate(const FoldedStackContainer &cont, const FGSContainer &g,
                      FGSTensor &out)
{
  out.zeros();
  for (int l = 1; l <= out.dimen(); l++)
    {
      long int mem = SystemResources::availableMemory();
      cont.multAndAdd(l, g, out);
      JournalRecord rec(journal);
      int mem_mb = mem/1024/1024;
      rec << "dim=" << l << " avmem=" << mem_mb << endrec;
    }
}

// |FaaDiBruno::calculate| unfolded sparse code
/* This is the same as |@<|FaaDiBruno::calculate| folded sparse
   code@>|. The only difference is that we construct unfolded fine
   container. */
void
FaaDiBruno::calculate(const StackContainer<UGSTensor> &cont,
                      const TensorContainer<FSSparseTensor> &f,
                      UGSTensor &out)
{
  out.zeros();
  for (int l = 1; l <= out.dimen(); l++)
    {
      int mem_mb, p_size_mb;
      int max = estimRefinment(out.getDims(), out.nrows(), l, mem_mb, p_size_mb);
      UnfoldedFineContainer fine_cont(cont, max);
      fine_cont.multAndAdd(l, f, out);
      JournalRecord recc(journal);
      recc << "dim=" << l << " avmem=" << mem_mb << " tmpmem=" << p_size_mb << " max=" << max
           << " stacks=" << cont.numStacks() << "->" << fine_cont.numStacks() << endrec;
    }
}

// |FaaDiBruno::calculate| unfolded dense code
/* Again, no tuning opportunity here. */
void
FaaDiBruno::calculate(const UnfoldedStackContainer &cont, const UGSContainer &g,
                      UGSTensor &out)
{
  out.zeros();
  for (int l = 1; l <= out.dimen(); l++)
    {
      long int mem = SystemResources::availableMemory();
      cont.multAndAdd(l, g, out);
      JournalRecord rec(journal);
      int mem_mb = mem/1024/1024;
      rec << "dim=" << l << " avmem=" << mem_mb << endrec;
    }
}

/* This function returns a number of maximum rows used for refinement of
   the stacked container. We want to set the maximum so that the expected
   memory consumption for the number of paralel threads would be less
   than available memory. On the other hand we do not want to be too
   pesimistic since a very fine refinement can be very slow.

   Besides memory needed for a dense unfolded slice of a tensor from
   |f|, each thread needs |magic_mult*per_size| bytes of memory. In the
   worst case, |magic_mult| will be equal to 2, this means memory
   |per_size| for target temporary (permuted symmetry) tensor plus one
   copy for intermediate result. However, this shows to be too
   pesimistic, so we set |magic_mult| to 1.5. The memory for permuted
   symmetry temporary tensor |per_size| is estimated as a weigthed
   average of unfolded memory of the |out| tensor and unfolded memory of
   a symetric tensor with the largest coordinate size. Some experiments
   showed that the best combination of the two is to take 100\% if the
   latter, so we set |lambda| to zero.

   The |max| number of rows in the refined |cont| must be such that each
   slice fits to remaining memory. Number of columns of the slice are
   never greater $max^l$. (This is not true, since stacks corresponing to
   unit/zero matrices cannot be further refined). We get en equation:

   $$nthreads\cdot max^l\cdot 8\cdot r = mem -
   magic\_mult\cdot nthreads\cdot per\_size\cdot 8\cdot r,$$
   where |mem| is available memory in bytes, |nthreads| is a number of
   threads, $r$ is a number of rows, and $8$ is |sizeof(double)|.

   If the right hand side is less than zero, we set |max| to 10, just to
   let it do something. */

int
FaaDiBruno::estimRefinment(const TensorDimens &tdims, int nr, int l,
                           int &avmem_mb, int &tmpmem_mb)
{
  int nthreads = sthread::detach_thread_group::max_parallel_threads;
  long int per_size1 = tdims.calcUnfoldMaxOffset();
  auto per_size2 = (long int) pow((double) tdims.getNVS().getMax(), l);
  double lambda = 0.0;
  long int per_size = sizeof(double)*nr
    *(long int) (lambda*per_size1+(1-lambda)*per_size2);
  long int mem = SystemResources::availableMemory();
  int max = 0;
  double num_cols = ((double) (mem-magic_mult*nthreads*per_size))
    /nthreads/sizeof(double)/nr;
  if (num_cols > 0)
    {
      double maxd = pow(num_cols, ((double) 1)/l);
      max = (int) floor(maxd);
    }
  if (max == 0)
    {
      max = 10;
      JournalRecord rec(journal);
      rec << "dim=" << l << " run out of memory, imposing max=" << max;
      if (nthreads > 1)
        rec << " (decrease number of threads)";
      rec << endrec;
    }
  avmem_mb = mem/1024/1024;
  tmpmem_mb = (nthreads*per_size)/1024/1024;
  return max;
}
