/* $Header: /var/lib/cvs/dynare_cpp/sylv/cc/Vector.h,v 1.1.1.1 2004/06/04 13:01:13 kamenik Exp $ */

/* Tag $Name:  $ */

#ifndef VECTOR_H
#define VECTOR_H

/* NOTE: Vector and ConstVector have not common super class in order
   to avoid running virtual method invokation mechanism. Some
   members, and methods are thus duplicated */

#include <complex>
#include <utility>

#if defined(MATLAB_MEX_FILE) || defined(OCTAVE_MEX_FILE)
# include <dynmex.h>
#endif

class GeneralMatrix;
class ConstVector;

class Vector
{
  friend class ConstVector;
protected:
  int len{0};
  int s{1}; // stride (also called “skip” in some places)
  double *data;
  bool destroy{true};
public:
  Vector() : data{nullptr}, destroy{false}
  {
  }
  Vector(int l) : len{l}, data{new double[l]}
  {
  }
  Vector(Vector &v) : len{v.len}, s{v.s}, data{v.data}, destroy{false}
  {
  }
  Vector(const Vector &v);
  Vector(Vector &&v) : len{std::exchange(v.len, 0)}, s{v.s},
                       data{std::exchange(v.data, nullptr)},
                       destroy{std::exchange(v.destroy, false)}
  {
  }
  // We don't want implict conversion from ConstVector, since it’s expensive
  explicit Vector(const ConstVector &v);
  Vector(double *d, int l)
    : len(l), data{d}, destroy{false}
  {
  }
  Vector(Vector &v, int off_arg, int l);
  Vector(const Vector &v, int off_arg, int l);
  Vector(Vector &v, int off_arg, int skip, int l);
  Vector(const Vector &v, int off_arg, int skip, int l);
#if defined(MATLAB_MEX_FILE) || defined(OCTAVE_MEX_FILE)
  explicit Vector(mxArray *p);
#endif
  Vector &operator=(const Vector &v);
  Vector &operator=(Vector &&v);
  Vector &operator=(const ConstVector &v);
  double &
  operator[](int i)
  {
    return data[s*i];
  }
  const double &
  operator[](int i) const
  {
    return data[s*i];
  }
  const double *
  base() const
  {
    return data;
  }
  double *
  base()
  {
    return data;
  }
  int
  length() const
  {
    return len;
  }
  int
  skip() const
  {
    return s;
  }

  // Exact equality.
  bool operator==(const Vector &y) const;
  bool operator!=(const Vector &y) const;
  // Lexicographic ordering.
  bool operator<(const Vector &y) const;
  bool operator<=(const Vector &y) const;
  bool operator>(const Vector &y) const;
  bool operator>=(const Vector &y) const;

  virtual ~Vector()
  {
    if (destroy)
      delete[] data;
  }
  void zeros();
  void nans();
  void infs();
  void rotatePair(double alpha, double beta1, double beta2, int i);
  // Computes this = this + r·v
  void add(double r, const Vector &v);
  // Computes this = this + r·v
  void add(double r, const ConstVector &v);
  // Computes this = this + z·v (where this and v are intepreted as complex vectors)
  void addComplex(const std::complex<double> &z, const Vector &v);
  // Computes this = this + z·v (where this and v are intepreted as complex vectors)
  void addComplex(const std::complex<double> &z, const ConstVector &v);
  void mult(double r);
  double getNorm() const;
  double getMax() const;
  double getNorm1() const;
  double dot(const Vector &y) const;
  bool isFinite() const;
  void print() const;

  /* Computes:
     ⎛x₁⎞ ⎛ α −β₁⎞   ⎛b₁⎞
     ⎢  ⎥=⎢      ⎥⊗I·⎢  ⎥
     ⎝x₂⎠ ⎝−β₂ α ⎠   ⎝b₂⎠
  */
  static void mult2(double alpha, double beta1, double beta2,
                    Vector &x1, Vector &x2,
                    const Vector &b1, const Vector &b2);
  /* Computes:
     ⎛x₁⎞ ⎛x₁⎞ ⎛ α −β₁⎞   ⎛b₁⎞
     ⎢  ⎥=⎢  ⎥+⎢      ⎥⊗I·⎢  ⎥
     ⎝x₂⎠ ⎝x₂⎠ ⎝−β₂ α ⎠   ⎝b₂⎠
  */
  static void mult2a(double alpha, double beta1, double beta2,
                     Vector &x1, Vector &x2,
                     const Vector &b1, const Vector &b2);
  /* Computes:
     ⎛x₁⎞ ⎛x₁⎞ ⎛ α −β₁⎞   ⎛b₁⎞
     ⎢  ⎥=⎢  ⎥−⎢      ⎥⊗I·⎢  ⎥
     ⎝x₂⎠ ⎝x₂⎠ ⎝−β₂ α ⎠   ⎝b₂⎠
  */
  static void
  mult2s(double alpha, double beta1, double beta2,
         Vector &x1, Vector &x2,
         const Vector &b1, const Vector &b2)
  {
    mult2a(-alpha, -beta1, -beta2, x1, x2, b1, b2);
  }
private:
  void copy(const double *d, int inc);
};

class ConstGeneralMatrix;

class ConstVector
{
  friend class Vector;
protected:
  int len;
  int s{1}; // stride (also called “skip” in some places)
  const double *data;
public:
  // Implicit conversion from Vector is ok, since it’s cheap
  ConstVector(const Vector &v);
  ConstVector(const ConstVector &v) = default;
  ConstVector(ConstVector &&v) = default;
  ConstVector(const double *d, int l) : len{l}, data{d}
  {
  }
  ConstVector(const ConstVector &v, int off_arg, int l);
  ConstVector(const ConstVector &v, int off_arg, int skip, int l);
  ConstVector(const double *d, int skip, int l);
#if defined(MATLAB_MEX_FILE) || defined(OCTAVE_MEX_FILE)
  explicit ConstVector(const mxArray *p);
#endif
  virtual ~ConstVector() = default;
  ConstVector &operator=(const ConstVector &v) = delete;
  ConstVector &operator=(ConstVector &&v) = delete;
  const double &
  operator[](int i) const
  {
    return data[s*i];
  }
  const double *
  base() const
  {
    return data;
  }
  int
  length() const
  {
    return len;
  }
  int
  skip() const
  {
    return s;
  }
  // Exact equality
  bool operator==(const ConstVector &y) const;
  bool
  operator!=(const ConstVector &y) const
  {
    return !operator==(y);
  }
  // Lexicographic ordering
  bool operator<(const ConstVector &y) const;
  bool
  operator<=(const ConstVector &y) const
  {
    return operator<(y) || operator==(y);
  }
  bool
  operator>(const ConstVector &y) const
  {
    return !operator<=(y);
  }
  bool
  operator>=(const ConstVector &y) const
  {
    return !operator<(y);
  }

  double getNorm() const;
  double getMax() const;
  double getNorm1() const;
  double dot(const ConstVector &y) const;
  bool isFinite() const;
  void print() const;
};

#endif /* VECTOR_H */
