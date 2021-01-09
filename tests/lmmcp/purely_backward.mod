// Regression test for bug #1720 (in the purely backward case)

var y;

varexo eps;

parameters rho;

rho = 0.9;

model;
  [ mcp = 'y>1' ]
  y = y(-1)^rho*exp(eps);
end;

initval;
    y = 1;
  eps = 0;
end;

steady;

check;

shocks;
    var eps;
    periods 1 10;
    values -1 1;
end;

perfect_foresight_setup(periods=20);
perfect_foresight_solver(lmmcp);

if ~oo_.deterministic_simulation.status
   error('Perfect foresight simulation failed')
end

if any(oo_.endo_simul < 1)
  error('y>1 constraint not enforced')
end
