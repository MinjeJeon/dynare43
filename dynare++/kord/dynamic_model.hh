// Copyright 2005, Ondra Kamenik

// Dynamic model abstraction

/* This file only defines a generic interface to an SDGE model. The model
   takes the form:
   $$E_t\left[f(g^{**}(g^*(y,u_t),u_{t+1}),g(y,u),y,u_t)\right]=0$$
   The interface is defined via pure virtual class |DynamicModel|. */

#ifndef DYNAMIC_MODEL_H
#define DYNAMIC_MODEL_H

#include "t_container.hh"
#include "sparse_tensor.hh"

#include "Vector.h"

/* The class is a virtual pure class which provides an access to names
   of the variables. */

class NameList
{
public:
  virtual ~NameList()
  {
  }
  virtual int getNum() const = 0;
  virtual const char *getName(int i) const = 0;
  void print() const;
  void writeMat(mat_t *fd, const char *vname) const;
  void writeMatIndices(mat_t *fd, const char *prefix) const;
};

/* This is the interface to an information on a generic SDGE
   model. It is sufficient for calculations of policy rule Taylor
   approximations at some (not necessarily deterministic) steady state.

   We need to know a partitioning of endogenous variables $y$. We suppose
   that $y$ is partitioned as
   $$y=\left[\matrix{\hbox{static}\cr\hbox{pred}\cr\hbox{both}\cr\hbox{forward}}\right]$$
   of which we define
   $$y^*=\left[\matrix{\hbox{pred}\cr\hbox{both}}\right]\quad
   y^{**}=\left[\matrix{\hbox{both}\cr\hbox{forward}}\right]$$
   where ``static'' are meant those variables, which appear only at time
   $t$; ``pred'' are meant those variables, which appear only at $t$ and
   $t-1$; ``both'' are meant those variables, which appear at least at
   $t-1$ and $t+1$; and ``forward'' are meant those variables, which
   appear only at $t$ and $t+1$. This partitioning is given by methods
   |nstat()|, |npred()|, |nboth()|, and |nforw()|. The number of
   equations |numeq()| must be the same as a number of endogenous
   variables.

   In order to complete description, we need to know a number of
   exogenous variables, which is a size of $u$, hence |nexog()| method.

   The model contains an information about names of variables, the
   variance-covariance matrix of the shocks, the derivatives of equations
   of $f$ at some steady state, and the steady state. These can be
   retrieved by the corresponding methods.

   The derivatives of the system are calculated with respect to stacked
   variables, the stack looks as:
   $$\left[\matrix{y^{**}_{t+1}\cr y_t\cr y^*_{t-1}\cr u_t}\right].$$

   There are only three operations. The first
   |solveDeterministicSteady()| solves the deterministic steady steate
   which can be retrieved by |getSteady()| later. The method
   |evaluateSystem| calculates $f(y^{**},y,y^*,u)$, where $y$ and $u$ are
   passed, or $f(y^{**}_{t+1}, y_t, y^*_{t-1}, u)$, where $y^{**}_{t+1}$,
   $y_t$, $y^*_{t-1}$, $u$ are passed. Finally, the method
   |calcDerivativesAtSteady()| calculates derivatives of $f$ at the
   current steady state, and zero shocks. The derivatives can be
   retrieved with |getModelDerivatives()|. All the derivatives are done
   up to a given order in the model, which can be retrieved by |order()|.

   The model initialization is done in a constructor of the implementing
   class. The constructor usually calls a parser, which parses a given
   file (usually a text file), and retrieves all necessary information
   about the model, inluding variables, partitioning, variance-covariance
   matrix, information helpful for calculation of the deterministic
   steady state, and so on. */

class DynamicModel
{
public:
  virtual DynamicModel *clone() const = 0;
  virtual ~DynamicModel()
  {
  }

  virtual int nstat() const = 0;
  virtual int nboth() const = 0;
  virtual int npred() const = 0;
  virtual int nforw() const = 0;
  virtual int nexog() const = 0;
  virtual int order() const = 0;
  int
  numeq() const
  {
    return nstat()+nboth()+npred()+nforw();
  }

  virtual const NameList&getAllEndoNames() const = 0;
  virtual const NameList&getStateNames() const = 0;
  virtual const NameList&getExogNames() const = 0;
  virtual const TwoDMatrix&getVcov() const = 0;
  virtual const TensorContainer<FSSparseTensor>&getModelDerivatives() const = 0;
  virtual const Vector&getSteady() const = 0;
  virtual Vector&getSteady() = 0;

  virtual void solveDeterministicSteady() = 0;
  virtual void evaluateSystem(Vector &out, const Vector &yy, const Vector &xx) = 0;
  virtual void evaluateSystem(Vector &out, const Vector &yym, const Vector &yy,
                              const Vector &yyp, const Vector &xx) = 0;
  virtual void calcDerivativesAtSteady() = 0;
};

#endif
