/*
 * Copyright (C) 2008-2019 Dynare Team
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
 * along with Dynare.  If not, see <http://www.gnu.org/licenses/>.
 */

// GP, based on work by O.Kamenik

#include "k_ord_dynare.hh"
#include "dynamic_abstract_class.hh"
#include "dynare_exception.hh"

#include <utility>

KordpDynare::KordpDynare(const std::vector<std::string> &endo,
                         const std::vector<std::string> &exo, int nexog, int npar,
                         Vector &ysteady, TwoDMatrix &vcov, Vector &inParams, int nstat,
                         int npred, int nforw, int nboth, const Vector &nnzd,
                         int nsteps, int norder,
                         Journal &jr, std::unique_ptr<DynamicModelAC> dynamicModelFile_arg,
                         const std::vector<int> &dr_order, const TwoDMatrix &llincidence,
                         std::unique_ptr<TwoDMatrix> g1_arg, std::unique_ptr<TwoDMatrix> g2_arg,
                         std::unique_ptr<TwoDMatrix> g3_arg) :
  nStat{nstat}, nBoth{nboth}, nPred{npred}, nForw{nforw}, nExog{nexog}, nPar{npar},
  nYs{npred + nboth}, nYss{nboth + nforw}, nY{nstat + npred + nboth + nforw},
  nJcols{nExog+nY+nYs+nYss}, NNZD{nnzd}, nSteps{nsteps},
  nOrder{norder}, journal{jr}, ySteady{ysteady}, params{inParams}, vCov{vcov},
  md{1}, dnl{*this, endo}, denl{*this, exo}, dsnl{*this, dnl, denl},
  ll_Incidence{llincidence},
  g1p{std::move(g1_arg)}, g2p{std::move(g2_arg)}, g3p{std::move(g3_arg)},
  dynamicModelFile{std::move(dynamicModelFile_arg)}
{
  computeJacobianPermutation(dr_order);
}

void
KordpDynare::solveDeterministicSteady()
{
  JournalRecordPair pa(journal);
  pa << "Non-linear solver for deterministic steady state skipped" << endrec;
}

void
KordpDynare::evaluateSystem(Vector &out, const ConstVector &yy, const Vector &xx)
{
  // This method is only called when checking the residuals at steady state (Approximation::check), so return zero residuals
  out.zeros();
}

void
KordpDynare::evaluateSystem(Vector &out, const ConstVector &yym, const ConstVector &yy,
                            const ConstVector &yyp, const Vector &xx)
{
  // This method is only called when checking the residuals at steady state (Approximation::check), so return zero residuals
  out.zeros();
}

void
KordpDynare::calcDerivativesAtSteady()
{
  if (!g1p)
    {
      g1p = std::make_unique<TwoDMatrix>(nY, nJcols);
      g1p->zeros();

      if (nOrder > 1)
        {
          // allocate space for sparse Hessian
          g2p = std::make_unique<TwoDMatrix>(static_cast<int>(NNZD[1]), 3);
          g2p->zeros();
        }

      if (nOrder > 2)
        {
          g3p = std::make_unique<TwoDMatrix>(static_cast<int>(NNZD[2]), 3);
          g3p->zeros();
        }

      Vector xx(nexog());
      xx.zeros();

      Vector out(nY);
      out.zeros();
      Vector llxSteady(nJcols-nExog);
      LLxSteady(ySteady, llxSteady);

      dynamicModelFile->eval(llxSteady, xx, params, ySteady, out, g1p.get(), g2p.get(), g3p.get());
    }

  populateDerivativesContainer(*g1p, 1);

  if (nOrder > 1)
    populateDerivativesContainer(*g2p, 2);

  if (nOrder > 2)
    populateDerivativesContainer(*g3p, 3);
}

void
KordpDynare::populateDerivativesContainer(const TwoDMatrix &g, int ord)
{
  // model derivatives FSSparseTensor instance
  auto mdTi = std::make_unique<FSSparseTensor>(ord, nJcols, nY);

  IntSequence s(ord, 0);

  if (ord == 1)
    for (int i = 0; i < g.ncols(); i++)
      {
        for (int j = 0; j < g.nrows(); j++)
          {
            double x = g.get(j, dynppToDyn[s[0]]);
            if (x != 0.0)
              mdTi->insert(s, j, x);
          }
        s[0]++;
      }
  else if (ord == 2)
    {
      for (int i = 0; i < g.nrows(); i++)
        {
          int j = static_cast<int>(g.get(i, 0))-1; // hessian indices start with 1
          int i1 = static_cast<int>(g.get(i, 1))-1;
          if (j < 0 || i1 < 0)
            continue; // Discard empty entries (see comment in DynamicModelAC::unpackSparseMatrix())
          int s0 = i1 / nJcols;
          int s1 = i1 % nJcols;
          s[0] = dynToDynpp[s0];
          s[1] = dynToDynpp[s1];
          if (s[1] >= s[0])
            {
              double x = g.get(i, 2);
              mdTi->insert(s, j, x);
            }
        }
    }
  else if (ord == 3)
    {
      int nJcols2 = nJcols*nJcols;
      for (int i = 0; i < g.nrows(); i++)
        {
          int j = static_cast<int>(g.get(i, 0))-1;
          int i1 = static_cast<int>(g.get(i, 1))-1;
          if (j < 0 || i1 < 0)
            continue; // Discard empty entries (see comment in DynamicModelAC::unpackSparseMatrix())
          int s0 = i1 / nJcols2;
          int i2 = i1 % nJcols2;
          int s1 = i2 / nJcols;
          int s2 = i2 % nJcols;
          s[0] = dynToDynpp[s0];
          s[1] = dynToDynpp[s1];
          s[2] = dynToDynpp[s2];
          if (s.isSorted())
            {
              double x = g.get(i, 2);
              mdTi->insert(s, j, x);
            }
        }
    }

  md.insert(std::move(mdTi));
}

