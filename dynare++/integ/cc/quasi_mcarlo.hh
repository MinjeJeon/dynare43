/*
 * Copyright © 2005 Ondra Kamenik
 * Copyright © 2019 Dynare Team
 *
 * This file is part of Dynare.
 *
 * Dynare is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Dynare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Dynare.  If not, see <https://www.gnu.org/licenses/>.
 */

// Quasi Monte Carlo quadrature.

/* This defines quasi Monte Carlo quadratures for cube and for a function
   multiplied by normal density. The quadrature for a cube is named
   QMCarloCubeQuadrature and integrates:

      ∫    f(x)dx
    [0,1]ⁿ

   The quadrature for a function of normally distributed parameters is named
   QMCarloNormalQuadrature and integrates:

        1
    ────────     ∫     f(x)e^{−½xᵀx}dx
    √{(2π)ⁿ}  [−∞,+∞]ⁿ

   For a cube we define qmcpit as iterator of QMCarloCubeQuadrature, and for
   the normal density multiplied function we define qmcnpit as iterator of
   QMCarloNormalQuadrature.

   The quasi Monte Carlo method generates low discrepancy points with equal
   weights. The one dimensional low discrepancy sequences are generated by
   RadicalInverse class, the sequences are combined for higher dimensions by
   HaltonSequence class. The Halton sequence can use a permutation scheme;
   PermutattionScheme is an abstract class for all permutaton schemes. We have
   three implementations: WarnockPerScheme, ReversePerScheme, and
   IdentityPerScheme. */

#ifndef QUASI_MCARLO_H
#define QUASI_MCARLO_H

#include "int_sequence.hh"
#include "quadrature.hh"

#include "Vector.hh"

#include <vector>

/* This abstract class declares permute() method which permutes coefficient ‘c’
   having index of ‘i’ fro the base ‘base’ and returns the permuted coefficient
   which must be in 0,…,base−1. */

class PermutationScheme
{
public:
  PermutationScheme() = default;
  virtual ~PermutationScheme() = default;
  virtual int permute(int i, int base, int c) const = 0;
};

/* This class represents an integer number ‘num’ as c₀+c₁b+c₂b²+…+cⱼbʲ, where b
   is ‘base’ and c₀,…,cⱼ is stored in ‘coeff’. The size of IntSequence coeff
   does not grow with growing ‘num’, but is fixed from the very beginning and
   is set according to supplied maximum ‘maxn’.

   The basic method is eval() which evaluates the RadicalInverse with a given
   permutation scheme and returns the point, and increase() which increases
   ‘num’ and recalculates the coefficients. */

class RadicalInverse
{
  int num;
  int base;
  int maxn;
  int j;
  IntSequence coeff;
public:
  RadicalInverse(int n, int b, int mxn);
  RadicalInverse(const RadicalInverse &ri) = default;
  RadicalInverse &operator=(const RadicalInverse &radi) = default;
  double eval(const PermutationScheme &p) const;
  void increase();
  void print() const;
};

/* This is a vector of RadicalInverses, each RadicalInverse has a different
   prime as its base. The static members ‘primes’ and ‘num_primes’ define a
   precalculated array of primes. The increase() method of the class increases
   indices in all RadicalInverse’s and sets point ‘pt’ to contain the points in
   each dimension. */

class HaltonSequence
{
private:
  static std::array<int, 170> primes;
protected:
  int num;
  int maxn;
  std::vector<RadicalInverse> ri;
  const PermutationScheme &per;
  Vector pt;
public:
  HaltonSequence(int n, int mxn, int dim, const PermutationScheme &p);
  HaltonSequence(const HaltonSequence &hs) = default;
  HaltonSequence &operator=(const HaltonSequence &hs) = delete;
  void increase();
  const Vector &
  point() const
  {
    return pt;
  }
  const int
  getNum() const
  {
    return num;
  }
  void print() const;
protected:
  void eval();
};

/* This is a specification of quasi Monte Carlo quadrature. It consists of
   dimension ‘dim’, number of points (or level) ‘lev’, and the permutation
   scheme. This class is common to all quasi Monte Carlo classes. */

class QMCSpecification
{
protected:
  int dim;
  int lev;
  const PermutationScheme &per_scheme;
public:
  QMCSpecification(int d, int l, const PermutationScheme &p)
    : dim(d), lev(l), per_scheme(p)
  {
  }
  virtual ~QMCSpecification() = default;
  int
  dimen() const
  {
    return dim;
  }
  int
  level() const
  {
    return lev;
  }
  const PermutationScheme &
  getPerScheme() const
  {
    return per_scheme;
  }
};

/* This is an iterator for quasi Monte Carlo over a cube QMCarloCubeQuadrature.
   The iterator maintains HaltonSequence of the same dimension as given by the
   specification. An iterator can be constructed from a given number ‘n’, or by
   a copy constructor. */

class qmcpit
{
protected:
  const QMCSpecification &spec;
  HaltonSequence halton;
  ParameterSignal sig;
public:
  qmcpit(const QMCSpecification &s, int n);
  qmcpit(const qmcpit &qpit) = default;
  virtual ~qmcpit() = default;
  bool operator==(const qmcpit &qpit) const;
  bool
  operator!=(const qmcpit &qpit) const
  {
    return !operator==(qpit);
  }
  qmcpit &operator=(const qmcpit &qpit) = delete;
  qmcpit &operator++();
  const ParameterSignal &
  signal() const
  {
    return sig;
  }
  const Vector &
  point() const
  {
    return halton.point();
  }
  double weight() const;
  void
  print() const
  {
    halton.print();
  }
};

/* This is an easy declaration of quasi Monte Carlo quadrature for a cube.
   Everything important has been done in its iterator qmcpit, so we only
   inherit from general Quadrature and reimplement begin() and numEvals(). */

class QMCarloCubeQuadrature : public QuadratureImpl<qmcpit>, public QMCSpecification
{
public:
  QMCarloCubeQuadrature(int d, int l, const PermutationScheme &p)
    : QuadratureImpl<qmcpit>(d), QMCSpecification(d, l, p)
  {
  }
  ~QMCarloCubeQuadrature() override = default;
  int
  numEvals(int l) const override
  {
    return l;
  }
protected:
  qmcpit
  begin(int ti, int tn, int lev) const override
  {
    return qmcpit(*this, ti*level()/tn + 1);
  }
};

/* Declares Warnock permutation scheme. */
class WarnockPerScheme : public PermutationScheme
{
public:
  int permute(int i, int base, int c) const override;
};

/* Declares reverse permutation scheme. */
class ReversePerScheme : public PermutationScheme
{
public:
  int permute(int i, int base, int c) const override;
};

/* Declares no permutation (identity) scheme. */
class IdentityPerScheme : public PermutationScheme
{
public:
  int
  permute(int i, int base, int c) const override
  {
    return c;
  }
};

#endif
