// Copyright 2005, Ondra Kamenik

// Vector function.

/* This file defines interface for functions taking a vector as an input
   and returning a vector (with a different size) as an output. We are
   also introducing a parameter signalling; it is a boolean vector which
   tracks parameters which were changed from the previous call. The
   |VectorFunction| implementation can exploit this information and
   evaluate the function more efficiently. The information can be
   completely ignored.

   From the signalling reason, and from other reasons, the function
   evaluation is not |const|. */

#ifndef VECTOR_FUNCTION_H
#define VECTOR_FUNCTION_H

#include "Vector.hh"
#include "GeneralMatrix.hh"

#include <vector>

/* This is a simple class representing a vector of booleans. The items
   night be retrieved or changed, or can be set |true| after some
   point. This is useful when we multiply the vector with lower
   triangular matrix.

   |true| means that a parameter was changed. */

class ParameterSignal
{
protected:
  bool *data;
  int num;
public:
  ParameterSignal(int n);
  ParameterSignal(const ParameterSignal &sig);
  ~ParameterSignal()
  {
    delete [] data;
  }
  void signalAfter(int l);
  const bool &
  operator[](int i) const
  {
    return data[i];
  }
  bool &
  operator[](int i)
  {
    return data[i];
  }
};

/* This is the abstract class for vector function. At this level of
   abstraction we only need to know size of input vector and a size of
   output vector.

   The important thing here is a clone method, we will need to make hard
   copies of vector functions since the evaluations are not |const|. The
   hardcopies apply for parallelization. */

class VectorFunction
{
protected:
  int in_dim;
  int out_dim;
public:
  VectorFunction(int idim, int odim)
    : in_dim(idim), out_dim(odim)
  {
  }
  VectorFunction(const VectorFunction &func)
     
  = default;
  virtual ~VectorFunction()
  = default;
  virtual VectorFunction *clone() const = 0;
  virtual void eval(const Vector &point, const ParameterSignal &sig, Vector &out) = 0;
  int
  indim() const
  {
    return in_dim;
  }
  int
  outdim() const
  {
    return out_dim;
  }
};

/* This makes |n| copies of |VectorFunction|. The first constructor
   make exactly |n| new copies, the second constructor copies only the
   pointer to the first and others are hard (real) copies.

   The class is useful for making a given number of copies at once, and
   this set can be reused many times if we need mupliple copis of the
   function (for example for paralelizing the code). */

class VectorFunctionSet
{
protected:
  std::vector<VectorFunction *> funcs;
  bool first_shallow;
public:
  VectorFunctionSet(const VectorFunction &f, int n);
  VectorFunctionSet(VectorFunction &f, int n);
  ~VectorFunctionSet();
  VectorFunction &
  getFunc(int i)
  {
    return *(funcs[i]);
  }
  int
  getNum() const
  {
    return funcs.size();
  }
};

/* This class wraps another |VectorFunction| to allow integration of a
   function through normally distributed inputs. Namely, if one wants to
   integrate
   $${1\over\sqrt{(2\pi)^n\vert\Sigma\vert}}\int f(x)e^{-{1\over2}x^T\Sigma^{-1}x}{\rm d}x$$
   then if we write $\Sigma=AA^T$ and $x=\sqrt{2}Ay$, we get integral
   $${1\over\sqrt{(2\pi)^n\vert\Sigma\vert}}
   \int f\left(\sqrt{2}Ay\right)e^{-y^Ty}\sqrt{2^n}\vert A\vert{\rm d}y=
   {1\over\sqrt{\pi^n}}\int f\left(\sqrt{2}Ay\right)e^{-y^Ty}{\rm d}y,$$
   which means that a given function $f$ we have to wrap to yield a function
   $$g(y)={1\over\sqrt{\pi^n}}f\left(\sqrt{2}Ay\right).$$
   This is exactly what this class is doing. This transformation is
   useful since the Gauss--Hermite points and weights are defined for
   weighting function $e^{-y^2}$, so this transformation allows using
   Gauss--Hermite quadratures seemlessly in a context of integration through
   normally distributed inputs.

   The class maintains a pointer to the function $f$. When the object is
   constructed by the first constructor, the $f$ is not copied. If the
   object of this class is copied, then $f$ is copied and we need to
   remember to destroy it in the desctructor; hence |delete_flag|. The
   second constructor takes a pointer to the function and differs from
   the first only by setting |delete_flag| to |true|. */

class GaussConverterFunction : public VectorFunction
{
protected:
  VectorFunction *func;
  bool delete_flag;
  GeneralMatrix A;
  double multiplier;
public:
  GaussConverterFunction(VectorFunction &f, const GeneralMatrix &vcov);
  GaussConverterFunction(VectorFunction *f, const GeneralMatrix &vcov);
  GaussConverterFunction(const GaussConverterFunction &f);
  ~GaussConverterFunction() override
  {
    if (delete_flag)
      delete func;
  }
  VectorFunction *
  clone() const override
  {
    return new GaussConverterFunction(*this);
  }
  void eval(const Vector &point, const ParameterSignal &sig, Vector &out) override;
private:
  double calcMultiplier() const;
  void calcCholeskyFactor(const GeneralMatrix &vcov);
};

#endif
