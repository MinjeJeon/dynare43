function print_expectations(eqname, expectationmodelname, expectationmodelkind, withcalibration)

% Prints the exansion of the VAR_EXPECTATION or PAC_EXPECTATION term in files.
%
% INPUTS
% - eqname                      [string]    Name of the equation.
% - epxpectationmodelname       [string]    Name of the expectation model.
% - expectationmodelkind        [string]    Kind of the expectation model.
% - withcalibration             [logical]   Prints calibration if true.
%
% OUTPUTS
% None
%
% REMARKS
% The routine creates two text files
%
% - {expectationmodelname}-parameters.inc     which contains the declaration of the parameters specific to the expectation model kind term.
% - {expectationmodelname}-expression.inc     which contains the expanded version of the expectation model kind term.
%
% These routines are saved under the {modfilename}/model/{expectationmodelkind} subfolder, and can be
% used after in another mod file (ie included with the macro directive @#include).
%
% The variable expectationmodelkind can take two values 'var-expctations' or 'pac-expectations'.

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

global M_

if nargin<4 || isempty(withcalibration)
    withcalibration = true;
end

% Check that the first input is a row character array.
if ~isrow(eqname)==1 || ~ischar(eqname)
    error('First input argument must be a row character array.')
end

% Check that the second input is a row character array.
if ~isrow(expectationmodelname)==1 || ~ischar(expectationmodelname)
    error('Second input argument must be a row character array.')
end

% Check that the third input is a row character array.
if ~isrow(expectationmodelkind)==1 || ~ischar(expectationmodelkind)
    error('Third input argument must be a row character array.')
end

% Check that the value of the second input is correct.
if ~ismember(expectationmodelkind, {'var-expectations', 'pac-expectations'})
    error('Wrong value for the second input argument.')
end

% Check that the model exists.
switch expectationmodelkind
  case 'var-expectations'
    if ~isfield(M_.var_expectation, expectationmodelname)
        error('VAR_EXPECTATION_MODEL %s is not defined.', expectationmodelname)
    else
        expectationmodelfield = 'var_expectation';
    end
  case 'pac-expectations'
    if ~isfield(M_.pac, expectationmodelname)
        error('PAC_EXPECTATION_MODEL %s is not defined.', expectationmodelname)
    else
        expectationmodelfield = 'pac';
    end
  otherwise
end

if isequal(expectationmodelkind, 'pac-expectations')
    % Get the equation tag (in M_.pac.(pacmodl).equations)
    eqtag = M_.pac.(expectationmodelname).tag_map{strcmp(M_.pac.(expectationmodelname).tag_map(:,1), eqname),2};
end

% Get the expectation model description
expectationmodel = M_.(expectationmodelfield).(expectationmodelname);

% Get the name of the associated VAR model and test its existence.
if ~isfield(M_.(expectationmodel.auxiliary_model_type), expectationmodel.auxiliary_model_name)
    switch expectationmodelkind
      case 'var-expectations'
        error('Unknown VAR/TREND_COMPONENT model (%s) in VAR_EXPECTATION_MODEL (%s)!', expectationmodel.auxiliary_model_name, expectationmodelname)
      case 'pac-expectations'
        error('Unknown VAR/TREND_COMPONENT model (%s) in PAC_EXPECTATION_MODEL (%s)!', expectationmodel.auxiliary_model_name, expectationmodelname)
      otherwise
    end
end

auxmodel = M_.(expectationmodel.auxiliary_model_type).(expectationmodel.auxiliary_model_name);

%
% First print the list of parameters appearing in the VAR_EXPECTATION/PAC_EXPECTATION term.
%
if ~exist(sprintf('%s/model/%s', M_.fname, expectationmodelkind), 'dir')
    mkdir(sprintf('%s/model/%s', M_.fname, expectationmodelkind))
end

if isequal(expectationmodelkind, 'pac-expectations')
    filename = sprintf('%s/model/%s/%s-%s-parameters.inc', M_.fname, expectationmodelkind, eqtag, expectationmodelname);
else
    filename = sprintf('%s/model/%s/%s-parameters.inc', M_.fname, expectationmodelkind, expectationmodelname);
end
fid = fopen(filename, 'w');
fprintf(fid, '// This file has been generated by dynare (%s).\n\n', datestr(now));

switch expectationmodelkind
  case 'var-expectations'
    parameter_declaration = 'parameters';
    for i=1:length(expectationmodel.param_indices)
        parameter_declaration = sprintf('%s %s', parameter_declaration, M_.param_names{expectationmodel.param_indices(i)});
    end
    fprintf(fid, '%s;\n\n', parameter_declaration);
    if withcalibration
        for i=1:length(expectationmodel.param_indices)
            fprintf(fid, '%s = %s;\n', M_.param_names{expectationmodel.param_indices(i)}, num2str(M_.params(expectationmodel.param_indices(i)), 16));
        end
    end
  case 'pac-expectations'
    if ~isempty(expectationmodel.equations.(eqtag).h0_param_indices)
        parameter_declaration = 'parameters';
        for i=1:length(expectationmodel.equations.(eqtag).h0_param_indices)
            parameter_declaration = sprintf('%s %s', parameter_declaration, M_.param_names{expectationmodel.equations.(eqtag).h0_param_indices(i)});
        end
        fprintf(fid, '%s;\n\n', parameter_declaration);
        if withcalibration
            for i=1:length(expectationmodel.equations.(eqtag).h0_param_indices)
                fprintf(fid, '%s = %s;\n', M_.param_names{expectationmodel.equations.(eqtag).h0_param_indices(i)}, num2str(M_.params(expectationmodel.equations.(eqtag).h0_param_indices(i)), 16));
            end
        end
    end
    if ~isempty(expectationmodel.equations.(eqtag).h1_param_indices)
        parameter_declaration = 'parameters';
        for i=1:length(expectationmodel.equations.(eqtag).h1_param_indices)
            parameter_declaration = sprintf('%s %s', parameter_declaration, M_.param_names{expectationmodel.equations.(eqtag).h1_param_indices(i)});
        end
        fprintf(fid, '%s;\n\n', parameter_declaration);
        if withcalibration
            for i=1:length(expectationmodel.equations.(eqtag).h1_param_indices)
                fprintf(fid, '%s = %s;\n', M_.param_names{expectationmodel.equations.(eqtag).h1_param_indices(i)}, num2str(M_.params(expectationmodel.equations.(eqtag).h1_param_indices(i)), 16));
            end
        end
    end
    if isfield(expectationmodel, 'growth_neutrality_param_index')
        fprintf(fid, '\n');
        fprintf(fid, 'parameters %s;\n\n', M_.param_names{expectationmodel.growth_neutrality_param_index});
        if withcalibration
            fprintf(fid, '%s = %s;\n', M_.param_names{expectationmodel.growth_neutrality_param_index}, num2str(M_.params(expectationmodel.growth_neutrality_param_index), 16));
        end
        growth_correction = true;
    else
        growth_correction = false;
    end
  otherwise
end

fclose(fid);

fprintf('Parameters declarations and calibrations are saved in %s.\n', filename);

%
% Second print the expanded VAR_EXPECTATION/PAC_EXPECTATION term.
%

if isequal(expectationmodelkind, 'pac-expectations')
    filename = sprintf('%s/model/%s/%s-%s-expression.inc', M_.fname, expectationmodelkind, eqtag, expectationmodelname);
else
    filename = sprintf('%s/model/%s/%s-expression.inc', M_.fname, expectationmodelkind, expectationmodelname);
end
fid = fopen(filename, 'w');
fprintf(fid, '// This file has been generated by dynare (%s).\n', datestr(now));

id = 0;

maxlag = max(auxmodel.max_lag);
if isequal(expectationmodel.auxiliary_model_type, 'trend_component')
    % Need to add a lag since the error correction equations are rewritten in levels.
    maxlag = maxlag+1;
end

for i=1:maxlag
    for j=1:length(auxmodel.list_of_variables_in_companion_var)
        id = id+1;
        variable = auxmodel.list_of_variables_in_companion_var{j};
        transformations = {};
        ida = get_aux_variable_id(variable);
        op = 0;
        while ida
            op = op+1;
            if isequal(M_.aux_vars(ida).type, 8)
                transformations(op) = {'diff'};
                variable = M_.endo_names{M_.aux_vars(ida).orig_index};
                ida = get_aux_variable_id(variable);
            elseif isequal(M_.aux_vars(ida).type, 10)
                transformations(op) = {M_.aux_vars(ida).unary_op};
                variable = M_.endo_names{M_.aux_vars(ida).orig_index};
                ida = get_aux_variable_id(variable);
            else
                error('This case is not implemented.')
            end
        end
        switch expectationmodelkind
          case 'var-expectations'
            parameter = M_.param_names{expectationmodel.param_indices(id)};
          case 'pac-expectations'
            parameter = '';
            if ~isempty(expectationmodel.equations.(eqtag).h0_param_indices)
                parameter = M_.param_names{expectationmodel.equations.(eqtag).h0_param_indices(id)};
            end
            if ~isempty(expectationmodel.equations.(eqtag).h1_param_indices)
                if isempty(parameter)
                    parameter = M_.param_names{expectationmodel.equations.(eqtag).h1_param_indices(id)};
                else
                    parameter = sprintf('(%s+%s)', parameter, M_.param_names{expectationmodel.equations.(eqtag).h1_param_indices(id)});
                end
            end
          otherwise
        end
        switch expectationmodelkind
          case 'var-expectations'
            if i>1
                variable = sprintf('%s(-%d)', variable, i-1);
            end
          case 'pac-expectations'
            variable = sprintf('%s(-%d)', variable, i);
          otherwise
        end
        if ~isempty(transformations)
            for k=length(transformations):-1:1
                variable = sprintf('%s(%s)', transformations{k}, variable);
            end
        end
        if isequal(id, 1)
            expression = sprintf('%s*%s', parameter, variable);
        else
            expression = sprintf('%s + %s*%s', expression, parameter, variable);
        end
    end
end

fprintf(fid, '%s', expression);
fclose(fid);

fprintf('Expectation unrolled expression is saved in %s.\n', filename);

%
% Second bis print the PAC growth neutrality correction term (if any).
%

if isequal(expectationmodelkind, 'pac-expectations') && growth_correction
    filename = sprintf('%s/model/%s/%s-%s-growth-neutrality-correction.inc', M_.fname, expectationmodelkind, eqtag, expectationmodelname);
    fid = fopen(filename, 'w');
    fprintf(fid, '// This file has been generated by dynare (%s).\n', datestr(now));
    pgrowth = M_.param_names{expectationmodel.growth_neutrality_param_index};
    switch expectationmodel.growth_type
      case 'parameter'
        vgrowth = M_.param_names{expectationmodel.growth_index};
      case 'endogenous'
        vgrowth = M_.endo_names{expectationmodel.growth_index};
        lgrowth = expectationmodel.growth_lag;
      case 'exogenous'
        if expectationmodel.growth_index<=M_.exo_nbr
            vgrowth = M_.exo_names{expectationmodel.growth_index};
        else
            vgrowth = M_.endo_names{expectationmodel.growth_index};
        end
        lgrowth = expectationmodel.growth_lag;
      otherwise
    end
    vgrowth = rewritegrowthvariable(vgrowth, lgrowth, M_);
    fprintf(fid, '%s*%s', pgrowth, vgrowth);
    fclose(fid);
    fprintf('Growth neutrality correction is saved in %s.\n', filename);
end

%
% Third print a routine for evaluating VAR_EXPECTATION/PAC_EXPECTATION term (returns a dseries object).
%
kind = strrep(expectationmodelkind, '-', '_');
mkdir(sprintf('+%s/+%s/+%s', M_.fname, kind, expectationmodelname));
if isequal(expectationmodelkind, 'pac-expectations')
    filename = sprintf('+%s/+%s/+%s/%s_evaluate.m', M_.fname, kind, expectationmodelname, eqtag);
else
    filename = sprintf('+%s/+%s/+%s/evaluate.m', M_.fname, kind, expectationmodelname);
end
fid = fopen(filename, 'w');
if isequal(expectationmodelkind, 'pac-expectations')
    fprintf(fid, 'function ds = %s_evaluate(dbase)\n\n', eqtag);
else
    fprintf(fid, 'function ds = evaluate(dbase)\n\n');
end
if isequal(expectationmodelkind, 'pac-expectations')
    fprintf(fid, '%% Evaluates %s term (%s in %s).\n', kind, expectationmodelname, eqname);
else
    fprintf(fid, '%% Evaluates %s term (%s).\n', kind, expectationmodelname);
end
fprintf(fid, '%%\n');
fprintf(fid, '%% INPUTS\n');
fprintf(fid, '%% - dbase     [dseries]  databse containing all the variables appearing in the auxiliary model for the expectation.\n');
fprintf(fid, '%%\n');
fprintf(fid, '%% OUTPUTS\n');
fprintf(fid, '%% - ds        [dseries]  the expectation term .\n');
fprintf(fid, '%%\n');
fprintf(fid, '%% REMARKS\n');
fprintf(fid, '%% The name of the appended variable in dbase is the declared name for the (PAC/VAR) expectation model.\n\n');
fprintf(fid, '%% This file has been generated by dynare (%s).\n\n', datestr(now));
fprintf(fid, 'ds = dseries();\n\n');

id = 0;

maxlag = max(auxmodel.max_lag);
if isequal(expectationmodel.auxiliary_model_type, 'trend_component')
    % Need to add a lag since the error correction equations are rewritten in levels.
    maxlag = maxlag+1;
end

for i=1:maxlag
    for j=1:length(auxmodel.list_of_variables_in_companion_var)
        id = id+1;
        variable = auxmodel.list_of_variables_in_companion_var{j};
        transformations = {};
        ida = get_aux_variable_id(variable);
        op = 0;
        while ida
            op = op+1;
            if isequal(M_.aux_vars(ida).type, 8)
                transformations(op) = {'diff'};
                variable = M_.endo_names{M_.aux_vars(ida).orig_index};
                ida = get_aux_variable_id(variable);
            elseif isequal(M_.aux_vars(ida).type, 10)
                transformations(op) = {M_.aux_vars(ida).unary_op};
                variable = M_.endo_names{M_.aux_vars(ida).orig_index};
                ida = get_aux_variable_id(variable);
            else
                error('This case is not implemented.')
            end
        end
        switch expectationmodelkind
          case 'var-expectations'
            parameter = M_.params(expectationmodel.param_indices(id));
          case 'pac-expectations'
            parameter = 0;
            if ~isempty(expectationmodel.equations.(eqtag).h0_param_indices)
                parameter = M_.params(expectationmodel.equations.(eqtag).h0_param_indices(id));
            end
            if ~isempty(expectationmodel.equations.(eqtag).h1_param_indices)
                if ~parameter
                    parameter = M_.params(expectationmodel.equations.(eqtag).h1_param_indices(id));
                else
                    parameter = parameter+M_.params(expectationmodel.equations.(eqtag).h1_param_indices(id));
                end
            end
          otherwise
        end
        switch expectationmodelkind
          case 'var-expectations'
            if i>1
                variable = sprintf('dbase.%s(-%d)', variable, i-1);
            else
                variable = sprintf('dbase.%s', variable);
            end
          case 'pac-expectations'
            variable = sprintf('dbase.%s(-%d)', variable, i);
          otherwise
        end
        if ~isempty(transformations)
            for k=length(transformations):-1:1
                variable = sprintf('%s.%s()', variable, transformations{k});
            end
        end
        if isequal(id, 1)
            if isequal(expectationmodelkind, 'pac-expectations') && growth_correction
                pgrowth = M_.params(expectationmodel.growth_neutrality_param_index);
                switch expectationmodel.growth_type
                  case 'parameter'
                    vgrowth = M_.param_names{expectationmodel.growth_index};
                  case 'endogenous'
                    vgrowth = M_.endo_names{expectationmodel.growth_index};
                    lgrowth = expectationmodel.growth_lag;
                  case 'exogenous'
                    if expectationmodel.growth_index<=M_.exo_nbr
                        vgrowth = M_.exo_names{expectationmodel.growth_index};
                    else
                        vgrowth = M_.endo_names{expectationmodel.growth_index};
                    end
                    lgrowth = expectationmodel.growth_lag;
                  otherwise
                end
                vgrowth = rewritegrowthvariable(vgrowth, lgrowth, M_);
                if parameter>=0
                    expression = sprintf('%s*%s+%s*%s', num2str(pgrowth, '%1.16f'), vgrowth, num2str(parameter, '%1.16f'), variable);
                else
                    expression = sprintf('%s*%s-%s*%s', num2str(pgrowth, '%1.16f'), vgrowth, num2str(-parameter, '%1.16f'), variable);
                end
            else
                expression = sprintf('%s*%s', num2str(parameter, '%1.16f'), variable);
            end
        else
            if parameter>=0
                expression = sprintf('%s + %s*%s', expression, num2str(parameter, '%1.16f'), variable);
            else
                expression = sprintf('%s - %s*%s', expression, num2str(-parameter, '%1.16f'), variable);
            end
        end
    end
end

fprintf(fid, 'ds.%s = %s;', expectationmodelname, expression);
fclose(fid);

fprintf('Expectation dseries expression is saved in %s.\n', filename);

skipline();

function vgrowth = rewritegrowthvariable(vgrowth, lgrowth, M_)
    if isauxiliary(vgrowth)
        % We need to rewrite vgrowth in terms of the original (exogenous or endogenous) variable.
        auxinfo = M_.aux_vars(get_aux_variable_id(vgrowth));
        switch auxinfo.type
          case 1
            tmp = get_aux_variable_id(auxinfo.orig_index);
            % Lagged endogenous.
            if ~isauxiliary(auxinfo.orig_index)
                v = M_.endo_names{auxinfo.orig_index};
                s = auxinfo.orig_lead_lag;
                d = 0;
                t = [];
            elseif tmp.type==10
                t = tmp.unary_op;
                s = auxinfo.orig_lead_lag;
                d = 0;
                v = M_.endo_names{tmp.orig_index};
            else
                error('Auxiliary variable has wrong type (1).')
            end
          case 3
            % Lagged exogenous.
            v = M_.exo_names{auxinfo.orig_index};
            s = auxinfo.orig_lead_lag;
            d = 0;
            t = [];
          case 8
            tmp = get_aux_variable_id(auxinfo.orig_index);
            % First difference.
            if ~isauxiliary(auxinfo.orig_index)
                v = M_.endo_names{auxinfo.orig_index};
                s = 0;
                d = 1;
                t = [];
            elseif tmp.type==10
                t = tmp.unary_op;
                s = 0;
                d = 1;
                v = M_.endo_names{tmp.orig_index};
            else
                error('Auxiliary variable has wrong type (8).')
            end
          case 9
            % Lagged first difference
            s = 0;
            while auxinfo.type==9
                s = s+1;
                auxinfo = M_.aux_vars(get_aux_variable_id(auxinfo.orig_index));
            end
            if auxinfo.type==8
                if isauxiliary(auxinfo.orig_index)
                    % First difference of an auxiliary variable.
                    tmp = get_aux_variable_id(auxinfo.orig_index);
                    if tmp.type==10
                        t = tmp.unary_op;
                        d = 1;
                        v = M_.endo_names{tmp.orig_index};
                    else
                        error('Auxiliary variable has wrong type (9).')
                    end
                else
                    % First difference of a declared endogenous variable.
                    t = [];
                    d = 1;
                    v = M_.endo_names{auxinfo.orig_index};
                end
            else
                error('Auxiliary variable has wrong type (9).')
            end
          otherwise
            error('Auxiliary variable has wrong type.')
        end
        s = s+lgrowth;
        if s
            vgrowth = sprintf('%s(-%u)', v, abs(s));
        else
            vgrowth = v;
        end
        if ~isempty(t)
            vgrowth = sprintf('%s(%s)', t, vgrowth);
        end
        if d
            vgrowth = sprintf('%s(%s)', 'diff', vgrowth);
        end
    end