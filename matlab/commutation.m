function k = commutation(n, m, sparseflag)
% k = commutation(n, m, sparseflag)
% -------------------------------------------------------------------------
% Returns Magnus and Neudecker's commutation matrix of dimensions n by m, 
% that solves k*vec(X)=vec(X')
% =========================================================================
% INPUTS
%   n:          [integer] row number of original matrix
%   m:          [integer] column number of original matrix
%   sparseflag: [integer] whether to use sparse matrices (=1) or not (else)
% -------------------------------------------------------------------------
% OUTPUTS
%   k:          [n by m] commutation matrix 
% -------------------------------------------------------------------------
% This function is called by 
%   * get_perturbation_params_derivs.m (previously getH.m)
%   * get_identification_jacobians.m (previously getJJ.m)
% -------------------------------------------------------------------------
% This function calls
%   * vec (embedded)
% =========================================================================
% Copyright (C) 1997 Tom Minka <minka@microsoft.com>
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
% =========================================================================
% Original author: Thomas P Minka (tpminka@media.mit.edu), April 22, 2013

if nargin < 2
  m = n(2);
  n = n(1);
end
if nargin < 3
  sparseflag = 0;
end

if 0
  % first method
  i = 1:(n*m);
  a = reshape(i, n, m);
  j = vec(transpose(a));
  k = zeros(n*m,n*m);
  for r = i
    k(r, j(r)) = 1;
  end
else
  % second method
  k = reshape(kron(vec(eye(n)), eye(m)), n*m, n*m);
end

if sparseflag ~= 0
    k = sparse(k);
end

function V = vec(A)
    V = A(:); 
end

end
