function write_latex_prior_table
%function write_latex_prior_table
% Writes a latex table with some descriptive statistics about the prior distribution.
%
% INPUTS
%    none
%
% OUTPUTS
%    none
%
% SPECIAL REQUIREMENTS
%    none

% Copyright (C) 2015-2019 Dynare Team
%
% This file is part of Dynare.
%
% Dynare is free software: you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
%
% Dynare is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with Dynare.  If not, see <http://www.gnu.org/licenses/>.

global M_ options_ bayestopt_ estim_params_

if ~isbayes(estim_params_)
    fprintf('\nwrite_latex_prior_table:: No prior distributions detected. Skipping table creation.\n')
    return
end

if (size(estim_params_.var_endo,1) || size(estim_params_.corrn,1))
    % Prior over measurement errors are defined...
    if ((isfield(options_,'varobs') && isempty(options_.varobs)) || ~isfield(options_,'varobs'))
        % ... But the list of observed variabled is not yet defined.
        fprintf(['\nwrite_latex_prior_table:: varobs should be declared before. Skipping table creation.\n'])
        return
    end
end

% Fill or update bayestopt_ structure
[xparam1, EstimatedParameters, BayesOptions, lb, ub, Model] = set_prior(estim_params_, M_, options_);

% Get untruncated bounds
bounds = prior_bounds(BayesOptions, options_.prior_trunc);
lb=bounds.lb;
ub=bounds.ub;

PriorNames = { 'Beta' , 'Gamma' , 'Gaussian' , 'Inv. Gamma' , 'Uniform' , 'Inv. Gamma -- 2', '', 'Weibull' };

if ~exist([M_.dname '/latex'],'dir')
    mkdir(M_.dname,'latex');
end
fidTeX = fopen([M_.dname, '/latex/' Model.fname '_priors_table.tex'],'w+');
fprintf(fidTeX,'%% TeX-table generated by Dynare write_latex_prior_table.m.\n');
fprintf(fidTeX,'%% Prior Information\n');
fprintf(fidTeX,['%% ' datestr(now,0)]);
fprintf(fidTeX,' \n');
fprintf(fidTeX,' \n');
fprintf(fidTeX,'\\begin{center}\n');
fprintf(fidTeX,'\\begin{longtable}{lcccccccc} \n');
fprintf(fidTeX,'\\caption{Prior information (parameters)}\\\\\n ');
fprintf(fidTeX,'\\label{Table:Prior}\\\\\n');
fprintf(fidTeX, '\\toprule%%\n');
if options_.prior_trunc==0
    fprintf(fidTeX,'  &  &  &  &  & \\multicolumn{2}{c}{Bounds} & \\multicolumn{2}{c}{90\\%% HPDI} \\\\ \n');
else
    fprintf(fidTeX,'  &  &  &  &  & \\multicolumn{2}{c}{Bounds*} & \\multicolumn{2}{c}{90\\%% HPDI} \\\\ \n');
end
fprintf(fidTeX,'  \\cmidrule(r{.75em}){6-7} \\cmidrule(r{.75em}){8-9}\n');
fprintf(fidTeX,'  & Distribution & Mean & Mode & Std.dev. & Lower & Upper & Lower & Upper  \\\\ \n');
fprintf(fidTeX, '\\midrule\n');
fprintf(fidTeX, '\\endfirsthead\n');
fprintf(fidTeX,'\\caption{(continued)}\\\\\n ');
fprintf(fidTeX, '\\toprule%%\n');
if options_.prior_trunc==0
    fprintf(fidTeX,'  &  &  &  &  & \\multicolumn{2}{c}{Bounds} & \\multicolumn{2}{c}{90\\%% HPDI} \\\\ \n');
else
    fprintf(fidTeX,'  &  &  &  &  & \\multicolumn{2}{c}{Bounds*} & \\multicolumn{2}{c}{90\\%% HPDI} \\\\ \n');
end
fprintf(fidTeX,'  \\cmidrule(r{.75em}){6-7} \\cmidrule(r{.75em}){8-9}\n');
fprintf(fidTeX,'  & Distribution & Mean & Mode & Std.dev. & Lower & Upper & Lower & Upper  \\\\ \n');
fprintf(fidTeX,'\\midrule\n');
fprintf(fidTeX,'\\endhead\n');
if options_.prior_trunc~=0
    fprintf(fidTeX,'\\midrule\n');
    fprintf(fidTeX,sprintf('\\\\caption*{*Displayed bounds are after applying a prior truncation of options_.trunc=%4.3f}\\\\\\\\\n',options_.prior_trunc));
