verbatim;

    options_.noprint = true;

    number_of_simulations = 1;

    calibrated_values = M_.params;
    Sigma_e = M_.Sigma_e;

    options_.bnlms.set_dynare_seed_to_default = false;

    Beta = zeros(number_of_simulations, M_.param_nbr);

    for i=1:number_of_simulations
        % Set initial conditions randomly
        firstobs = rand(3, length(M_.endo_names));
        % Set parameters to calibrated values (because after the
        % estimation parameters in equation 7 are updated with OLS
        % estimator).
        M_.params = calibrated_values;
        M_.Sigma_e = Sigma_e;
        % Simulate the model.
        simdata = simul_backward_model(dseries(firstobs, dates('1995Q1'), M_.endo_names), 1100);
        % Select a subsample.
        simdata = simdata(simdata.dates(101:1100));
        % Get vector of indices to exclude exogenous variables from the dataset.
        names = regexp(simdata.name, 'res\w*');
        idxs = find(cellfun(@isempty, names));
        % Perform the estimation of equation 7.
        if strcmp(M_.fname, 'ols_param_names')
            dyn_ols(simdata{idxs}, {}, {'eq7'}, '', {{'de_g_yer_ecm_u2_stn_L1', 'de_g_yer_de_g_yer_L1'}});
        elseif strcmp(M_.fname, 'ols_date_range')
            dyn_ols(simdata{idxs}, {}, {'eq7'}, '', {}, simdata.dates(2:end-3));
        else
            dyn_ols(simdata{idxs}, {}, {'eq7'});
        end
        if i ~= number_of_simulations
            oo_ = rmfield(oo_, 'ols');
        end
        % Store the estimation results in Beta
        Beta(i, :) = M_.params';
    end

    % Get the indices of the estimated parameters
    pid = oo_.ols.eq7.param_idxs;

    if number_of_simulations > 1
        if max(abs(calibrated_values(pid)-mean(Beta(:,pid)', 2)))>1e-2
            error('There is probably an error in the OLS routine.')
        end
    else
        if strcmp(M_.fname, 'ols_base')
            good = [0.025487074270635
                -0.126705792250240
                0.002639663639062
                0.486913131141265
                -0.027702100375691];
        elseif strcmp(M_.fname, 'ols_param_names')
            good = [-0.125265361034515
                0.497003481797959];
        elseif strcmp(M_.fname, 'ols_date_range')
            good = [0.468607715487806
                -0.011224442004140
                0.033681224841824
                -0.126743281213504];
        elseif strcmp(M_.fname, 'ols_wc_1')
            good = [0.466534186307255
                -0.010596174928406
                0.035098464382347
                -0.126502172176703];
        elseif strcmp(M_.fname, 'ols_wc_2')
            good = [0.485802603276717
                0.002660854661647
                0.025434782170432
                -0.126939890810119
                0.043688440124315];
        elseif strcmp(M_.fname, 'ols_wc_3')
            good = [0.490636551206316
                0.014209413280418
                0.028534044956486
                -0.094387460262917];
        end
        if max(abs(good-Beta(:,pid)'))>1e-2
            error('There is probably an error in the OLS routine.')
        end
        return
    end

    for i=1:length(pid)
        figure(i)
        hold on
        title(strrep(M_.param_names(pid(i),:), '_', '\_'));
        bandwidth = mh_optimal_bandwidth(Beta(:,pid(i)), length(Beta(:,pid(i))), -1, 'gaussian');
        [abscissa, f] = kernel_density_estimate(Beta(:,pid(i)), 256, length(Beta(:,pid(i))), bandwidth, 'gaussian');
        plot(abscissa, f, '-k', 'linewidth', 2);
        line([calibrated_values(pid(i)) calibrated_values(pid(i))], [0 max(f)*1.05], 'LineWidth', 2, 'Color', 'r');
        line([mean(Beta(:,pid(i))) mean(Beta(:,pid(i)))], [0 max(f)*1.05], 'LineWidth', 2, 'Color', 'g');
        axis tight
        box on
        hold off
    end

end;

