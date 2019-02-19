// Copyright 2004, Ondra Kamenik

// Tensor concept.

/* Here we define a tensor class. Tensor is a mathematical object
   corresponding to a $(n+1)$-dimensional array. An element of such array
   is denoted $[B]_{\alpha_1\ldots\alpha_n}^\beta$, where $\beta$ is a
   special index and $\alpha_1\ldots\alpha_n$ are other indices. The
   class |Tensor| and its subclasses view such array as a 2D matrix,
   where $\beta$ corresponds to one dimension, and
   $\alpha_1\ldots\alpha_2$ unfold to the other dimension. Whether
   $\beta$ correspond to rows or columns is decided by tensor subclasses,
   however, most of our tensors will have rows indexed by $\beta$, and
   $\alpha_1\ldots\alpha_n$ will unfold column-wise.

   There might be some symmetries in the tensor data. For instance, if
   $\alpha_1$ is interchanged with $\alpha_3$ and the both elements equal
   for all possible $\alpha_i$, and $\beta$, then there is a symmetry
   of $\alpha_1$ and $\alpha_3$.

   For any symmetry, there are basically two possible storages of the
   data. The first is unfolded storage, which stores all elements
   regardless the symmetry. The other storage type is folded, which
   stores only elements which do not repeat. We declare abstract classes
   for unfolded tensor, and folded tensor.

   Also, here we also define a concept of tensor index which is the
   $n$-tuple $\alpha_1\ldots\alpha_n$. It is an iterator, which iterates
   in dependence of symmetry and storage of the underlying tensor.

   Although we do not decide about possible symmetries at this point, it
   is worth noting that we implement two kinds of symmetries. The first
   one is a full symmetry where all indices are interchangeable. The
   second one is a generalization of the first. We define tensor of a
   symmetry, where there are a few groups of indices interchangeable
   within a group and not across. Moreover, the groups are required to be
   consequent partitions of the index $n$-tuple. This is, we do not allow
   $\alpha_1$ be interchangeable with $\alpha_3$ and not with $\alpha_2$
   at the same time.

   However, some intermediate results are, in fact, tensors of a symmetry
   not fitting to our concept. We develop the tensor abstraction for it,
   but these objects are not used very often. They have limited usage
   due to their specialized constructor. */

#ifndef TENSOR_H
#define TENSOR_H

#include "int_sequence.hh"
#include "twod_matrix.hh"

#include <iostream>

/* Here is the |Tensor| class, which is nothing else than a simple subclass
   of |TwoDMatrix|. The unique semantically new member is |dim| which is tensor
   dimension (length of $\alpha_1\ldots\alpha_n$). We also declare
   |increment|, |decrement| and |getOffset| methods as pure virtual.

   We also add members for index begin and index end. This is useful,
   since |begin| and |end| methods do not return instance but only
   references, which prevent making additional copy of index (for example
   in for cycles as |in != end()| which would do a copy of index for each
   cycle). The index begin |in_beg| is constructed as a sequence of all
   zeros, and |in_end| is constructed from the sequence |last| passed to
   the constructor, since it depends on subclasses. Also we have to say,
   along what coordinate is the multidimensional index. This is used only
   for initialization of |in_end|. */

class Tensor : public TwoDMatrix
{
public:
  enum class indor {along_row, along_col};

