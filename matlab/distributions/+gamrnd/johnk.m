function g = johnk(a, b)

% Returns gamma variates, see Devroye (1986) page 418.
%
% INPUTS
% - a    [double]     n*1 vector, first hyperparameter.
% - b    [double]     n*1 vector, second hyperparameter.
%
% OUTPUTS
% - g    [double]     n*1 vector, gamma variates.

% Copyright (C) 2006-2018 Dynare Team
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

nn = length(a);
mm = nn;
aa = 1./a;
bb = 1./(1-a);
INDEX = 1:mm;
index = INDEX;
UV = NaN(nn,2);
X  = NaN(nn,1);
Y  = NaN(nn,1);

while mm
    UV(index,:) = rand(mm,2);
    X(index) = UV(index,1).^aa(index);
    Y(index) = UV(index,2).^bb(index);
    id = find(X+Y>1);
    if isempty(id)
        mm = 0;
    else
        index = INDEX(id);
        mm = length(index);
    end
end

g = (exprnd(ones(nn,1)).*(X./(X+Y))).*b;