function plot_contributions(equationname, ds1, ds0)

% Plots the contribution to the lhs variable of the rhs variables in an equation.
%
% INPUTS
%  - equationname      [string]                 Name of an equation.
%  - ds1               [string, dseries]        Object containing all the variables (exogenous and endogenous)
%                                               appearing in the equation, or the name of the dseries object.
%  - ds0               [string, dseries]        Object containing the baseline for all the variables (exogenous
%                                               and endogenous) appearing in the equation, or the name of the
%                                               dseries object.
%
% OUTPUTS
%  none
%
% SPECIAL REQUIREMENTS
%  The user must have attached names to the equations using equation
%  tags. Each equation in the model block must be preceeded with a
%  tag (see the reference manual). For instance, we should have
%  something as:
%
%      [name='Phillips curve']
%      pi = beta*pi(1) + slope*y + lam;

% Copyright (C) 2017 Dynare Team
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

jsonfile = [M_.fname '_original.json'];
if exist(jsonfile, 'file') ~= 2
    error('Could not find %s! Please use the json option (See the Dynare invocation section in the reference manual).', jsonfile);
end

% Check the number of input arguments.
if nargin>3
    error('plot_contributions:: Exactly three arguments are required!')
end

% Check the type of the first argument
if ~ischar(equationname)
    error('First argument must be a string.')
end

% Check that the equation name is actually the name of an equation in the model.
if ~ismember(equationname, M_.equations_tags(strmatch('name', M_.equations_tags(:,2)),3))
    error('plot_contributions:: There is no equation named as %s!', equationname);
end

% Check second argument
if ischar(ds1)
    if ismember(ds1, evalin('caller','who'))
        ds = evalin('caller', ds1);
        if isdseries(ds)
            ds1 = copy(ds); clear ds;
        else
            error('plot_contributions:: %s is not a dseries object!', ds1)
        end
    else
        error('plot_contributions:: %s is unknown!', ds1)
    end
else
    if ~isdseries(ds1)
        error('plot_contributions:: Second input argument must be a dseries object!')
    end
end

% Check third argument
if ischar(ds0)
    if ismember(ds0, evalin('caller','who'))
        ds = evalin('caller', ds0);
        if isdseries(ds)
            ds0 = copy(ds); clear ds;
        else
            error('plot_contributions:: %s is not a dseries object!', ds0)
        end
    else
        error('plot_contributions:: %s is unknown!', ds0)
    end
else
    if ~isdseries(ds0)
        error('plot_contributions:: Third input argument must be a dseries object!')
    end
end

% Get equation.
jsonmodel = loadjson(jsonfile);
jsonmodel = jsonmodel.model;
[lhs, rhs, ~] = getEquationsByTags(jsonmodel, 'name', equationname);
lhs = lhs{:};
rhs = rhs{:};

% Get variable and parameter names in the equation.
rhs_ = strsplit(rhs,{'+','-','*','/','^','log(','exp(','(',')'});
rhs_(cellfun(@(x) all(isstrprop(x, 'digit')), rhs_)) = []; % Remove numbers
pnames = cellstr(M_.param_names);
vnames = setdiff(rhs_, pnames);
pnames = setdiff(rhs_, vnames);

regexprnoleads = cell2mat(strcat('(', vnames, {'\(\d+\))|'}));
if ~isempty(regexp(rhs, regexprnoleads(1:end-1), 'match'))
    error(['plot_contributions: you cannot have leads in equation on line ' lineno ': ' lhs ' = ' rhs]);
end

% Get values for the parameters
idp = strmatch(pnames{1}, M_.param_names, 'exact');
str = sprintf('%s = M_.params(%d);', pnames{1}, idp);
for i=2:length(pnames)
    idp = strmatch(pnames{i}, M_.param_names, 'exact');
    str = sprintf('%s %s = M_.params(%d);', str, pnames{i}, idp);
end
eval(str)

% Replace variables with ds.variablename
for i = 1:length(vnames)
    if ismember(vnames{i}, ds1.name) && ismember(vnames{i}, ds0.name)
        regularexpression = ['(([\s\+\-\*\/\^\)])|^)(' vnames{i} ')(([\s\(\+\-\*\/\^])|$)'];
        newstring = ['$1ds.' vnames{i} '$3'];
        rhs = regexprep(rhs, regularexpression, newstring);
    else
        if ismember(vnames{i}, ds1.name)
            error('Variable %s is not available in the second dseries (baseline paths)!', vnames{i})
        else
            error('Variable %s is not available in the first dseries (actual paths)!', vnames{i})
        end
    end
end

% Initialize an array for the contributions.
contribution = zeros(ds1.nobs, ds1.vobs + 1);

% Evaluate RHS with all the actual paths.
ds = ds1;
rhseval = eval(rhs);
contribution(:, 1) = rhseval.data;

% Evaluate RHS with the baseline paths.
ds = ds0;
rhs0 = eval(rhs);

% Compute the marginal effect of each variable on the RHS, by evaluating the
% RHS with all variables at the Baseline paths except one for which the
% actual path is used.
for i = 1:length(vnames)
    ds = ds0; % Set all variable to Baseline paths.
    ds{vnames{i}} = ds1{vnames{i}};
    rhsval = eval(rhs)-rhs0;
    contribution(:, i+1) = rhsval.data;
end

% Create the contributions plot.
figure('Name', lhs);
hold on
cc = contribution(:,2:end);
ccneg = cc; ccneg(cc>=0) = nan;
ccpos = cc; ccpos(cc<0) = nan;
bar(1:ds.nobs, ccneg,'stack');
bar(1:ds.nobs, ccpos,'stack');
plot(1:ds.nobs, contribution(:,1), '-k', 'linewidth', 3);
hold off
title(sprintf('Decomposition of %s', lhs))
vnames = strrep(vnames,'_','\_');
legend(vnames{:});