/*
 * Copyright (C) 2021 Dynare Team
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
/*
 * This file computes a second-order approximation of the neo-classical growth model.
 * It assesses the conditional and unconditional welfares computed by the evaluate_planner_objective function
 * and compares them to a by-hand assessment stemming from the results the model neo_growth.mod incur.
 */

@#include "neo_growth_ramsey_common.inc"

shocks;
var e;
stderr 1;
end;

stoch_simul(order=2, irf=0);

planner_objective_value = evaluate_planner_objective(M_, options_, oo_);

if ~exist(['neo_growth' filesep 'Output' filesep 'neo_growth_results.mat'],'file');
   error('neo_growth must be run first');
end;

oo1 = load(['neo_growth' filesep 'Output' filesep 'neo_growth_results'],'oo_');
M1 = load(['neo_growth' filesep 'Output' filesep 'neo_growth_results'],'M_');
options1 = load(['neo_growth' filesep 'Output' filesep 'neo_growth_results'],'options_');
unc_W_hand = oo1.oo_.mean(strmatch('W',M1.M_.endo_names,'exact'));

initial_condition_states = repmat(oo1.oo_.dr.ys,1,M1.M_.maximum_lag);
shock_matrix = zeros(1,M1.M_.exo_nbr);
y_sim = simult_(M1.M_,options1.options_,initial_condition_states,oo1.oo_.dr,shock_matrix,options1.options_.order);
cond_W_hand=y_sim(strmatch('W',M1.M_.endo_names,'exact'),2);

if abs((unc_W_hand - planner_objective_value(1))/unc_W_hand) > 1e-6;
   error('Inaccurate unconditional welfare assessment');
end;
if abs((cond_W_hand - planner_objective_value(2))/cond_W_hand) > 1e-6;
   error('Inaccurate conditional welfare assessment');
end;
