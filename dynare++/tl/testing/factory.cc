/* $Id: factory.cpp 148 2005-04-19 15:12:26Z kamenik $ */
/* Copyright 2004, Ondra Kamenik */

#include "factory.hh"

void
Factory::init(const Symmetry &s, const IntSequence &nvs)
{
  IntSequence sym(s);
  decltype(mtgen)::result_type seed = sym[0];
  seed = 256*seed + nvs[0];
  if (sym.size() > 1)
    seed = 256*seed + sym[1];
  if (nvs.size() > 1)
    seed = 256*seed + nvs[0];
  mtgen.seed(seed);
}

void
Factory::init(int dim, int nv)
{
  decltype(mtgen)::result_type seed = dim;
  seed = 256*seed + nv;
  mtgen.seed(seed);
}

double
Factory::get()
{
  return dis(mtgen)-0.5;
}

void
Factory::fillMatrix(TwoDMatrix &m)
{
  Vector &d = m.getData();
  for (int i = 0; i < d.length(); i++)
    d[i] = get();
}

Vector
Factory::makeVector(int n)
{
  init(n, n*n);

  Vector v(n);
  for (int i = 0; i < n; i++)
    v[i] = get();

  return v;
}
