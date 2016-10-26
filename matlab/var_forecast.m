function y = var_forecast(M_, options_, name, h, y, fcv)

% name : filename
% M_
% options_
% name        string    name of var model, provided in var statement
% h           int       number of steps-ahead forecast
% y           matrix    rows: realizations of endogenous variables in declaration order; cols: realizations in t, t-1, t-2 ... order of VAR
% fcv         string    name of variable we want forecast for

% returns the h-step-ahead VAR(order) forecast for fcv

% example calling:
% In Matlab:
% >> coefficients{1} = [0.5000    0.1000; 0.4000    0.5000];
% >> coefficients{2} = [0         0     ; 0.2500    0     ];
% >> mu              = [0.0200; 0.0300];
% >> save('m1.mat', 'mu','coefficients');

% In .mod file:
% var a b c d;
% ...
% var(model_name=m1,order=2) a c;

% From Matlab backend:
% >> yt   = [0.0600;    33.0000;    0.0300;    22.0000];
% >> ytm1 = [0.0550;    11.0000;    0.0300;    88.0000];
% >> var_forecast(M_, options_, 'm1', 1, [yt ytm1])
% >> var_forecast(M_, options_, 'm1', 2, [yt ytm1], ['a'])

%% Find var in options_
order = '';
var_list = '';
for i=1:length(options_.var)
    if strcmp(options_.var(i).name, name)
        order = options_.var(i).order;
        var_list = options_.var(i).var_list_;
        break;
    end
end

if isempty(order)
    error([name ' not found in var specification declared in .mod file']);
end

%% construct y
assert(length(y) == length(M_.endo_names));
endo_names = cellstr(M_.endo_names);
yidx = zeros(size(endo_names));
for i=1:length(var_list)
    yidx = yidx | strcmp(strtrim(var_list(i,:)), endo_names);
end
y = y(yidx,:);

if nargin == 6
    fvidx = strcmp(fcv, endo_names);
end

%% load .mat file and rewrite as VAR(1)
load(name, 'coefficients', 'mu');
if ~exist('coefficients', 'var') || ~exist('mu', 'var')
    error([name ' : must contain the variables coefficients and mu']);
end
assert(h >= 1);

lm = length(mu);
lc = length(coefficients);
assert(lc == order);
if size(y,1) ~= lm || size(y,2) ~= order
    error('The dimensions of y are not correct. It should be an nvars x order matrix');
end

A = zeros(lm*lc, lm*lc);
for i=1:lc
    if any([lm lm] ~= size(coefficients{i}))
        error('The dimensions of mu and coefficients are off');
    end
    col = lm*(i-1)+1:lm*i;
    A(1:lm, col) = coefficients{i};
    if i ~= lc
        A(lm*i+1:lm*i+lm, col) = eye(lm, lm);
    end
end

mu = [mu; zeros(lm,1)];

%% Calculate Forecast
%  New Introduction to Multiple Time Series Analysis
%  Helmut Lutkepohl
%  page 34
%
% An = eye(size(A));
% for i=1:h-1
%     An = An + A^i;
% end
% y = An*mu + A^h*y(:);

for i=1:h
    y = mu + A*y(:);
end
y = y(1:lm);

if nargin == 6
    retidx = find(fvidx & yidx == 1);
    if isempty(retidx)
        return;
    elseif retidx == 1
        y = y(1);
    else
        y = y(sum(yidx(1:retidx-1))+1);
    end
end
end