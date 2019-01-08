// Copyright 2005, Ondra Kamenik

// Refined stack of containers.

/* This file defines a refinement of the stack container. It makes a
   vertical refinement of a given stack container, it refines only matrix
   items, the items which are always zero, or can be identity matrices
   are not refined.

   The refinement is done by a simple construction from the stack
   container being refined. A parameter is passed meaning a maximum size
   of each stack in the refined container. The resulting object is stack
   container, so everything works seamlessly.

   We define here a class for refinement of sizes |SizeRefinement|, this
   is purely an auxiliary class allowing us to write a code more
   concisely. The main class of this file is |FineContainer|, which
   corresponds to refining. The two more classes |FoldedFineContainer|
   and |UnfoldedFineContainer| are its specializations.

   NOTE: This code was implemented with a hope that it will help to cut
   down memory allocations during the Faa Di Bruno formula
   evaluation. However, it seems that this needs to be accompanied with a
   similar thing for tensor multidimensional index. Thus, the abstraction
   is not currently used, but it might be useful in future. */

#ifndef FINE_CONTAINER_H
#define FINE_CONTAINER_H

#include "stack_container.hh"

#include <vector>

/* This class splits the first |nc| elements of the given sequence |s|
   to a sequence not having items greater than given |max|. The remaining
   elements (those behind |nc|) are left untouched. It also remembers the
   mapping, i.e. for a given index in a new sequence, it is able to
   return a corresponding index in old sequence. */

class SizeRefinement
{
  vector<int> rsizes;
  vector<int> ind_map;
  int new_nc;
public:
  SizeRefinement(const IntSequence &s, int nc, int max);
  int
  getRefSize(int i) const
  {
    return rsizes[i];
  }
  int
  numRefinements() const
  {
    return rsizes.size();
  }
  int
  getOldIndex(int i) const
  {
    return ind_map[i];
  }
  int
  getNC() const
  {
    return new_nc;
  }
};

/* This main class of this class refines a given stack container, and
   inherits from the stack container. It also defines the |getType|
   method, which returns a type for a given stack as the type of the
   corresponding (old) stack of the former stack container. */

template <class _Ttype>
class FineContainer : public SizeRefinement, public StackContainer<_Ttype>
{
protected:
  typedef StackContainer<_Ttype> _Stype;
  typedef typename StackContainerInterface<_Ttype>::_Ctype _Ctype;
  typedef typename StackContainerInterface<_Ttype>::itype itype;
  _Ctype **const ref_conts;
  const _Stype &stack_cont;
public:
  /* Here we construct the |SizeRefinement| and allocate space for the
     refined containers. Then, the containers are created and put to
     |conts| array. Note that the containers do not claim any further
     space, since all the tensors of the created containers are in-place
     submatrices.

     Here we use a dirty trick of converting |const| pointer to non-|const|
     pointer and passing it to a subtensor container constructor. The
     containers are stored in |ref_conts| and then in |conts| from
     |StackContainer|. However, this is safe since neither |ref_conts| nor
     |conts| are used in non-|const| contexts. For example,
     |StackContainer| has only a |const| method to return a member of
     |conts|. */

  FineContainer(const _Stype &sc, int max)
    : SizeRefinement(sc.getStackSizes(), sc.numConts(), max),
      StackContainer<_Ttype>(numRefinements(), getNC()),
    ref_conts(new _Ctype *[getNC()]),
    stack_cont(sc)
  {
    for (int i = 0; i < numRefinements(); i++)
      _Stype::stack_sizes[i] = getRefSize(i);
    _Stype::calculateOffsets();

    int last_cont = -1;
    int last_row = 0;
    for (int i = 0; i < getNC(); i++)
      {
        if (getOldIndex(i) != last_cont)
          {
            last_cont = getOldIndex(i);
            last_row = 0;
          }
        union {const _Ctype *c; _Ctype *n;} convert;
        convert.c = stack_cont.getCont(last_cont);
        ref_conts[i] = new _Ctype(last_row, _Stype::stack_sizes[i],
                                  *(convert.n));
        _Stype::conts[i] = ref_conts[i];
        last_row += _Stype::stack_sizes[i];
      }
  }

  /* Here we deallocate the refined containers, and deallocate the array of
     refined containers. */
  virtual ~FineContainer()
  {
    for (int i = 0; i < _Stype::numConts(); i++)
      delete ref_conts[i];
    delete [] ref_conts;
  }
  itype
  getType(int i, const Symmetry &s) const
  {
    return stack_cont.getType(getOldIndex(i), s);
  }

};

/* Here is |FineContainer| specialization for folded tensors. */
class FoldedFineContainer : public FineContainer<FGSTensor>, public FoldedStackContainer
{
public:
  FoldedFineContainer(const StackContainer<FGSTensor> &sc, int max)
    : FineContainer<FGSTensor>(sc, max)
  {
  }
};

/* Here is |FineContainer| specialization for unfolded tensors. */
class UnfoldedFineContainer : public FineContainer<UGSTensor>, public UnfoldedStackContainer
{
public:
  UnfoldedFineContainer(const StackContainer<UGSTensor> &sc, int max)
    : FineContainer<UGSTensor>(sc, max)
  {
  }
};

#endif
