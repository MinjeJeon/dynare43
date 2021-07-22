/* Checks whether second order coefficients are the same with order=2 and order=3 with k_order_solver
   (use_dll version) */

var m P c e W R k d n l gy_obs gp_obs y dA ;
varexo e_a e_m;

parameters alp bet gam mst rho psi del;

alp = 0.33;
bet = 0.99;
gam = 0.003;
mst = 1.011;
rho = 0.7;
psi = 0.787;
del = 0.02;

model (use_dll);
dA = exp(gam+e_a);
log(m) = (1-rho)*log(mst) + rho*log(m(-1))+e_m;
-P/(c(+1)*P(+1)*m)+bet*P(+1)*(alp*exp(-alp*(gam+log(e(+1))))*k^(alp-1)*n(+1)^(1-alp)+(1-del)*exp(-(gam+log(e(+1)))))/(c(+2)*P(+2)*m(+1))=0;
W = l/n;
-(psi/(1-psi))*(c*P/(1-n))+l/n = 0;
R = P*(1-alp)*exp(-alp*(gam+e_a))*k(-1)^alp*n^(-alp)/W;
1/(c*P)-bet*P*(1-alp)*exp(-alp*(gam+e_a))*k(-1)^alp*n^(1-alp)/(m*l*c(+1)*P(+1)) = 0;
c+k = exp(-alp*(gam+e_a))*k(-1)^alp*n^(1-alp)+(1-del)*exp(-(gam+e_a))*k(-1);
P*c = m;
m-1+d = l;
e = exp(e_a);
y = k(-1)^alp*n^(1-alp)*exp(-alp*(gam+e_a));
gy_obs = dA*y/y(-1);
gp_obs = (P/P(-1))*m(-1)/dA;
end;

steady_state_model;
  dA = exp(gam);
  gst = 1/dA;
  m = mst;
  khst = ( (1-gst*bet*(1-del)) / (alp*gst^alp*bet) )^(1/(alp-1));
  xist = ( ((khst*gst)^alp - (1-gst*(1-del))*khst)/mst )^(-1);
  nust = psi*mst^2/( (1-alp)*(1-psi)*bet*gst^alp*khst^alp );
  n  = xist/(nust+xist);
  P  = xist + nust;
  k  = khst*n;

  l  = psi*mst*n/( (1-psi)*(1-n) );
  c  = mst/P;
  d  = l - mst + 1;
  y  = k^alp*n^(1-alp)*gst^alp;
  R  = mst/bet;
  W  = l/n;
  ist  = y-c;
  q  = 1 - d;

  e = 1;
  
  gp_obs = m/dA;
  gy_obs = dA;
end;

shocks;
var e_a; stderr 0.014;
var e_m; stderr 0.005;
end;

steady;

stoch_simul(order=3,periods=1000);

if ~exist(['fs2000k2a' filesep 'Output' filesep 'fs2000k2a_results.mat'],'file');
   error('fs2000k2a must be run first');
end;

oo1 = load(['fs2000k2a' filesep 'Output' filesep 'fs2000k2a_results'],'oo_');

dr0 = oo1.oo_.dr;
dr = oo_.dr;

if max(max(abs(dr0.ghxx - dr.ghxx))) > 1e-12;
   error('error in ghxx');
end;
if max(max(abs(dr0.ghuu - dr.ghuu))) > 1e-12;
   error('error in ghuu');
end;
if max(max(abs(dr0.ghxu - dr.ghxu))) > 1e-12;
   error('error in ghxu');
end;
if max(max(abs(dr0.ghs2 - dr.ghs2))) > 1e-12;
   error('error in ghs2');
end;