/* Returns ySteady extended with leads and lags suitable for passing to
   <model>_dynamic */
void
KordpDynare::LLxSteady(const Vector &yS, Vector &llxSteady)
{
  if (yS.length() == nJcols-nExog)
    throw DynareException(__FILE__, __LINE__, "ySteady already of right size");

  /* Create temporary square 2D matrix size nEndo×nEndo (sparse)
     for the lag, current and lead blocks of the jacobian */
  if (llxSteady.length() != nJcols-nExog)
    throw DynareException(__FILE__, __LINE__, "llxSteady has wrong size");

  for (int ll_row = 0; ll_row < ll_Incidence.nrows(); ll_row++)
    // populate (non-sparse) vector with ysteady values
    for (int i = 0; i < nY; i++)
      if (ll_Incidence.get(ll_row, i))
        llxSteady[static_cast<int>(ll_Incidence.get(ll_row, i))-1] = yS[i];
}

/*
   Computes mapping between Dynare and Dynare++ orderings of the (dynamic)
   variable indices in derivatives.

   If one defines:
   – y (resp. x) as the vector of all endogenous (size nY), in DR-order (resp.
     declaration order)
   – y⁻ (resp. x⁻) as the vector of endogenous that appear at previous period (size nYs),
     in DR-order (resp. declaration order)
   – y⁺ (resp. x⁺) as the vector of endogenous that appear at future period (size nYss) in
     DR-order (resp. declaration order)
   – u as the vector of exogenous (size nExog)

   In Dynare, the ordering is (x⁻, x, x⁺, u).
   In Dynare++, the ordering is (y⁺, y, y⁻, u).

   dr_order is typically equal to M_.order_var.
*/
void
KordpDynare::computeJacobianPermutation(const std::vector<int> &dr_order)
{
  // Compute restricted inverse DR-orderings: x⁻→y⁻ and x⁺→y⁺
  std::vector<int> dr_inv_order_forw(nBoth+nForw), dr_inv_order_pred(nBoth+nPred);
  std::iota(dr_inv_order_forw.begin(), dr_inv_order_forw.end(), 0);
  std::sort(dr_inv_order_forw.begin(), dr_inv_order_forw.end(),
            [&](int i, int j) { return dr_order[nStat+nPred+i] < dr_order[nStat+nPred+j]; });
  std::iota(dr_inv_order_pred.begin(), dr_inv_order_pred.end(), 0);
  std::sort(dr_inv_order_pred.begin(), dr_inv_order_pred.end(),
            [&](int i, int j) { return dr_order[nStat+i] < dr_order[nStat+j]; });

  // Compute restricted DR-orderings: y⁻→x⁻ and y⁺→x⁺
  std::vector<int> dr_order_forw(nBoth+nForw), dr_order_pred(nBoth+nPred);
  for (int i = 0; i < nBoth+nForw; i++)
    dr_order_forw[dr_inv_order_forw[i]] = i;
  for (int i = 0; i < nBoth+nPred; i++)
    dr_order_pred[dr_inv_order_pred[i]] = i;

  // Compute Dynare++ → Dynare ordering
  dynppToDyn.resize(nJcols);
  int j = 0;
  for (; j < nYss; j++)
    dynppToDyn[j] = dr_order_forw[j]+nYs+nY; // Forward variables
  for (; j < nYss+nY; j++)
    dynppToDyn[j] = dr_order[j-nYss]+nYs; // Variables in current period
  for (; j < nYss+nY+nYs; j++)
    dynppToDyn[j] = dr_order_pred[j-nY-nYss]; // Predetermined variables
  for (; j < nJcols; j++)
    dynppToDyn[j] = j; // Exogenous

  // Compute Dynare → Dynare++ ordering
  dynToDynpp.resize(nJcols);
  for (int i = 0; i < nJcols; i++)
    dynToDynpp[dynppToDyn[i]] = i;
}


DynareNameList::DynareNameList(const KordpDynare &dynare, std::vector<std::string> names_arg)
  : names(std::move(names_arg))
{
}

DynareStateNameList::DynareStateNameList(const KordpDynare &dynare, const DynareNameList &dnl,
                                         const DynareNameList &denl)
{
  for (int i = 0; i < dynare.nYs; i++)
    names.emplace_back(dnl.getName(i+dynare.nstat()));
  for (int i = 0; i < dynare.nexog(); i++)
    names.emplace_back(denl.getName(i));
}
