// --+ options: json=compute, transform_unary_ops, stochastic +--

var x1 x2 x1bar x2bar z ;

varexo ex1 ex2 ex1bar ex2bar ez ;

parameters a_x1_0 a_x1_1 a_x1_2 a_x1_x2_1 a_x1_x2_2
	   a_x2_0 a_x2_1 a_x2_2 a_x2_x1_1 a_x2_x1_2
	   e_c_m c_z_1 c_z_2 gamma beta ;

a_x1_0 =  -.9999;
a_x1_1 =  .4;
a_x1_2 =  0;//.3;
a_x1_x2_1 = .1;
a_x1_x2_2 = 0;//.2;


a_x2_0 =  -.9;
a_x2_1 =   .2;
a_x2_2 =  0;//-.1;
a_x2_x1_1 = -.1;
a_x2_x1_2 = 0;//.2;

beta  =  .1;
e_c_m =  .1;
c_z_1 =  .07;
c_z_2 = -.3;

gamma =  .7;

trend_component_model(model_name=toto, eqtags=['eq:x1', 'eq:x2', 'eq:x1bar',  'eq:x2bar'], targets=['eq:x1bar',  'eq:x2bar']);

pac_model(auxiliary_model_name=toto, discount=beta, model_name=pacman);

model;

[name='eq:x1']
diff(diff(log(x1))) = a_x1_0*(diff(log(x1(-1)))-x1bar(-1)) + a_x1_1*diff(diff(log(x1(-1)))) + a_x1_2*diff(diff(x1(-2))) + a_x1_x2_1*diff(log(x2(-1))) + a_x1_x2_2*diff(log(x2(-2))) + ex1;     

[name='eq:x2']
diff(log(x2)) = a_x2_0*(log(x2(-1))-x2bar(-1)) + a_x2_1*diff(diff(log(x1(-1)))) + a_x2_2*diff(diff(log(x1(-2)))) + a_x2_x1_1*diff(log(x2(-1))) + a_x2_x1_2*diff(log(x2(-2))) + ex2;     

[name='eq:x1bar']
x1bar = x1bar(-1) + ex1bar;

[name='eq:x2bar']
x2bar = x2bar(-1) + ex2bar;

[name='eq:pac']
diff(z) = gamma*(e_c_m*(log(x1(-1))-z(-1)) + c_z_1*diff(z(-1))  + c_z_2*diff(z(-2)) + pac_expectation(pacman)) + (1-gamma)*ez;


end;

shocks;
    var ex1 = 1;
    var ex2 = 1;
    var ex1bar = .1;
    var ex2bar = .11;
    var ez = 1;
end;

// Initialize the PAC model (build the Companion VAR representation for the auxiliary model).
pac.initialize('pacman');

// Update the parameters of the PAC expectation model (h0 and h1 vectors).
pac.update.expectation('pacman');

// Set initial conditions to zero for non logged variables, and one for logged variables
init = .1*ones(10,M_.endo_nbr+M_.exo_nbr);
initialconditions = dseries(init, 2000Q1, vertcat(M_.endo_names,M_.exo_names));

// Simulate the model for 500 periods
TrueData = simul_backward_model(initialconditions, 10);