end
fprintf(fidTeX,'\\midrule\n');
fprintf(fidTeX,'\\multicolumn{9}{r}{(Continued on next page)} \\\\ \n');
fprintf(fidTeX,'\\bottomrule\n');
fprintf(fidTeX,'\\endfoot\n');
if options_.prior_trunc~=0
    fprintf(fidTeX,'\\midrule\n');
    fprintf(fidTeX,sprintf('\\\\caption*{\\\\emph{Note:} Displayed bounds are after applying a prior truncation of options\\\\_.prior\\\\_trunc=%3.2e}\\\\\\\\\n',options_.prior_trunc));
end
fprintf(fidTeX,'\\bottomrule\n');
fprintf(fidTeX,'\\endlastfoot\n');
% Column 1: a string for the name of the prior distribution.
% Column 2: the prior mean.
% Column 3: the prior mode.
% Column 4: the prior standard deviation.
% Column 5: the lower bound of the prior density support.
% Column 6: the upper bound of the prior density support.
% Column 7: the lower bound of the interval containing 90% of the prior mass.
% Column 8: the upper bound of the interval containing 90% of the prior mass.
PriorIntervals = prior_bounds(BayesOptions,(1-options_.prior_interval)/2) ;
for i=1:size(BayesOptions.name,1)
    [tmp,TexName] = get_the_name(i, 1, Model, EstimatedParameters, options_);
    PriorShape = PriorNames{ BayesOptions.pshape(i) };
    PriorMean = BayesOptions.p1(i);
    PriorMode = BayesOptions.p5(i);
    PriorStandardDeviation = BayesOptions.p2(i);
    switch BayesOptions.pshape(i)
      case { 1 , 5 }
        LowerBound = BayesOptions.p3(i);
        UpperBound = BayesOptions.p4(i);
        if ~isinf(lb(i))
            LowerBound=max(LowerBound,lb(i));
        end
        if ~isinf(ub(i))
            UpperBound=min(UpperBound,ub(i));
        end
      case { 2 , 4 , 6, 8 }
        LowerBound = BayesOptions.p3(i);
        if ~isinf(lb(i))
            LowerBound=max(LowerBound,lb(i));
        end
        if ~isinf(ub(i))
            UpperBound=ub(i);
        else
            UpperBound = '$\infty$';
        end
      case 3
        if isinf(BayesOptions.p3(i)) && isinf(lb(i))
            LowerBound = '$-\infty$';
        else
            LowerBound = BayesOptions.p3(i);
            if ~isinf(lb(i))
                LowerBound=max(LowerBound,lb(i));
            end
        end
        if isinf(BayesOptions.p4(i)) && isinf(ub(i))
            UpperBound = '$\infty$';
        else
            UpperBound = BayesOptions.p4(i);
            if ~isinf(ub(i))
                UpperBound=min(UpperBound,ub(i));
            end
        end
      otherwise
        error('write_latex_prior_table:: Dynare bug!')
    end
    format_string = build_format_string(PriorMode, PriorStandardDeviation,LowerBound,UpperBound);
    fprintf(fidTeX,format_string, ...
            TexName, ...
            PriorShape, ...
            PriorMean, ...
            PriorMode, ...
            PriorStandardDeviation, ...
            LowerBound, ...
            UpperBound, ...
            PriorIntervals.lb(i), ...
            PriorIntervals.ub(i) );
end
fprintf(fidTeX,'\\end{longtable}\n ');
fprintf(fidTeX,'\\end{center}\n');
fprintf(fidTeX,'%% End of TeX file.\n');
fclose(fidTeX);

function format_string = build_format_string(PriorMode,PriorStandardDeviation,LowerBound,UpperBound)
format_string = ['%s & %s & %6.4f &'];
if isnan(PriorMode)
    format_string = [ format_string , ' %s &'];
else
    format_string = [ format_string , ' %6.4f &'];
end
if ~isnumeric(PriorStandardDeviation)
    format_string = [ format_string , ' %s &'];
else
    format_string = [ format_string , ' %6.4f &'];
end
if ~isnumeric(LowerBound)
    format_string = [ format_string , ' %s &'];
else
    format_string = [ format_string , ' %6.4f &'];
end
if ~isnumeric(UpperBound)
    format_string = [ format_string , ' %s &'];
else
    format_string = [ format_string , ' %6.4f &'];
end
format_string = [ format_string , ' %6.4f & %6.4f \\\\ \n'];