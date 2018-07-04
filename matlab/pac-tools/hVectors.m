function [h0, h1, longruncorrectionparameter] = hVectors(params, H, ids, idns, auxmodel)

% INPUTS
% - params          [double]     (m+1)*1 vector, PAC parameters (lag polynomial coefficients and discount factor).
% - H               [double]     (np*np) matrix, companion matrix of the VAR(p) model.
% - ids             [integer]    n*1 vector, selection of the stationary components of the VAR.
% - idns            [integer]    n*1 vector, selection of the non stationary components of the VAR.
%
% OUTPUTS
% - h0              [double]     1*n vector.
% - h1              [double]     1*n vector.

if nargout>1 && nargin<4
    error('Wrong number of Inputs/Outputs!')
end

[G, alpha, beta] = buildGmatrixWithAlphaAndBeta(params);

A = [alpha; 1];

A_1 = polyval(A, 1.0);
A_b = polyval(A, beta);

m = length(alpha);
n = length(H);

tmp = eye(n*m)-kron(G, transpose(H));

if isempty(ids)
    h0 = [];
else
    h0 = A_1*A_b*((kron(iota(m, m), H))'*(tmp\kron(iota(m, m), iota(n, ids))));
end

if nargout>1
    if isempty(idns)
        h1 = [];
    else
        switch auxmodel
          case 'var'
            h1 = A_1*A_b*(kron(iota(m, m)'*inv(eye(m)-G), H')*(tmp\kron(iota(m, m), iota(n, idns))));
          case 'vecm'
            h1 = A_1*A_b*(kron(iota(m, m)'*inv(eye(m)-G), (H'-eye(length(H))))*(tmp\kron(iota(m, m), iota(n, idns))));
        end
    end
end

if nargout>2
    d = A_1*A_b*(iota(m, m)'*inv((eye(m)-G)*(eye(m)-G))*iota(m, m));
    longruncorrectionparameter = (1-sum(params(2:end-2))-d);
end