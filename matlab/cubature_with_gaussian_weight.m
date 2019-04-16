function [nodes, weights] = cubature_with_gaussian_weight(d,n,method)

% Computes nodes and weights for a n-order cubature with gaussian weight.
%
% INPUTS
% - d        [integer]     scalar, dimension of the region of integration.
% - n        [integer]     scalar, approximation order (3 or 5).
% - method   [string]      Method of approximation ('Stroud' or 'ScaledUnscentedTransform')
%
% OUTPUTS
% - nodes    [double]      n×m matrix, with m=2×d if n=3 or m=2×d²+1 if n=5, nodes where the integrated function has to be evaluated.
% - weights  [double]      m×1 vector, weights associated to the nodes.
%
% REMARKS
% The routine returns nodes and associated weights to compute a multivariate integral of the form:
%      ∞           -<x,x>
%     ∫   f(x) × e        dx
%      -∞

% Copyright (C) 2012-2019 Dynare Team
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

% Set default.
if nargin<3 || isempty(method)
    method = 'Stroud';
end

if strcmp(method,'Stroud') && isequal(n,3)
    r = sqrt(d);
    nodes = r*[eye(d),-eye(d)];
    weights = ones(2*d,1)/(2*d);
    return
end

if strcmp(method,'ScaledUnscentedTransform') && isequal(n,3)
    % For alpha=1 and beta=kappa=0 we obtain the same weights and nodes than the 'Stroud' method (with n=3).
    % For alpha=1, beta=0 and kappa=.5 we obtain sigma points with equal weights.
    alpha = 1;
    beta = 0;
    kappa = 0.5;
    lambda = (alpha^2)*(d+kappa) - d;
    nodes = [ zeros(d,1) ( sqrt(d+lambda).*([ eye(d), -eye(d)]) ) ];
    w0_m = lambda/(d+lambda);
    w0_c = w0_m + (1-alpha^2+beta);
    weights = [w0_c; .5/(d+lambda)*ones(2*d,1)];
    return
end

if strcmp(method,'Stroud') &&  isequal(n,5)
    r = sqrt((d+2));
    s = sqrt((d+2)/2);
    m = 2*d^2+1;
    A = 2/(n+2);
    B = (4-d)/(2*(n+2)^2);
    C = 1/(n+2)^2;
    % Initialize the outputs
    nodes = zeros(d,m);
    weights = zeros(m,1);
    % Set the weight for the first node (0)
    weights(1) = A;
    skip = 1;
    % Set the remaining nodes and associated weights.
    nodes(:,skip+(1:d)) = r*eye(d);
    weights(skip+(1:d)) = B;
    skip = skip+d;
    nodes(:,skip+(1:d)) = -r*eye(d);
    weights(skip+(1:d)) = B;
    skip = skip+d;
    for i=1:d-1
        for j = i+1:d
            nodes(:,skip+(1:4)) = s*ee(d,i,j);
            weights(skip+(1:4)) = C;
            skip = skip+4;
        end
    end
    return
end

if strcmp(method,'Stroud')
    error(['cubature_with_gaussian_weight:: Cubature (Stroud tables) is not yet implemented with n = ' int2str(n) '!'])
end




function v = e(n,i)
v = zeros(n,1);
v(i) = 1;

function m = ee(n,i,j)
m = zeros(n,4);
m(:,1) =  e(n,i)+e(n,j);
m(:,2) =  e(n,i)-e(n,j);
m(:,3) = -m(:,2);
m(:,4) = -m(:,1);

%@test:1
%$ % Set problem
%$ d = 4;
%$
%$ t = zeros(5,1);
%$
%$ % Call the tested routine
%$ try
%$     [nodes,weights] = cubature_with_gaussian_weight(d,3);
%$     t(1) = 1;
%$ catch
%$     exception = lasterror;
%$     t = t(1);
%$     T = all(t);
%$     LOG = getReport(exception,'extended');
%$     return
%$ end
%$
%$ % Check the results.
%$
%$ % Compute (approximated) first order moments.
%$ m1 = nodes*weights;
%$
%$ % Compute (approximated) second order moments.
%$ m2 = nodes.^2*weights;
%$
%$ % Compute (approximated) third order moments.
%$ m3 = nodes.^3*weights;
%$
%$ % Compute (approximated) fourth order moments.
%$ m4 = nodes.^4*weights;
%$
%$ t(2) = dassert(m1,zeros(d,1),1e-12);
%$ t(3) = dassert(m2,ones(d,1),1e-12);
%$ t(4) = dassert(m3,zeros(d,1),1e-12);
%$ t(5) = dassert(m4,d*ones(d,1),1e-10);
%$ T = all(t);
%@eof:1

%@test:2
%$ % Set problem
%$ d = 4;
%$ Sigma = diag(1:d);
%$ Omega = diag(sqrt(1:d));
%$
%$ t = zeros(5,1);
%$
%$ % Call the tested routine
%$ try
%$     [nodes,weights] = cubature_with_gaussian_weight(d,3);
%$     t(1) = 1;
%$ catch
%$     exception = lasterror;
%$     t = t(1);
%$     T = all(t);
%$     LOG = getReport(exception,'extended');
%$     return
%$ end
%$
%$ % Check the results.
%$ nodes = Omega*nodes;
%$
%$ % Compute (approximated) first order moments.
%$ m1 = nodes*weights;
%$
%$ % Compute (approximated) second order moments.
%$ m2 = nodes.^2*weights;
%$
%$ % Compute (approximated) third order moments.
%$ m3 = nodes.^3*weights;
%$
%$ % Compute (approximated) fourth order moments.
%$ m4 = nodes.^4*weights;
%$
%$ t(2) = dassert(m1,zeros(d,1),1e-12);
%$ t(3) = dassert(m2,transpose(1:d),1e-12);
%$ t(4) = dassert(m3,zeros(d,1),1e-12);
%$ t(5) = dassert(m4,d*transpose(1:d).^2,1e-10);
%$ T = all(t);
%@eof:2

