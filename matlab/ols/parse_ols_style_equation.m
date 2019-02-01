function [Y, lhssub, X, residual, fp, lp] = parse_ols_style_equation(ds, ast)
%function [Y, lhssub, X, residual, fp, lp] = parse_ols_style_equation(ds, ast)
% Run OLS on chosen model equations; unlike olseqs, allow for time t
% endogenous variables on LHS
%
% INPUTS
%   ds          [dseries]     data
%   ast         [struct]      AST representing the equation to be parsed
%
% OUTPUTS
%   Y           [dseries]     LHS of the equation (with lhssub subtracted)
%   lhssub      [dseries]     RHS subtracted from LHS
%   X           [dseries]     RHS of the equation
%   residual    [string]      name of residual in equation
%   fp          [date]        first common observed period between Y, lhssub, and X
%   lp          [date]        last common observed period between Y, lhssub, and X
%
% SPECIAL REQUIREMENTS
%   none

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

%% Check inputs
if nargin ~= 2
    error('parse_ols_style_equation takes 2 arguments')
end

if isempty(ds) || ~isdseries(ds)
    error('parse_ols_style_equation: arg 1 must be a dseries');
end

if isempty(ast) || ~isstruct(ast)
    error('parse_ols_style_equation: arg 2 must be a struct');
end

line = ast.line;
if ~strcmp(ast.AST.node_type, 'BinaryOpNode') ...
        || ~strcmp(ast.AST.op, '=')
    parsing_error('expecting equation with equal sign', line);
end

% Check LHS
if ~strcmp(ast.AST.arg1.node_type, 'VariableNode') ...
        && ~strcmp(ast.AST.arg1.node_type, 'UnaryOpNode')
    parsing_error('expecting Variable or UnaryOp on LHS', line);
else
    if ~isOlsVar(ds, ast.AST.arg1) || ~isBaseVarLagEqualToZero(ast.AST.arg1)
        parsing_error('the LHS of the equation must be an Variable or UnaryOp with lag == 0 that exists in the dataset', line);
    end
end

%% Set LHS (Y)
lhssub = dseries();
Y = evalNode(ds, ast.AST.arg1, line, dseries());

%% Set RHS (X)
plus_node = ast.AST.arg2;
last_node_to_parse = [];
residual = '';
X = dseries();
while ~isempty(plus_node) || ~isempty(last_node_to_parse)
    Xtmp = dseries();
    if isempty(last_node_to_parse)
        [plus_node, node_to_parse, last_node_to_parse] = findNextplus_node(plus_node, line);
    else
        node_to_parse = last_node_to_parse;
        last_node_to_parse = [];
    end
    if strcmp(node_to_parse.node_type, 'VariableNode')
        if strcmp(node_to_parse.type, 'parameter')
            % Intercept
            Xtmp = dseries(1, ds.dates, node_to_parse.name);
        elseif strcmp(node_to_parse.type, 'exogenous') && ~any(strcmp(ds.name, node_to_parse.name))
            % Residual if not contained in ds
            if isempty(residual)
                residual = node_to_parse.name;
            else
                parsing_error(['only one residual allowed per equation; encountered ' residual ' & ' node_to_parse.name], line);
            end
        elseif strcmp(node_to_parse.type, 'endogenous') ...
                || (strcmp(node_to_parse.type, 'exogenous') && any(strcmp(ds.name, node_to_parse.name)))
            % Subtract VariableNode from LHS
            % NB: treat exogenous that exist in ds as endogenous
            lhssub = lhssub + evalNode(ds, node_to_parse, line, dseries());
        else
            parsing_error('unexpected variable type found', line);
        end
    elseif strcmp(node_to_parse.node_type, 'UnaryOpNode')
        % Subtract UnaryOpNode from LHS
        % NB: treat exogenous that exist in ds as endogenous
        lhssub = lhssub + evalNode(ds, node_to_parse, line, dseries());
    elseif strcmp(node_to_parse.node_type, 'BinaryOpNode') && strcmp(node_to_parse.op, '*')
        % Parse param_expr * endog_expr
        Xtmp = parseTimesNode(ds, node_to_parse, line);
        if Xtmp.vobs > 1 || ...
                (Xtmp.vobs == 1 && ~isnan(str2double(Xtmp.name)))
            % Handle constraits
            % Look through Xtmp names for constant
            % if found, subtract from LHS
            names = Xtmp.name;
            for j = length(names):-1:1
                if ~isnan(str2double(names{j}))
                    lhssub = lhssub + str2double(names{j}) * Xtmp.(names{j});
                    Xtmp = Xtmp.remove(names{j});
                else
                    % Multiply by -1 now so that it can be added together below
                    % Otherwise, it would matter which was encountered first,
                    % a parameter on its own or a linear constraint
                    Xtmp.(names{j}) = -1 * Xtmp.(names{j});
                end
            end
        end
    else
        parsing_error('didn''t expect to arrive here', line);
    end

    names = Xtmp.name;
    for j = length(names):-1:1
        % Handle constraits
        idx = find(strcmp(X.name, names{j}));
        if ~isempty(idx)
            X.(X.name{idx}) = X{idx} + Xtmp{j};
            Xtmp = Xtmp.remove(names{j});
        end
    end
    X = [X Xtmp];
