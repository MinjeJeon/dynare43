/* $Header: /var/lib/cvs/dynare_cpp/sylv/cc/SchurDecomp.cpp,v 1.1.1.1 2004/06/04 13:00:44 kamenik Exp $ */

/* Tag $Name:  $ */

#include "SchurDecomp.hh"

#include <memory>

#include <dynlapack.h>

SchurDecomp::SchurDecomp(const SqSylvMatrix &m)
  : q(m.nrows())
{
  lapack_int rows = m.nrows();
  SqSylvMatrix auxt(m);
  lapack_int lda = auxt.getLD(), ldvs = q.getLD();
  lapack_int sdim;
  auto wr = std::make_unique<double []>(rows);
  auto wi = std::make_unique<double []>(rows);
  lapack_int lwork = 6*rows;
  auto work = std::make_unique<double []>(lwork);
  lapack_int info;
  dgees("V", "N", nullptr, &rows, auxt.base(), &lda, &sdim,
        wr.get(), wi.get(), q.base(), &ldvs,
        work.get(), &lwork, nullptr, &info);
  t_storage = std::make_unique<QuasiTriangular>(auxt.getData(), rows);
  t = t_storage.get();
}

SchurDecomp::SchurDecomp(const QuasiTriangular &tr)
  : q(tr.nrows()), t_storage{std::make_unique<QuasiTriangular>(tr)}, t{t_storage.get()}
{
  q.setUnit();
}

SchurDecomp::SchurDecomp(QuasiTriangular &tr)
  : q(tr.nrows()), t{&tr}
{
  q.setUnit();
}

int
SchurDecomp::getDim() const
{
  return t->nrows();
}

SchurDecompZero::SchurDecompZero(const GeneralMatrix &m)
  : SchurDecomp(SqSylvMatrix(m, m.nrows()-m.ncols(), 0, m.ncols())),
    ru(m, 0, 0, m.nrows()-m.ncols(), m.ncols())
{
  ru.multRight(getQ());
}

int
SchurDecompZero::getDim() const
{
  return getT().nrows()+ru.nrows();
}