  /* The index represents $n$-tuple $\alpha_1\ldots\alpha_n$. Since its
     movement is dependent on the underlying tensor (with storage and
     symmetry), we maintain a reference to that tensor, we maintain the
     $n$-tuple (or coordinates) as |IntSequence| and also we maintain the
     offset number (column, or row) of the index in the tensor. The reference
     is const, since we do not need to change data through the index.

     Here we require the |tensor| to implement |increment| and |decrement|
     methods, which calculate following and preceding $n$-tuple. Also, we
     need to calculate offset number from the given coordinates, so the
     tensor must implement method |getOffset|. This method is used only in
     construction of the index from the given coordinates. As the index is
     created, the offset is automatically incremented, and decremented
     together with index. The |getOffset| method can be relatively
     computationally complex. This must be kept in mind.  Also we generally
     suppose that n-tuple of all zeros is the first offset (first columns
     or row).

     What follows is a definition of index class, the only
     interesting point is |operator==| which decides only according to
     offset, not according to the coordinates. This is useful since there
     can be more than one of coordinate representations of past-the-end
     index. */
  class index
  {
    const Tensor &tensor;
    int offset;
    IntSequence coor;
  public:
    index(const Tensor &t, int n)
      : tensor(t), offset(0), coor(n, 0)
    {
    }
    index(const Tensor &t, const IntSequence &cr, int c)
      : tensor(t), offset(c), coor(cr)
    {
    }
    index(const Tensor &t, const IntSequence &cr)
      : tensor(t), offset(tensor.getOffset(cr)), coor(cr)
    {
    }
    index(const index &) = default;
    index(index &&) = default;
    index &operator=(const index &) = delete;
    index &operator=(index &&) = delete;
    index &
    operator++()
    {
      tensor.increment(coor);
      offset++;
      return *this;
    }
    index &
    operator--()
    {
      tensor.decrement(coor);
      offset--;
      return *this;
    }
    int
    operator*() const
    {
      return offset;
    }
    bool
    operator==(const index &n) const
    {
      return offset == n.offset;
    }
    bool
    operator!=(const index &n) const
    {
      return offset != n.offset;
    }
    const IntSequence &
    getCoor() const
    {
      return coor;
    }
    void
    print() const
    {
      std::cout << offset << ": ";
      coor.print();
    }
  };

protected:
  const index in_beg;
  const index in_end;
  int dim;
public:
  Tensor(indor io, const IntSequence &last, int r, int c, int d)
    : TwoDMatrix(r, c),
      in_beg(*this, d),
      in_end(*this, last, (io == indor::along_row) ? r : c),
      dim(d)
  {
  }
  Tensor(indor io, const IntSequence &first, const IntSequence &last,
         int r, int c, int d)
    : TwoDMatrix(r, c),
      in_beg(*this, first, 0),
      in_end(*this, last, (io == indor::along_row) ? r : c),
      dim(d)
  {
  }
  Tensor(int first_row, int num, Tensor &t)
    : TwoDMatrix(first_row, num, t),
      in_beg(t.in_beg),
      in_end(t.in_end),
      dim(t.dim)
  {
  }
  Tensor(const Tensor &t)
    : TwoDMatrix(t),
      in_beg(*this, t.in_beg.getCoor(), *(t.in_beg)),
      in_end(*this, t.in_end.getCoor(), *(t.in_end)),
      dim(t.dim)
  {
  }
  Tensor(Tensor &&) = default;
  ~Tensor() override = default;

  Tensor &operator=(const Tensor &) = delete;
  Tensor &operator=(Tensor &&) = delete;

  virtual void increment(IntSequence &v) const = 0;
  virtual void decrement(IntSequence &v) const = 0;
  virtual int getOffset(const IntSequence &v) const = 0;
  int
  dimen() const
  {
    return dim;
  }

  const index &
  begin() const
  {
    return in_beg;
  }
  const index &
  end() const
  {
    return in_end;
  }

  static int
  noverseq(const IntSequence &s)
  {
    IntSequence seq(s);
    return noverseq_ip((IntSequence &) s);
  }
private:
  static int noverseq_ip(IntSequence &s);
};

/* Here is an abstraction for unfolded tensor. We provide a pure
   virtual method |fold| which returns a new instance of folded tensor of
   the same symmetry. Also we provide static methods for incrementing and
   decrementing an index with full symmetry and general symmetry as
   defined above. */

class FTensor;
class UTensor : public Tensor
{
public:
  UTensor(indor io, const IntSequence &last, int r, int c, int d)
    : Tensor(io, last, r, c, d)
  {
  }
  UTensor(const UTensor &) = default;
  UTensor(UTensor &&) = default;
  UTensor(int first_row, int num, UTensor &t)
    : Tensor(first_row, num, t)
  {
  }
  ~UTensor() override = default;
  virtual FTensor &fold() const = 0;

  UTensor &operator=(const UTensor &) = delete;
  UTensor &operator=(UTensor &&) = delete;

  static void increment(IntSequence &v, int nv);
  static void decrement(IntSequence &v, int nv);
  static void increment(IntSequence &v, const IntSequence &nvmx);
  static void decrement(IntSequence &v, const IntSequence &nvmx);
  static int getOffset(const IntSequence &v, int nv);
  static int getOffset(const IntSequence &v, const IntSequence &nvmx);
};

/* This is an abstraction for folded tensor. It only provides a method
   |unfold|, which returns the unfolded version of the same symmetry, and
   static methods for decrementing indices.

   We also provide static methods for decrementing the |IntSequence| in
   folded fashion and also calculating an offset for a given
   |IntSequence|. However, this is relatively complex calculation, so
   this should be avoided if possible. */

class FTensor : public Tensor
{
public:
  FTensor(indor io, const IntSequence &last, int r, int c, int d)
    : Tensor(io, last, r, c, d)
  {
  }
  FTensor(const FTensor &) = default;
  FTensor(FTensor &&) = default;
  FTensor(int first_row, int num, FTensor &t)
    : Tensor(first_row, num, t)
  {
  }
  ~FTensor() override = default;
  virtual UTensor &unfold() const = 0;

  FTensor &operator=(const FTensor &) = delete;
  FTensor &operator=(FTensor &&) = delete;

  static void decrement(IntSequence &v, int nv);
  static int
  getOffset(const IntSequence &v, int nv)
  {
    IntSequence vtmp(v);
    return getOffsetRecurse(vtmp, nv);
  }
private:
  static int getOffsetRecurse(IntSequence &v, int nv);
};

#endif