end
Y = Y - lhssub;

%% Set start and end dates
fp = Y.firstobservedperiod;
lp = Y.lastobservedperiod;
if ~isempty(X)
    % X is empty when AR(1) without parameter is encountered
    fp = max(fp, X.firstobservedperiod);
    lp = min(lp, X.lastobservedperiod);
end
if ~isempty(lhssub)
    fp = max(fp, lhssub.firstobservedperiod);
    lp = min(lp, lhssub.lastobservedperiod);
end

% If it exists, account for tag set in mod file
if isfield(ast, 'tags') ...
        && isfield(ast.tags, 'sample') ...
        && ~isempty(ast.tags.sample)
    colon_idx = strfind(ast.tags.sample, ':');
    fsd = dates(ast.tags.sample(1:colon_idx-1));
    lsd = dates(ast.tags.sample(colon_idx+1:end));
    if fp > fsd
        warning(['The sample over which you want to estimate contains NaNs. '...
            'Adjusting estimation range to begin on: ' fp.char])
    else
        fp = fsd;
    end
    if lp < lsd
        warning(['The sample over which you want to estimate contains NaNs. '...
            'Adjusting estimation range to end on: ' lp.char])
    else
        lp = lsd;
    end
end

Y = Y(fp:lp);
if ~isempty(X)
    X = X(fp:lp);
end
if ~isempty(lhssub)
    lhssub = lhssub(fp:lp);
end
end

%% Helper Functions
function parsing_error(msg, line)
error(['ERROR encountered parsing of equation on line ' num2str(line) ': ' msg])
end

function [next_plus_node, node_to_parse, last_node_to_parse] = findNextplus_node(plus_node, line)
% Given an additive entry in the AST, find the next additive entry
% (next_plus_node). Also find the node that will be parsed into
% parameter*endogenous||param||exog|endog (node_to_parse).
% Function used for moving through the AST.
if ~(strcmp(plus_node.node_type, 'BinaryOpNode') && strcmp(plus_node.op, '+'))
    parsing_error('pairs of nodes must be separated additively', line);
end
next_plus_node = [];
last_node_to_parse = [];
if strcmp(plus_node.arg1.node_type, 'BinaryOpNode') && strcmp(plus_node.arg1.op, '+')
    next_plus_node = plus_node.arg1;
    node_to_parse = getOlsNode(plus_node.arg2, line);
elseif strcmp(plus_node.arg2.node_type, 'BinaryOpNode') && strcmp(plus_node.arg2.op, '+')
    next_plus_node = plus_node.arg2;
    node_to_parse = getOlsNode(plus_node.arg1, line);
else
    node_to_parse = getOlsNode(plus_node.arg1, line);
    last_node_to_parse = getOlsNode(plus_node.arg2, line);
end
end

function node_to_parse = getOlsNode(node, line)
if ~(strcmp(node.node_type, 'BinaryOpNode') && strcmp(node.op, '*')) ...
        && ~strcmp(node.node_type, 'VariableNode') ...
        && ~strcmp(node.node_type, 'UnaryOpNode')
    parsing_error('couldn''t find node to parse', line);
end
node_to_parse = node;
end

function X = parseTimesNode(ds, node, line)
% Separate the parameter expression from the endogenous expression
assert(strcmp(node.node_type, 'BinaryOpNode') && strcmp(node.op, '*'))
[param, X] = parseTimesNodeHelper(ds, node.arg1, line, {}, dseries());
[param, X] = parseTimesNodeHelper(ds, node.arg2, line, param, X);
X = X.rename(param{1});
for ii = 2:length(param)
    X = [X dseries(X{1}.data, X{1}.firstdate, param{ii})];
