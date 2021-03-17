var R Pie Y;

varobs Y Pie R;

svar_identification;
lower_cholesky;
end;

markov_switching(chain=1,number_of_regimes=2,duration=2.5);

svar(variances, chain=1);

set_dynare_seed(5);

ms_estimation(datafile=msdata
		,freq=4
		,initial_year=1959
		,final_year=2005
		,nlags=4
		,max_repeated_optimization_runs=1
		,max_number_of_stages=0
);
ms_simulation(mh_replic=1000);
ms_compute_mdd;
ms_compute_probabilities;
ms_irf(parameter_uncertainty);
ms_forecast;
ms_variance_decomposition(no_error_bands);
