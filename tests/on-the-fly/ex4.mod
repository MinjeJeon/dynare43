// --+ options: nostrict +--
/*
** Same as ex3.mod without the first equation, hours (h) are exogenous.
*/

model;

//[endogenous='h']
//c*theta*h^(1+psi)=(1-alpha)*y;

[endogenous='k']
k = beta*(((exp(b)*c)/(exp(b(+1))*c(+1)))
    *(exp(b(+1))*alpha*y(+1)+(1-delta)*k));

[endogenous='y']
y = exp(a)*(k(-1)^alpha)*(h^(1-alpha));

[endogenous='c']
k = exp(b)*(y-c)+(1-delta)*k(-1);

[endogenous='a']
a = rho*a(-1)+tau*b(-1) + e;

[endogenous='b']
b = tau*a(-1)+rho*b(-1) + u;

end;

if ~isequal(length(intersect(M_.endo_names, {'c'; 'y'; 'k'; 'b'; 'a'})), 5)
   error('Endogenous variables are wrong.')
end

if length(M_.param_names) > 0
   error('Parameters are wrong.')
end

if ~isequal(length(intersect(M_.exo_names, {'e'; 'u'; 'h'; 'alpha'; 'beta'; 'delta'; 'rho'; 'tau'})), 8)
   error('Exogenous variables are wrong.')
end