end
end

function [param, X] = parseTimesNodeHelper(ds, node, line, param, X)
if isOlsParamExpr(node, line)
    param = assignParam(param, node, line);
elseif isOlsVarExpr(ds, node, line)
    if isempty(X)
        X = evalNode(ds, node, line, X);
    else
        parsing_error(['got endog * endog' node.name ' (' node.type ')'], line);
    end
else
    parsing_error('unexpected expression', line);
end
end

function param = assignParam(param, node, line)
if ~isempty(param)
    parsing_error(['got param * param' node.name ' (' node.type ')'], line);
end
param = assignParamHelper(param, node, line);
end

function param = assignParamHelper(param, node, line)
if strcmp(node.node_type, 'NumConstNode')
    param{end+1} = num2str(node.value);
elseif strcmp(node.node_type, 'VariableNode')
    param{end+1} = node.name;
elseif strcmp(node.node_type, 'BinaryOpNode')
    if ~strcmp(node.op, '-')
        parsing_error(['got unexpected parameter op ' node.op], line);
    end
    param = assignParamHelper(param, node.arg1, line);
    param = assignParamHelper(param, node.arg2, line);
else
    parsing_error(['got unexpected node (' node.type ')'], line);
end
end

function tf = isBaseVarLagEqualToZero(node)
if strcmp(node.node_type, 'VariableNode')
    tf = node.lag == 0;
elseif strcmp(node.node_type, 'UnaryOpNode')
    tf = isBaseVarLagEqualToZero(node.arg);
else
    tf = false;
end
end

function tf = isOlsVar(ds, node)
if strcmp(node.node_type, 'VariableNode') ...
        && (strcmp(node.type, 'endogenous') ...
        || (strcmp(node.type, 'exogenous') && any(strcmp(ds.name, node.name))))
    tf = true;
elseif strcmp(node.node_type, 'UnaryOpNode')
    tf = isOlsVar(ds, node.arg);
else
    tf = false;
end
end

function tf = isOlsVarExpr(ds, node, line)
if strcmp(node.node_type, 'VariableNode') || strcmp(node.node_type, 'UnaryOpNode')
    tf = isOlsVar(ds, node);
elseif strcmp(node.node_type, 'BinaryOpNode')
    tf = isOlsVarExpr(ds, node.arg1, line) || isOlsVarExpr(ds, node.arg2, line);
else
    parsing_error(['got unexpected type ' node.node_type], line);
end
end

function X = evalNode(ds, node, line, X)
if strcmp(node.node_type, 'NumConstNode')
    X = dseries(node.value, ds.dates, 'const');
elseif strcmp(node.node_type, 'VariableNode')
    if ~(strcmp(node.type, 'endogenous') ...
            || (strcmp(node.type, 'exogenous') && any(strcmp(ds.name, node.name))))
        parsing_error(['got unexpected type ' node.name ': ' node.type], line);
    end
    X = ds.(node.name)(node.lag);
elseif strcmp(node.node_type, 'UnaryOpNode')
    Xtmp = evalNode(ds, node.arg, line, X);
    % Only works if dseries supports . notation for unary op (true for log/diff)
    % Otherwise, use: X = eval([node.op '(Xtmp)']);
    X = Xtmp.(node.op);
elseif strcmp(node.node_type, 'BinaryOpNode')
    Xtmp1 = evalNode(ds, node.arg1, line, X);
    Xtmp2 = evalNode(ds, node.arg2, line, X);
    X = X + eval(['Xtmp1 ' node.op ' Xtmp2']);
else
    parsing_error(['got unexpected node type ' node.node_type], line);
end
end

function tf = isOlsParam(node)
if strcmp(node.node_type, 'VariableNode') && strcmp(node.type, 'parameter')
    tf = true;
else
    tf = false;
end
end

function tf = isOlsParamExpr(node, line)
if strcmp(node.node_type, 'NumConstNode')
    tf = true;
elseif strcmp(node.node_type, 'VariableNode')
    tf = isOlsParam(node);
elseif strcmp(node.node_type, 'UnaryOpNode')
    tf = false;
elseif strcmp(node.node_type, 'BinaryOpNode')
    tf = isOlsParamExpr(node.arg1) && isOlsParamExpr(node.arg2);
    if tf && ~strcmp(node.op, '-')
        parsing_error(['got unexpected op ' node.op], line);
    end
else
    parsing_error(['got unexpected type ' node.node_type], line);
end
end
