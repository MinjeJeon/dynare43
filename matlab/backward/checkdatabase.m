function [dbase, info] = checkdatabase(dbase, DynareModel, inversionflag, simulationflag)

% Check that dbase contains all the endogenous variables of the model, and
% reorder the endogenous variables as declared in the mod file. If Dynare
% adds auxiliary variables, for lags greater than 1 on endogenous variables,
% endogenous variables in difference (which may be lagged), or lags on the
% exogenous variables, then thee routine complete the database.

% Copyright (C) 2018-2019 Dynare Team
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

if DynareModel.maximum_endo_lead
    error('The model (%s) is assumed to be backward!', DynareModel.fname)
end

if nargin<3
    inversionflag = false;
end

set_auxiliary_series = [DynareModel.fname '.dynamic_set_auxiliary_series'];

if exist([set_auxiliary_series '.m'])
    dbase = feval(set_auxiliary_series, dbase, DynareModel.params);
end

listoflaggedexogenousvariables = {};
if ~isempty(DynareModel.aux_vars)
    listoflaggedexogenousvariables = DynareModel.exo_names([DynareModel.aux_vars(find([DynareModel.aux_vars.type]==3)).orig_index]);
end

listoflaggedendogenousvariables = {};
laggedendogenousvariablesidx = find(DynareModel.lead_lag_incidence(1,1:DynareModel.orig_endo_nbr));
if ~isempty(laggedendogenousvariablesidx)
    listoflaggedendogenousvariables = DynareModel.endo_names(laggedendogenousvariablesidx);
end
if ~isempty(DynareModel.aux_vars)
    laggedendogenousvariablesidx = find([DynareModel.aux_vars.type]==1);
    if ~isempty(laggedendogenousvariablesidx)
        listoflaggedendogenousvariables = union(listoflaggedendogenousvariables, DynareModel.endo_names([DynareModel.aux_vars(laggedendogenousvariablesidx).orig_index]));
    end
    laggedendogenousvariablesidx = find([DynareModel.aux_vars.type]==8);
    if ~isempty(laggedendogenousvariablesidx)
        listoflaggedendogenousvariables = union(listoflaggedendogenousvariables, DynareModel.endo_names([DynareModel.aux_vars(laggedendogenousvariablesidx).orig_index]));
    end
end

info = struct;
info.endonames = DynareModel.endo_names;
info.exonames = DynareModel.exo_names;
info.computeresiduals = false;

% Check that all the endogenous variables are defined in dbase.
missingendogenousvariables = setdiff(info.endonames, dbase.name);
if ~isempty(missingendogenousvariables)
    missinglaggedendogenousvariables = intersect(missingendogenousvariables, listoflaggedendogenousvariables);
    if ~isempty(missinglaggedendogenousvariables)
        error('Endogenous variable %s is missing in the database!', missinglaggedendogenousvariables{:})
    end
end

if inversionflag
    % If some exogenous variables are missing, check that they can be interpreted as residuals.
    missingexogenousvariables = setdiff(info.exonames, dbase.name);
    if ~isempty(missingexogenousvariables)
        dprintf('%s exogenous variables are missing in the database...', num2str(length(missingexogenousvariables)))
        listofmissinglaggedexognousvariables = intersect(listoflaggedexogenousvariables, missingexogenousvariables);
        if isempty(listofmissinglaggedexognousvariables)
            info.residuals = missingexogenousvariables;
            info.computeresiduals = true;
            disp('These variables can be calibrated by calling calibrateresiduals routine.')
        else
            info.residuals = setdiff(missingexogenousvariables, listofmissinglaggedexogenousvariables);
            disp('The following exogenous variables:')
            listofmissinglaggedexognousvariables
            disp('are not residuals, and cannot be calibrated by calling calibrateresiduals.')
        end
    else
        disp('All the endogenous and exogenous variables are calibrated!')
    end
elseif simulationflag
    % Check that all the exogenous variables are defined in dbase
    missingexogenousvariables = setdiff(info.exonames, dbase.name);
    listofmissingexovarforinit = intersect(missingexogenousvariables, listoflaggedexogenousvariables);
    if ~isempty(listofmissingexovarforinit)
        error('Exogenous variable %s is missing!', listofmissingexovarforinit{:})
    end
    info.residuals = [];
end