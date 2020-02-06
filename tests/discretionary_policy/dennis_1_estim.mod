/* Test for estimation under discretionary optimal policy.
   Uses the data generated by dennis_1.mod */

var y i pi pi_c q;
varexo g u e;

parameters omega kappa sigma beta alpha;

alpha = 0.4;
beta = 0.99;
sigma = 1;
omega = 0.9;
kappa = 0.3;

model(linear);
y = y(+1) -(omega/sigma)*(i-pi(+1))+g;
pi =  beta*pi(+1)+kappa*y+u;
pi_c = pi+(alpha/(1-alpha))*(q-q(-1));
q = q(+1)-(1-alpha)*(i-pi(+1))+(1-alpha)*e;
end;

shocks;
  var g; stderr 1;
  var u; stderr 1;
  var e; stderr 1;
end;

planner_objective pi_c^2 + y^2;
discretionary_policy(instruments=(i),irf=0,planner_discount=beta);

varobs y i pi;

estimated_params;
  omega, normal_pdf, 0.9, 0.1;
  kappa, normal_pdf, 0.2, 0.1;
end;

options_.plot_priors=0;
estimation(order = 1, datafile = dennis_simul, mh_replic = 2000, mh_nblocks=1,smoother,bayesian_irf,moments_varendo) y i pi pi_c q;

if max(abs(oo_.posterior.optimization.mode - [1; 0.3433])) > 0.025
  error('Posterior mode too far from true parameter values');
end
