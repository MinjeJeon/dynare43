function initialize(pacmodel)

% Initialization of a PAC model.
%
% INPUTS
% - pacmodel       [string]    Name of the pac model.
%
% OUTPUTS
% None

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

if isempty(M_.pac.(pacmodel).auxiliary_model_name)
    M_.pac.(pacmodel).model_consistent_expectations = true;
else
    M_.pac.(pacmodel).model_consistent_expectations = false;
    get_companion_matrix(M_.pac.(pacmodel).auxiliary_model_name, M_.pac.(pacmodel).auxiliary_model_type);
end