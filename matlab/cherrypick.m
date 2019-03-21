function cherrypick(infile, outfold, eqtags, noresids)

% Extract some equations in infile (mod file used for estimation)
% and write them in outfile (mod file used for simulation).
%
% INPUTS
% - infile        [string]    Name of the mod file where all the equations used for estimation are available.
% - outfold       [string]    Name of the folder where the generated files are saveda subset of the equations is to be printed.
% - eqtags        [cell]      Equation tags of the selected equations.
% - noresids      [logical]   Removes estimation residuals (not to be used in simulation) if true.
%
% OUTPUTS
% none.
%
% SPECIAL REQUIREMENTS
% It is expected that the file infile.mod has already been run, and
% that the associated JSON output is available.

% Copyright (C) 2019 Dynare Team
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

global M_

% Set default value
if nargin<4
    noresids = true;
end

% Delete outfold subdirectory if it already exists
if exist(outfold, 'dir')
    rmdir(outfold, 's');
end

% Create the subdirectoty where the generated files will be saved.
mkdir(outfold);

% Check that infile.mod and the related JSON output exist.
if ~exist(sprintf('%s.mod', infile), 'file')
    error('Cannot find %s.mod.', infile)
end
if ~exist(sprintf('%s/model/json', infile), 'dir')
    error('Cannot find %s/model/json folder. Did you run %s.mod with the json option?', infile, infile);
end

% Create a new file.
fid = fopen(sprintf('%s/model.inc', outfold), 'w');

plist = {};
elist = {};
xlist = {};

for i=1:length(eqtags)
    rhs = [];
    lhs = [];
    % Get the original equation.
    [LHS, RHS] = get_lhs_and_rhs(eqtags{i}, M_, true);
    % Get the parameters, endogenous and exogenous variables in the current equation.
    [pnames, enames, xnames] = get_variables_and_parameters_in_equation(LHS, RHS, M_);
    % Remove residual from equation if required.
    if noresids
        exogenous_variables_to_be_removed = ~ismember(xnames, M_.simulation_exo_names);
        if any(exogenous_variables_to_be_removed)
            switch sum(exogenous_variables_to_be_removed)
              case 1
                RHS = regexprep(RHS, sprintf('(\\ *)(+)(\\ *)%s', xnames{exogenous_variables_to_be_removed}), '');
              case 0
                % Nothing to do.
              otherwise
                error('Cannot remove more than one exogenous variable in an equation (%s).', eqtags{i})
            end
            xnames = setdiff(xnames, xnames{exogenous_variables_to_be_removed});
        end
    end
    % Unroll expectation terms if any.
    isvar = regexp(RHS, 'var_expectation\(model_name = (?<name>\w+)\)', 'names');
    ispac = regexp(RHS, 'pac_expectation\(model_name = (?<name>\w+)\)', 'names');
    if ~isempty(isvar)
        rhs = write_expectations(eqtags{i}, isvar.name, 'var');
        lhs = sprintf('var_expectation_%s', eqtags{i});
        RHS = strrep(RHS, sprintf('var_expectation(model_name = %s)', isvar.name), lhs);
    else
        if ~isempty(ispac)
            [rhs, growthneutralitycorrection] = write_expectations(eqtags{i}, ispac.name, 'pac');
            lhs = sprintf('pac_expectation_%s', eqtags{i});
            RHS = strrep(RHS, sprintf('pac_expectation(model_name = %s)', ispac.name), lhs);
            if ~isempty(growthneutralitycorrection)
                RHS = sprintf('%s + %s', RHS, growthneutralitycorrection);
            end
        end
    end
    % Print equation for unrolled PAC/VAR-expectation and update
    % list of parameters and endogenous variables (if any).
    if ~isempty(rhs)
        [expectation_pnames, expectation_enames] = get_variables_and_parameters_in_equation(lhs, rhs, M_);
        pnames = union(pnames, expectation_pnames);
        enames = union(enames, expectation_enames);
        fprintf(fid, '%s = %s;\n\n', lhs, rhs);
    end
    % Update pnames, enames and xnames if PAC with growth neutrality correction.
    if ~isempty(ispac) && ~isempty(growthneutralitycorrection)
        [growthneutralitycorrection_pnames, ...
             growthneutralitycorrection_enames, ...
             growthneutralitycorrection_xnames] = get_variables_and_parameters_in_equation('', growthneutralitycorrection, M_);
        if ~isempty(growthneutralitycorrection_pnames)
            pnames = union(pnames, growthneutralitycorrection_pnames);
        end
        if ~isempty(growthneutralitycorrection_enames)
            enames = union(enames, growthneutralitycorrection_enames);
        end
        if ~isempty(growthneutralitycorrection_xnames)
            xnames = union(xnames, growthneutralitycorrection_xnames);
        end
    end
    % Print equation.
    fprintf(fid, '%s = %s;\n\n', LHS, RHS);
    % Update lists of parameters, endogenous variables and exogenous variables.
    plist = union(plist, pnames);
    elist = union(elist, enames);
    xlist = union(xlist, xnames);
end
fclose(fid);

fid = fopen(sprintf('%s/parameters.inc', outfold), 'w');
fprintf(fid, 'parameters %s;', sprintf('%s ', plist{:}));
fclose(fid);

fid = fopen(sprintf('%s/endogenous.inc', outfold), 'w');
fprintf(fid, 'var %s;', sprintf('%s ', elist{:}));
fclose(fid);

fid = fopen(sprintf('%s/exogenous.inc', outfold), 'w');
fprintf(fid, 'varexo %s;', sprintf('%s ', xlist{:}));
fclose(fid);