/*
 * Copyright © 2021 Dynare Team
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

#ifndef APPROXIMATION_WELFARE_H
#define APPROXIMATION_WELFARE_H

#include "k_ord_objective.hh"
#include "journal.hh"

#include <memory>

class ApproximationWelfare
{
  KordwDynare &welfare;
  double discount_factor;
  std::unique_ptr<FGSContainer> rule_ders;
  std::unique_ptr<FGSContainer> rule_ders_s;
  std::unique_ptr<FGSContainer> unc_ders;
  std::unique_ptr<FGSContainer> cond_ders;
  std::unique_ptr<FoldDecisionRule> unc_fdr;
  std::unique_ptr<FoldDecisionRule> cond_fdr;
  Vector cond;
  // const FNormalMoments mom;
  IntSequence nvs;
  // TwoDMatrix ss;
  Journal &journal;
public:
  ApproximationWelfare(KordwDynare &w, double discount_factor, const FGSContainer &rule_ders, const FGSContainer &rule_ders_s, Journal &j);

  const KordwDynare &
  getWelfare() const
  {
    return welfare;
  }
  const FGSContainer &
  get_rule_ders() const
  {
    return *rule_ders;
  }
  const FGSContainer &
  get_rule_ders_s() const
  {
    return *rule_ders_s;
  }
  const FGSContainer &
  get_unc_ders() const
  {
    return *unc_ders;
  }
  const FGSContainer &
  get_cond_ders() const
  {
    return *cond_ders;
  }
  const Vector &
  getCond() const
  {
    return cond;
  }
  const FoldDecisionRule & getFoldUncWel() const;
  const FoldDecisionRule & getUnfoldUncWel() const;
  const FoldDecisionRule & getFoldCondWel() const;
  const FoldDecisionRule & getUnfoldCondWel() const;

  void approxAtSteady();

protected:
  void saveRuleDerivs(const FGSContainer &U, const FGSContainer &W);
  void calcStochShift();
};

#endif
