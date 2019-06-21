function [endogenousvariables, exogenousvariables] = static_model_inversion(constraints, exogenousvariables, endo_names, exo_names, freeinnovations, DynareModel, DynareOptions, ~)

% INPUTS
% - constraints         [dseries]        with N constrained endogenous variables from t1 to t2.
% - exogenousvariables  [dseries]        with Q exogenous variables.
% - endo_names          [cell]           list of endogenous variable names.
% - exo_names           [cell]           list of exogenous variable names.
% - freeinstruments     [cell]           list of exogenous variable names used to control the constrained endogenous variables.
%
% OUTPUTS
% - endogenous          [dseries]
% - exogenous           [dseries]
%
% REMARKS

% Copyright (C) 2019 Dynare Team
%
% This file is part of Dynare.
%
% Dynare is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
%
% Dynare is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with Dynare.  If not, see <http://www.gnu.org/licenses/>.

% Get indices for the calibrated and free innovations.
freeinnovations_id = zeros(length(freeinnovations), 1);
if length(freeinnovations)<DynareModel.exo_nbr
    for i=1:length(freeinnovations)
        freeinnovations_id(i) = find(strcmp(freeinnovations{i}, exo_names));
    end
    calibratedinnovations_id = setdiff(transpose(1:length(exo_names)), freeinnovations_id);
else
    freeinnovations_id = transpose(1:length(exo_names));
    calibratedinnovations_id = [];
end

nxfree = length(freeinnovations_id);
nxcalb = length(calibratedinnovations_id);

% Get indices for the the controlled and free endogenous variables.
controlledendogenousvariables_id = zeros(length(freeinnovations), 1);
if length(freeinnovations)<DynareModel.endo_nbr
    for i=1:length(freeinnovations)
        controlledendogenousvariables_id(i) = find(strcmp(constraints.name{i}, endo_names));
    end
    freeendogenousvariables_id = setdiff(transpose(1:length(endo_names)), controlledendogenousvariables_id);
else
    controlledendogenousvariables_id = transpose(1:length(endo_names));
    freeendogenousvariables_id = [];
end

nyfree = length(freeendogenousvariables_id);
nyctrl = length(controlledendogenousvariables_id);

% Get indices of variables appearing at time t-1.
% should be empty. iy1 = find(DynareModel.lead_lag_incidence(1,:)>0);

% Get indices of variables appearing at time t.
% all variables appear at time t in a static model. iy0 = find(DynareModel.lead_lag_incidence(2,:)>0);

% Build structure to be passed to the objective function.
ModelInversion.nyfree = nyfree;
ModelInversion.nyctrl = nyctrl;
ModelInversion.nxfree = nxfree;
ModelInversion.nxcalb = nxcalb;
ModelInversion.y_constrained_id = vec(DynareModel.lead_lag_incidence(controlledendogenousvariables_id));
ModelInversion.y_free_id = vec(DynareModel.lead_lag_incidence(freeendogenousvariables_id));
ModelInversion.x_free_id = freeinnovations_id;
ModelInversion.J_id = [ModelInversion.y_free_id ; sum(DynareModel.lead_lag_incidence(:)>0)+ModelInversion.x_free_id];

% Get the name of the dynamic model routines.
model_dynamic = str2func([DynareModel.fname,'.dynamic']);
model_stransf = str2func('static_model_for_inversion');

% Initialization of the returned simulations (endogenous variables).
Y = NaN(DynareModel.endo_nbr, nobs(constraints));

for i=1:nyctrl
    Y(controlledendogenousvariables_id(i),1:end) = transpose(constraints.data(:,i));
end

% Exogenous variables.
X = exogenousvariables.data;

% Inversion of the model, solvers for the free endogenous and exogenous variables (call a Newton-like algorithm in each period).

ity = 1;
itx = find(exogenousvariables.dates==constraints.dates(1));
DynareOptions.solve_algo=4;

for t = 1:nobs(constraints)
    % Set the current values of the constrained endogenous variables.
    ycur = Y(controlledendogenousvariables_id,ity);
    % Vector z gather the free endogenous variables (initialized with lagged
    % values) and the free exogenous variables (initialized with 0).
    z = [Y(freeendogenousvariables_id,ity); zeros(nxfree, 1)];
    % Solves for z.
    [z, failed] = dynare_solve(model_stransf, z, DynareOptions, model_dynamic, ycur, X(itx, :), DynareModel.params, ModelInversion);
    if failed
        error('Enable to solve the system of equations!')
    end
    % Update the matrix of exogenous variables.
    X(itx,freeinnovations_id) = z(nyfree+(1:nxfree));
    % Update the matrix of endogenous variables.
    if nyfree
        Y(freeendogenousvariables_id,ity) = z(1:nyfree);
    end
    % Increment counters
    ity = ity+1;
    itx = itx+1;
end

endogenousvariables = dseries(Y', constraints.dates(1), endo_names);
exogenousvariables = dseries(X(find(exogenousvariables.dates==constraints.dates(1))+(0:(nobs(constraints)-1)),:), constraints.dates(1), exo_names);