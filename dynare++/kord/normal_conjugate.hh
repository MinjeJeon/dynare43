// Copyright 2007, Ondra Kamenik

// Conjugate family for normal distribution

/* The main purpose here is to implement a class representing conjugate
   distributions for mean and variance of the normal distribution. The
   class has two main methods: the first one is to update itself with
   respect to one observation, the second one is to update itself with
   respect to anothe object of the class. In the both methods, the
   previous state of the class corresponds to the prior distribution, and
   the final state corresponds to the posterior distribution.

   The algrebra can be found in Gelman, Carlin, Stern, Rubin (p.87). It
   goes as follows: Prior conjugate distribution takes the following form:
   $$\eqalign{
   \Sigma \sim& {\rm InvWishart}_{\nu_0}(\Lambda_0^{-1}) \cr
   \mu\vert\Sigma \sim& N(\mu_0,\Sigma/\kappa_0)
   }$$
   If the observations are $y_1\ldots y_n$, then the posterior distribution has the
   same form with the following parameters:
   $$\eqalign{
   \mu_n = &\; {\kappa_0\over \kappa_0+n}\mu_0 + {n\over \kappa_0+n}\bar y\cr
   \kappa_n = &\; \kappa_0 + n\cr
   \nu_n = &\; \nu_0 + n\cr
   \Lambda_n = &\; \Lambda_0 + S + {\kappa_0 n\over\kappa_0+n}(\bar y-\mu_0)(\bar y-\mu_0)^T,
   }$$
   where
   $$\eqalign{
   \bar y = &\; {1\over n}\sum_{i=1}^ny_i\cr
   S = &\; \sum_{i=1}^n(y_i-\bar y)(y_i-\bar y)^T
   }$$ */

#ifndef NORMAL_CONJUGATE_H
#define NORMAL_CONJUGATE_H

#include "twod_matrix.hh"

/* The class is described by the four parameters: $\mu$, $\kappa$, $\nu$ and
   $\Lambda$. */

class NormalConj
{
protected:
  Vector mu;
  int kappa;
  int nu;
  TwoDMatrix lambda;
public:
  /* We provide the following constructors: The first constructs diffuse
     (Jeffrey's) prior. It sets $\kappa$, and $\Lambda$ to zeros, $nu$ to
     $-1$ and also the mean $\mu$ to zero (it should not be
     referenced). The second constructs the posterior using the diffuse
     prior and the observed data (columnwise). The third is a copy
     constructor. */
  NormalConj(int d);
  NormalConj(const ConstTwoDMatrix &ydata);
  NormalConj(const NormalConj &nc);

  virtual ~NormalConj()
  = default;
  void update(const ConstVector &y);
  void update(const ConstTwoDMatrix &ydata);
  void update(const NormalConj &nc);
  int
  getDim() const
  {
    return mu.length();
  }
  const Vector &
  getMean() const
  {
    return mu;
  }
  void getVariance(TwoDMatrix &v) const;
};

#endif
