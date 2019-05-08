function rplot(s1)
% function rplot(s1)
%
% Plots the simulated trajectory of one or several variables.
% The entire simulation period is plotted, unless instructed otherwise
% with "dsample".
%
% INPUTS
%    s1    [cell]           variable names
%
% OUTPUTS
%    none
%
% SPECIAL REQUIREMENTS
%    none

% Copyright (C) 2001-2019 Dynare Team
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

global M_ oo_ options_

if isempty(oo_.endo_simul)
    error('rplot: oo_.endo_simul is empty.')
end

% create subdirectory <fname>/graphs if it doesn't exist
if ~exist(M_.fname, 'dir')
    mkdir('.',M_.fname);
end
if ~exist([M_.fname filesep 'graphs'],'dir')
    mkdir(M_.fname,'graphs');
end

ix = (1 - M_.maximum_lag:size(oo_.endo_simul,2)-M_.maximum_lag)';

y = [];
for k = 1:length(s1)
    if ~any(strcmp(s1{k}, M_.endo_names))
        if ~any(strcmp(s1{k}, M_.exo_names))
            error ('rplot: One of the variables specified does not exist') ;
        else
            y = [y; oo_.exo_simul(:, strcmp(s1{k}, M_.exo_names))'] ;
        end
    else
        y = [y; oo_.endo_simul(strcmp(s1{k}, M_.endo_names), :)];
    end
end

if options_.smpl == 0
    i = (max(1, M_.maximum_lag):size(oo_.endo_simul,2))';
else
    i = (options_.smpl(1)+M_.maximum_lag:options_.smpl(2)+M_.maximum_lag)';
end

if options_.TeX && any(strcmp('eps',cellstr(options_.graph_format)))
    fidTeX = fopen([M_.fname, filesep, 'graphs', filesep, M_.fname '_simulated_trajectories_', num2str(options_.rplottype), '.tex'],'w');
    fprintf(fidTeX,'%% TeX eps-loader file generated by rplot.m (Dynare).\n');
    fprintf(fidTeX,['%% ' datestr(now,0) '\n']);
end

t = ['Plot of '] ;
if options_.rplottype == 0
    for j = 1:size(y,1)
        t = [t s1{j} ' '] ;
    end
    hh=dyn_figure(options_.nodisplay,'Name', 'Simulated Trajectory');
    plot(ix(i),y(:,i)) ;
    title (t,'Interpreter','none') ;
    xlabel('Periods') ;
    xlim([min(ix(i)) max(ix(i))])
    if length(s1) > 1
        if isoctave
            legend(s1);
        else
            h = legend(s1);
            set(h, 'Interpreter', 'none');
        end
    end
    dyn_saveas(hh,[M_.fname, filesep, 'graphs', filesep, 'SimulatedTrajectory_' s1{1}],options_.nodisplay,options_.graph_format)
    if options_.TeX && any(strcmp('eps',cellstr(options_.graph_format)))
        create_TeX_loader(fidTeX,[M_.fname, '/graphs/', 'SimulatedTrajectory_' s1{1}],'Simulated trajectories','SimulatedTrajectory_',s1{1},1)
    end
elseif options_.rplottype == 1
    for j = 1:size(y,1)
        hh=dyn_figure(options_.nodisplay,'Name', 'Simulated Trajectory');
        plot(ix(i),y(j,i)) ;
        xlim([min(ix(i)) max(ix(i))])
        title(['Plot of ' s1{j}],'Interpreter','none') ;
        xlabel('Periods') ;
        dyn_saveas(hh,[M_.fname, filesep, 'graphs', filesep, 'SimulatedTrajectory_' s1{j}],options_.nodisplay,options_.graph_format)
        if options_.TeX && any(strcmp('eps',cellstr(options_.graph_format)))
            create_TeX_loader(fidTeX,[M_.fname, '/graphs/', 'SimulatedTrajectory_' s1{j}],'Simulated trajectories','SimulatedTrajectory_',s1{j},1);
        end
    end
elseif options_.rplottype == 2
    hh=dyn_figure(options_.nodisplay,'Name', 'Simulated Trajectory');
    nl = max(1,fix(size(y,1)/4)) ;
    nc = ceil(size(y,1)/nl) ;
    for j = 1:size(y,1)
        subplot(nl,nc,j) ;
        plot(ix(i),y(j,i)) ;
        hold on ;
        if any(strcmp(s1{j}, M_.endo_names))
            plot(ix(i),oo_.steady_state(strcmp(s1{j}, M_.endo_names))*ones(1,size(i,1)),'r:') ;
        else
            plot(ix(i),oo_.exo_steady_state(strcmp(s1{j}, M_.exo_names))*ones(1,size(i,1)),'r:') ;
        end
        xlabel('Periods') ;
        ylabel([s1{j}],'Interpreter','none') ;
        title(['Plot of ' s1{j}],'Interpreter','none') ;
        axis tight;
    end
    dyn_saveas(hh,[M_.fname, filesep, 'graphs', filesep, 'SimulatedTrajectory_' s1{1}],options_.nodisplay,options_.graph_format)
    if options_.TeX && any(strcmp('eps',cellstr(options_.graph_format)))
        create_TeX_loader(fidTeX,[M_.fname, '/graphs/', 'SimulatedTrajectory_' s1{1}],'Simulated trajectories','SimulatedTrajectory_', s1{1},min(j/nc,1));
    end
end

if options_.TeX && any(strcmp('eps',cellstr(options_.graph_format)))
    fprintf(fidTeX,'%% End Of TeX file. \n');
    fclose(fidTeX);
end
end

function create_TeX_loader(fidTeX,figpath,caption,label_name,label_type,scale_factor)
if nargin<5
    scale_factor=1;
end
fprintf(fidTeX,' \n');
fprintf(fidTeX,'\\begin{figure}[H]\n');
fprintf(fidTeX,'\\centering \n');
fprintf(fidTeX,'\\includegraphics[width=%2.2f\\textwidth]{%s}\n',0.8*scale_factor,strrep(figpath,'\','/'));
fprintf(fidTeX,'\\caption{%s.}',caption);
fprintf(fidTeX,'\\label{Fig:%s:%s}\n',label_name,label_type);
fprintf(fidTeX,'\\end{figure}\n\n');
end