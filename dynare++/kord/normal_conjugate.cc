// Copyright 2007, Ondra Kamenik

#include "normal_conjugate.hh"
#include "kord_exception.hh"

// |NormalConj| diffuse prior constructor
NormalConj::NormalConj(int d)
  : mu(d), kappa(0), nu(-1), lambda(d, d)
{
  mu.zeros();
  lambda.zeros();
}

// |NormalConj| data update constructor
NormalConj::NormalConj(const ConstTwoDMatrix &ydata)
  : mu(ydata.numRows()), kappa(ydata.numCols()), nu(ydata.numCols()-1),
    lambda(ydata.numRows(), ydata.numRows())
{
  mu.zeros();
  for (int i = 0; i < ydata.numCols(); i++)
    mu.add(1.0/ydata.numCols(), ydata.getCol(i));

  lambda.zeros();
  for (int i = 0; i < ydata.numCols(); i++)
    {
      Vector diff{ydata.getCol(i)};
      diff.add(-1, mu);
      lambda.addOuter(diff);
    }
}

NormalConj::NormalConj(const NormalConj &nc)
   
= default;

// |NormalConj::update| one observation code
/* The method performs the following:
   $$\eqalign{
   \mu_1 = &\; {\kappa_0\over \kappa_0+1}\mu_0 + {1\over \kappa_0+1}y\cr
   \kappa_1 = &\; \kappa_0 + 1\cr
   \nu_1 = &\; \nu_0 + 1\cr
   \Lambda_1 = &\; \Lambda_0 + {\kappa_0\over\kappa_0+1}(y-\mu_0)(y-\mu_0)^T,
   }$$ */
void
NormalConj::update(const ConstVector &y)
{
  KORD_RAISE_IF(y.length() != mu.length(),
                "Wrong length of a vector in NormalConj::update");

  mu.mult(kappa/(1.0+kappa));
  mu.add(1.0/(1.0+kappa), y);

  Vector diff(y);
  diff.add(-1, mu);
  lambda.addOuter(diff, kappa/(1.0+kappa));

  kappa++;
  nu++;
}

// |NormalConj::update| multiple observations code
/* The method evaluates the formula in the header file. */
void
NormalConj::update(const ConstTwoDMatrix &ydata)
{
  NormalConj nc(ydata);
  update(nc);
}

// |NormalConj::update| with |NormalConj| code
void
NormalConj::update(const NormalConj &nc)
{
  double wold = ((double) kappa)/(kappa+nc.kappa);
  double wnew = 1-wold;

  mu.mult(wold);
  mu.add(wnew, nc.mu);

  Vector diff(nc.mu);
  diff.add(-1, mu);
  lambda.add(1.0, nc.lambda);
  lambda.addOuter(diff);

  kappa = kappa + nc.kappa;
  nu = nu + nc.kappa;
}

/* This returns ${1\over \nu-d-1}\Lambda$, which is the mean of the
   variance in the posterior distribution. If the number of degrees of
   freedom is less than $d$, then NaNs are returned. */
void
NormalConj::getVariance(TwoDMatrix &v) const
{
  if (nu > getDim()+1)
    {
      v = (const TwoDMatrix &) lambda;
      v.mult(1.0/(nu-getDim()-1));
    }
  else
    v.nans();
}
