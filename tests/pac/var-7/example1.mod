// --+ options: json=compute, stochastic +--

var y x z u;

varexo ex ey ez eu;

parameters a_y_0 a_y_1 a_y_2 b_y_1 b_y_2 b_x_0 b_x_1 b_x_2 ; // VAR parameters

parameters g beta e_c_m c_z_1 c_z_2;                         // PAC equation parameters

a_y_0 =  .4;
a_y_1 =  .2;
a_y_2 =  .3;
b_y_1 =  .1;
b_y_2 =  .4;
b_x_0 = -.1;
b_x_1 = -.1;
b_x_2 = -.2;

beta  =  .9;
e_c_m =  .1;
c_z_1 =  .7;
c_z_2 = -.3;

g = .1;

var_model(model_name=toto, structural, eqtags=['eq:x', 'eq:y']);

pac_model(auxiliary_model_name=toto, discount=beta, model_name=pacman, growth=diff(u(-1)));

model;

[name='eq:u']
diff(u) = g + eu;

[name='eq:y']
y = a_y_1*y(-1) + a_y_2*diff(x(-1)) + a_y_0*diff(x) + b_y_1*y(-2) + b_y_2*diff(x(-2)) + ey ;

[name='eq:x']
diff(x) = b_x_0*y + b_x_1*y(-2) + b_x_2*diff(x(-1)) + g*(1-b_x_2)  + ex ;

[name='eq:pac']
diff(z) = e_c_m*(x(-1)-z(-1)) + c_z_1*diff(z(-1))  + c_z_2*diff(z(-2)) + pac_expectation(pacman) + ez;

end;

shocks;
    var ex = 1.0;
    var ey = 1.0;
    var ez = 1.0;
    var eu = 0.1;
end;

// Initialize the PAC model (build the Companion VAR representation for the auxiliary model).
pac.initialize('pacman');

// Update the parameters of the PAC expectation model (h0 and h1 vectors).
pac.update.expectation('pacman');

// Set initial conditions to zero. Please use more sensible values if any...
initialconditions = dseries(zeros(10, M_.endo_nbr+M_.exo_nbr), 2000Q1, vertcat(M_.endo_names,M_.exo_names));

// Simulate the model for 20 periods
set_dynare_seed('default');
TrueData = simul_backward_model(initialconditions, 20);

// Print expanded PAC_EXPECTATION term.
pac.print('pacman', 'eq:pac');

verbatim;
  set_dynare_seed('default');
  y = zeros(M_.endo_nbr,1);
  y(1:M_.orig_endo_nbr) = rand(M_.orig_endo_nbr, 1);
  x = randn(M_.exo_nbr,1);
  y = example1.set_auxiliary_variables(y, x, M_.params);
  y = [y(find(M_.lead_lag_incidence(1,:))); y];
  [residual, g1] = example1.dynamic(y, x', M_.params, oo_.steady_state, 1);
  save('example1.mat', 'residual', 'g1', 'TrueData');
end;