%@test:3
%$ % Set problem
%$ d = 4;
%$ Sigma = diag(1:d);
%$ Omega = diag(sqrt(1:d));
%$
%$ t = zeros(4,1);
%$
%$ % Call the tested routine
%$ try
%$     [nodes,weights] = cubature_with_gaussian_weight(d,3);
%$     t(1) = 1;
%$ catch
%$     exception = lasterror;
%$     t = t(1);
%$     T = all(t);
%$     LOG = getReport(exception,'extended');
%$     return
%$ end
%$
%$ % Check the results.
%$ nodes = Omega*nodes;
%$
%$ % Compute (approximated) first order moments.
%$ m1 = nodes*weights;
%$
%$ % Compute (approximated) second order moments.
%$ m2 = bsxfun(@times,nodes,transpose(weights))*transpose(nodes);
%$
%$ t(2) = dassert(m1,zeros(d,1),1e-12);
%$ t(3) = dassert(diag(m2),transpose(1:d),1e-12);
%$ t(4) = dassert(m2(:),vec(diag(diag(m2))),1e-12);
%$ T = all(t);
%@eof:3

%@test:4
%$ % Set problem
%$ d = 10;
%$ a = randn(d,2*d);
%$ Sigma = a*a';
%$ Omega = chol(Sigma,'lower');
%$
%$ t = zeros(4,1);
%$
%$ % Call the tested routine
%$ try
%$     [nodes,weights] = cubature_with_gaussian_weight(d,3);
%$     t(1) = 1;
%$ catch
%$     exception = lasterror;
%$     t = t(1);
%$     T = all(t);
%$     LOG = getReport(exception,'extended');
%$     return
%$ end
%$
%$ % Correct nodes for the covariance matrix
%$ for i=1:length(weights)
%$     nodes(:,i) = Omega*nodes(:,i);
%$ end
%$
%$ % Check the results.
%$
%$ % Compute (approximated) first order moments.
%$ m1 = nodes*weights;
%$
%$ % Compute (approximated) second order moments.
%$ m2 =  bsxfun(@times,nodes,transpose(weights))*transpose(nodes);
%$
%$ % Compute (approximated) third order moments.
%$ m3 = nodes.^3*weights;
%$
%$ t(2) = dassert(m1,zeros(d,1),1e-12);
%$ t(3) = dassert(m2(:),vec(Sigma),1e-12);
%$ t(4) = dassert(m3,zeros(d,1),1e-12);
%$ T = all(t);
%@eof:4

%@test:5
%$ % Set problem
%$ d = 5;
%$
%$ t = zeros(6,1);
%$
%$ % Call the tested routine
%$ try
%$     [nodes,weights] = cubature_with_gaussian_weight(d,5);
%$     t(1) = 1;
%$ catch
%$     exception = lasterror;
%$     t = t(1);
%$     T = all(t);
%$     LOG = getReport(exception,'extended');
%$     return
%$ end
%$
%$ % Check the results.
%$ nodes = nodes;
%$
%$ % Compute (approximated) first order moments.
%$ m1 = nodes*weights;
%$
%$ % Compute (approximated) second order moments.
%$ m2 = nodes.^2*weights;
%$
%$ % Compute (approximated) third order moments.
%$ m3 = nodes.^3*weights;
%$
%$ % Compute (approximated) fourth order moments.
%$ m4 = nodes.^4*weights;
%$
%$ % Compute (approximated) fifth order moments.
%$ m5 = nodes.^5*weights;
%$
%$ t(2) = dassert(m1,zeros(d,1),1e-12);
%$ t(3) = dassert(m2,ones(d,1),1e-12);
%$ t(4) = dassert(m3,zeros(d,1),1e-12);
%$ t(5) = dassert(m4,3*ones(d,1),1e-12);
%$ t(6) = dassert(m5,zeros(d,1),1e-12);
%$ T = all(t);
%@eof:5

%@test:6
%$ % Set problem
%$ d = 3;
%$
%$ t = zeros(4,1);
%$
%$ % Call the tested routine
%$ try
%$     [nodes,weights] = cubature_with_gaussian_weight(d,3,'ScaledUnscentedTransform');
%$     nodes
%$     weights
%$     t(1) = 1;
%$ catch
%$     exception = lasterror;
%$     t = t(1);
%$     T = all(t);
%$     LOG = getReport(exception,'extended');
%$     return
%$ end
%$
%$ % Check the results.
%$
%$ % Compute (approximated) first order moments.
%$ m1 = nodes*weights;
%$
%$ % Compute (approximated) second order moments.
%$ m2 = nodes.^2*weights;
%$
%$ % Compute (approximated) third order moments.
%$ m3 = nodes.^3*weights;
%$
%$ t(2) = dassert(m1,zeros(d,1),1e-12);
%$ t(3) = dassert(m2,ones(d,1),1e-12);
%$ t(4) = dassert(m3,zeros(d,1),1e-12);
%$ T = all(t);
%@eof:6
