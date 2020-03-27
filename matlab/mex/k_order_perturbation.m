% [dynpp_derivs, dyn_derivs] = k_order_perturbation(dr,DynareModel,DynareOptions,g1,g2,g3)
% computes a k_order_petrubation solution for k=1,2,3
%
% INPUTS
% dr:            struct   describing the reduced form solution of the model.
% DynareModel:   struct   jobs's parameters
% DynareOptions: struct   job's options
% g1:            matrix   First order derivatives of the model (optional)
% g2:            matrix   Second order derivatives of the model (optional)
% g3:            matrix   Third order derivatives of the model (optional)
%
% OUTPUTS
% dynpp_derivs   struct   Derivatives of the decision rule in Dynare++ format.
%                         The tensors are folded and the Taylor coefficients (1/n!) are
%                         included.
%                         Fieldnames are g_0, g_1, g_2, …
% dyn_derivs     struct   Derivatives of the decision rule in Dynare format, up to third order.
%                         Matrices are dense and unfolded. The Taylor coefficients (1/2
%                         and 1/6) aren't included.
%                         The derivatives w.r.t. different categories of variables
%                         are separated.
%                         Fieldnames are:
%                          + gy, gu
%                          + if order ≥ 2: gyy, gyu, guu, gss
%                          + if order ≥ 3: gyyy, gyyu, gyuu, guuu, gyss, guss
%
% k_order_perturbation is a compiled MEX function. It's source code is in
% dynare/mex/sources/k_order_perturbation.cc and it uses code provided by
% dynare++

% Copyright (C) 2013-2020 Dynare Team
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
