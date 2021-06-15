/*
 * Copyright © 2005 Ondra Kamenik
 * Copyright © 2019-2021 Dynare Team
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

#include <utility>

#include "kord_exception.hh"
#include "approximation_welfare.hh"

ApproximationWelfare::ApproximationWelfare(KordwDynare &w, double discount_factor_arg, const FGSContainer &rule_ders_arg, const FGSContainer &rule_ders_s_arg, Journal &j)
  : welfare{w}, discount_factor(discount_factor_arg), cond(1),
    nvs{welfare.getModel().nys(), welfare.getModel().nexog(), welfare.getModel().nexog(), 1}, journal{j}
{
  rule_ders = std::make_unique<FGSContainer>(rule_ders_arg);
  rule_ders_s = std::make_unique<FGSContainer>(rule_ders_s_arg);
}

/* This methods assumes that the deterministic steady state is
   model.getSteady(). It makes an approximation about it and stores the
   derivatives to ‘rule_ders’ and ‘rule_ders_ss’. Also it runs a check() for
   σ=0. */
void
ApproximationWelfare::approxAtSteady()
{
  welfare.calcDerivativesAtSteady();
  KOrderWelfare korderwel(welfare.getModel().nstat(), welfare.getModel().npred(), welfare.getModel().nboth(), welfare.getModel().nforw(), welfare.getModel().nexog(), welfare.getModel().order(), discount_factor, welfare.getPlannerObjDerivatives(), get_rule_ders(), get_rule_ders_s(), welfare.getModel().getVcov(), journal); 
  for (int k = 1; k <= welfare.getModel().order(); k++)
    korderwel.performStep<Storage::fold>(k);
  saveRuleDerivs(korderwel.getFoldU(), korderwel.getFoldW());
  // construct the welfare approximations
  calcStochShift(); //conditional welfare

  // construct the resulting decision rules
  unc_fdr = std::make_unique<FoldDecisionRule>(*unc_ders, welfare.getModel().nys(), welfare.getModel().nexog(), welfare.getModel().getSteady());
  cond_fdr = std::make_unique<FoldDecisionRule>(*cond_ders, welfare.getModel().nys(), welfare.getModel().nexog(), welfare.getModel().getSteady());
}

/* This just returns ‘fdr’ with a check that it is created. */
const FoldDecisionRule &
ApproximationWelfare::getFoldUncWel() const
{
  KORD_RAISE_IF(!unc_fdr,
                "Folded decision rule has not been created in ApproximationWelfare::getFoldUncWel");
  return *unc_fdr;
}
const FoldDecisionRule &
ApproximationWelfare::getFoldCondWel() const
{
  KORD_RAISE_IF(!cond_fdr,
                "Folded decision rule has not been created in ApproximationWelfare::getFoldCondWel");
  return *cond_fdr;
}

void
ApproximationWelfare::saveRuleDerivs(const FGSContainer &U, const FGSContainer &W)
{
  unc_ders = std::make_unique<FGSContainer>(U);
  cond_ders = std::make_unique<FGSContainer>(W);
}

/* This method calculates the shift of the conditional welfare function w.r.t its steady-state value because of the presence of stochastic shocks, i.e.
               1
W ≈ Wbar + ∑   ── W_σᵈ
          d=1  d!
*/
void
ApproximationWelfare::calcStochShift()
{
  cond.zeros();
  cond.add(1.0/(1.0-discount_factor), welfare.getResid());
  KORD_RAISE_IF(cond.length() != 1,
                "Wrong length of output vector for ApproximationWelfare::calcStochShift");
  int dfac = 1;
  for (int d = 1; d <= cond_ders->getMaxDim(); d++, dfac *= d)
    if (KOrderWelfare::is_even(d))
      {
        Symmetry sym{0, 0, 0, d};
        cond.add(1.0/dfac, cond_ders->get(sym).getData());
      }
}
