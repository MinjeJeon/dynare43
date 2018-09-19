function initialize(pacmodel)

% Initialization of a PAC model.
%
% INPUTS
% - pacmodel       [string]    Name of the pac model.
%
% OUTPUTS
% None

% Copyright (C) 2018 Dynare Team
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

auxiliary_model_name = M_.pac.(pacmodel).auxiliary_model_name;

if isfield(M_, 'var') && isfield(M_.var, auxiliary_model_name)
    auxiliary_model_type = 'var';
elseif isfield(M_, 'trend_component') && isfield(M_.trend_component, auxiliary_model_name)
    auxiliary_model_type = 'trend_component';
else
    error('Unknown type of auxiliary model.')
end

M_.pac.(pacmodel).auxiliary_model_type = auxiliary_model_type;

get_companion_matrix_legacy(auxiliary_model_name, auxiliary_model_type);