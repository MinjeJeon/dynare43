function save_results(x,s_name,names)

% function save_results(x,s_name,names)
% save results in appropriate structure
%
% INPUT
%   x: matrix to be saved column by column
%   s_name: name of the structure where to save the results
%   names: names of the individual series
%
% OUTPUT
%   none
%
% SPECIAL REQUIREMENT
%   none

% Copyright (C) 2006-2009 Dynare Team
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
% along with Dynare.  If not, see <https://www.gnu.org/licenses/>.

global oo_

for i=1:size(x,2)
    eval([s_name deblank(names(i,:)) '= x(:,i);']);
end