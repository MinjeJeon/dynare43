// --+ options: json=compute, stochastic +--

var x1 x2 x1bar x2bar z y;

varexo ex1 ex2 ex1bar ex2bar ez ey;

parameters
       rho_1 rho_2
       a_x1_0 a_x1_1 a_x1_2 a_x1_x2_1 a_x1_x2_2
	   a_x2_0 a_x2_1 a_x2_2 a_x2_x1_1 a_x2_x1_2
	   e_c_m c_z_1 c_z_2 beta;

rho_1 = .9;
rho_2 = -.2;

a_x1_0 =  -.9;
a_x1_1 =  .4;
a_x1_2 =  .3;
a_x1_x2_1 = .1;
a_x1_x2_2 = .2;

a_x2_0 =  -.9;
a_x2_1 =   .2;
a_x2_2 =  -.1;
a_x2_x1_1 = -.1;
a_x2_x1_2 = .2;

beta  =  .2;
e_c_m =  .5;
c_z_1 =  .2;
c_z_2 = -.1;

trend_component_model(model_name=toto, eqtags=['eq:x1', 'eq:x2', 'eq:x1bar', 'eq:x2bar'], targets=['eq:x1bar', 'eq:x2bar']);

pac_model(auxiliary_model_name=toto, discount=beta, model_name=pacman);

model;

[name='eq:y']
y = rho_1*y(-1) + rho_2*y(-2) + ey;

[name='eq:x1', data_type='nonstationary']
diff(log(x1)) = a_x1_0*(log(x1(-1))-x1bar(-1)) + a_x1_1*diff(log(x1(-1))) + a_x1_2*diff(log(x1(-2))) + a_x1_x2_1*diff(x2(-1)) + a_x1_x2_2*diff(x2(-2)) + ex1;     

[name='eq:x2', data_type='nonstationary']
diff(x2) = a_x2_0*(x2(-1)-x2bar(-1)) + a_x2_1*diff(log(x1(-1))) + a_x2_2*diff(log(x1(-2))) + a_x2_x1_1*diff(x2(-1)) + a_x2_x1_2*diff(x2(-2)) + ex2;     

[name='eq:x1bar', data_type='nonstationary']
x1bar = x1bar(-1) + ex1bar;

[name='eq:x2bar', data_type='nonstationary']
x2bar = x2bar(-1) + ex2bar;

[name='zpac']
diff(z) = e_c_m*(log(x1(-1))-z(-1)) + c_z_1*diff(z(-1))  + c_z_2*diff(z(-2)) + pac_expectation(pacman) + ez;

end;

shocks;
    var ex1 = 1.0;
    var ex2 = 1.0;
    var ex1bar = .1;
    var ex2bar = .1;
    var ez = 1.0;
    var ey = 0.1;
end;

// Initialize the PAC model (build the Companion VAR representation for the auxiliary model).
pac.initialize('pacman');

// Update the parameters of the PAC expectation model (h0 and h1 vectors).
pac.update.expectation('pacman');

// Set initial conditions to zero. Please use more sensible values if any...
INITIALCONDITIONS = zeros(10, M_.endo_nbr+M_.exo_nbr);
INITIALCONDITIONS(:,1) = ones(10, 1);
initialconditions = dseries(INITIALCONDITIONS, 2000Q1, vertcat(M_.endo_names,M_.exo_names));

// Simulate the model for 500 periods
TrueData = simul_backward_model(initialconditions, 300);

// Define a structure describing the parameters to be estimated (with initial conditions). 
clear eparams
eparams.e_c_m = .9;
eparams.c_z_1 = .5;
eparams.c_z_2 = .2;

// Define the dataset used for estimation
edata = TrueData;
edata.ez = dseries(NaN(TrueData.nobs, 1), 2000Q1, 'ez');
pac.estimate.iterative_ols('zpac', eparams, edata, 2005Q1:2000Q1+200);

disp(sprintf('Estimate of e_c_m: %f', M_.params(strmatch('e_c_m', M_.param_names, 'exact'))))
disp(sprintf('Estimate of c_z_1: %f', M_.params(strmatch('c_z_1', M_.param_names, 'exact'))))
disp(sprintf('Estimate of c_z_2: %f', M_.params(strmatch('c_z_2', M_.param_names, 'exact'))))
