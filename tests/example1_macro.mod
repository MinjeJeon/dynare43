// Various tests for the macroprocessor

var y, c, k, a, h, b;
varexo e, u;

@#ifndef NOTDEFINED
parameters beta, rho, alpha, delta, theta, psi, tau;
@#else
@#error "IFNDEF PROBLEM"
@#endif

@#ifdef NOTDEFINED
@#error "IFDEF PROBLEM"
@#else
alpha = 0.36;
rho   = 0.95;
tau   = 0.025;
beta  = 0.99;
delta = 0.025;
psi   = 0;
theta = 2.95;
phi   = 0.1;
@#endif

@#define a = 5
@#define b = 2*(a + 3)/4-1
@#if b != 3
@#error "Arithmetic problem"
@#endif

@#define v = [ "a", 1, 2:3]
@#define empty = []
@#define z = v[2:3]
@#if z != [ 1, [ 2, 3 ]] || length(v) != 3 || 5 in v || !("a" in v) || length(empty) != 0
@#error "Array problem"
@#endif

@#define w = [ 1 ]
@#for elt in v
@#define w = w + [ elt ]
@#endfor
@#if w != [ 1, "a", 1, 2:3]
@#error "For loop problem"
@#endif

@#define s = "abcde"
@#if length(s) != 5 || s[3:4] != "cd"
@#error "String problem"
@#endif

@#define f(y, z) = "@{y}bar@{z}"
@#if f("foo", "baz") != "foobarbaz"
@#error "Function problem"
@#endif



model;
c*theta*h^(1+psi)=(1-alpha)*y;
k = beta*(((exp(b)*c)/(exp(b(+1))*c(+1)))
    *(exp(b(+1))*alpha*y(+1)+(1-delta)*k));
y = exp(a)*(k(-1)^alpha)*(h^(1-alpha));
k = exp(b)*(y-c)+(1-delta)*k(-1);
a = rho*a(-1)+tau*b(-1) + e;
b = tau*a(-1)+rho*b(-1) + u;
end;

initval;
y = 1.08068253095672;
c = 0.80359242014163;
h = 0.29175631001732;
k = 11.08360443260358;
a = 0;
b = 0;
e = 0;
u = 0;
end;

@#define DEFINED=0

@#ifndef DEFINED
@#error "IFNDEF PROBLEM"
@#else
shocks;
var e; stderr 0.009;
var u; stderr 0.009;
var e, u = phi*0.009*0.009;
end;
@#endif

@#ifdef DEFINED
stoch_simul;
@#else
@#error "IFDEF PROBLEM"
@#endif
