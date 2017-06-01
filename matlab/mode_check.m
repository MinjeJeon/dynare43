function mode_check(fun,x,hessian_mat,DynareDataset,DatasetInfo,DynareOptions,Model,EstimatedParameters,BayesInfo,BoundsInfo,DynareResults)
% Checks the estimated ML mode or Posterior mode.

%@info:
%! @deftypefn {Function File} mode_check (@var{fun}, @var{x}, @var{hessian_mat}, @var{DynareDataset}, @var{DynareOptions}, @var{Model}, @var{EstimatedParameters}, @var{BayesInfo}, @var{DynareResults})
%! @anchor{mode_check}
%! @sp 1
%! Checks the estimated ML mode or Posterior mode by plotting sections of the likelihood/posterior kernel.
%! Each plot shows the variation of the likelihood implied by the variations of a single parameter, ceteris paribus)
%! @sp 2
%! @strong{Inputs}
%! @sp 1
%! @table @ @var
%! @item fun
%! Objective function.
%! @item x
%! Estimated mode.
%! @item start
%! Hessian of the objective function at the estimated mode @var{x}.
%! @item DynareDataset
%! Structure specifying the dataset used for estimation (dataset_).
%! @item DynareOptions
%! Structure defining dynare's options (options_).
%! @item Model
%! Structure specifying the (estimated) model (M_).
%! @item EstimatedParameters
%! Structure specifying the estimated parameters (estimated_params_).
%! @item BayesInfo
%! Structure containing information about the priors used for estimation (bayestopt_).
%! @item DynareResults
%! Structure gathering the results (oo_).
%! @end table
%! @sp 2
%! @strong{Outputs}
%! @sp 2
%! @strong{This function is called by:}
%! @sp 2
%! @strong{This function calls:}
%! The objective function (@var{func}).
%! @end deftypefn
%@eod:

% Copyright (C) 2003-2017 Dynare Team
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

TeX = DynareOptions.TeX;
if ~isempty(hessian_mat);
    [ s_min, k ] = min(diag(hessian_mat));
end

fval = feval(fun,x,DynareDataset,DatasetInfo,DynareOptions,Model,EstimatedParameters,BayesInfo,BoundsInfo,DynareResults);

if ~isempty(hessian_mat);
    skipline()
    disp('MODE CHECK')
    skipline()
    fprintf('Fval obtained by the minimization routine (minus the posterior/likelihood)): %f', fval);
    skipline()
    if s_min<eps
        disp(sprintf('Most negative variance %f for parameter %d (%s = %f)', s_min, k , BayesInfo.name{k}, x(k)))
    end
end

[nbplt,nr,nc,lr,lc,nstar] = pltorg(length(x));

if TeX && any(strcmp('eps',cellstr(DynareOptions.graph_format)))
    fidTeX = fopen([Model.fname '_CheckPlots.tex'],'w');
    fprintf(fidTeX,'%% TeX eps-loader file generated by mode_check.m (Dynare).\n');
    fprintf(fidTeX,['%% ' datestr(now,0) '\n']);
    fprintf(fidTeX,' \n');
end

ll = DynareOptions.mode_check.neighbourhood_size;
if isinf(ll),
    DynareOptions.mode_check.symmetric_plots = 0;
end

mcheck = struct('cross',struct(),'emode',struct());

for plt = 1:nbplt,
    if TeX
        NAMES = [];
        TeXNAMES = [];
    end
    hh = dyn_figure(DynareOptions.nodisplay,'Name','Mode check plots');
    for k=1:min(nstar,length(x)-(plt-1)*nstar)
        subplot(nr,nc,k)
        kk = (plt-1)*nstar+k;
        [name,texname] = get_the_name(kk,TeX,Model,EstimatedParameters,DynareOptions);
        if TeX
            if isempty(NAMES)
                NAMES = name;
                TeXNAMES = texname;
            else
                NAMES = char(NAMES,name);
                TeXNAMES = char(TeXNAMES,texname);
            end
        end
        xx = x;
        l1 = max(BoundsInfo.lb(kk),(1-sign(x(kk))*ll)*x(kk)); m1 = 0; %lower bound
        l2 = min(BoundsInfo.ub(kk),(1+sign(x(kk))*ll)*x(kk)); %upper bound
        binding_lower_bound=0;
        binding_upper_bound=0;
        if isequal(x(kk),BoundsInfo.lb(kk))
            binding_lower_bound=1;
            bound_value=BoundsInfo.lb(kk);
        elseif isequal(x(kk),BoundsInfo.ub(kk))
            binding_upper_bound=1;
            bound_value=BoundsInfo.ub(kk);
        end      
        if DynareOptions.mode_check.symmetric_plots && ~binding_lower_bound && ~binding_upper_bound
            if l2<(1+ll)*x(kk) %test whether upper bound is too small due to prior binding
                l1 = x(kk) - (l2-x(kk)); %adjust lower bound to become closer
                m1 = 1;
            end
            if ~m1 && (l1>(1-ll)*x(kk)) && (x(kk)+(x(kk)-l1)<BoundsInfo.ub(kk)) % if lower bound was truncated and using difference from lower bound does not violate upper bound
                l2 = x(kk) + (x(kk)-l1); %set upper bound to same distance as lower bound
            end
        end
        z1 = l1:((x(kk)-l1)/(DynareOptions.mode_check.number_of_points/2)):x(kk);
        z2 = x(kk):((l2-x(kk))/(DynareOptions.mode_check.number_of_points/2)):l2;
        z  = union(z1,z2);
        if DynareOptions.mode_check.nolik==0,
            y = zeros(length(z),2);
            dy = priordens(xx,BayesInfo.pshape,BayesInfo.p6,BayesInfo.p7,BayesInfo.p3,BayesInfo.p4);
        end
        for i=1:length(z)
            xx(kk) = z(i);
            [fval, info, exit_flag] = feval(fun,xx,DynareDataset,DatasetInfo,DynareOptions,Model,EstimatedParameters,BayesInfo,BoundsInfo,DynareResults);
            if exit_flag
                y(i,1) = fval;
            else
                y(i,1) = NaN;
                if DynareOptions.debug
                    fprintf('mode_check:: could not solve model for parameter %s at value %4.3f, error code: %u\n',name,z(i),info(1))
                end
            end
            if DynareOptions.mode_check.nolik==0
                lnprior = priordens(xx,BayesInfo.pshape,BayesInfo.p6,BayesInfo.p7,BayesInfo.p3,BayesInfo.p4);
                y(i,2)  = (y(i,1)+lnprior-dy);
            end
        end
        mcheck.cross = setfield(mcheck.cross, name, [transpose(z), -y]);
        mcheck.emode = setfield(mcheck.emode, name, x(kk));
        fighandle=plot(z,-y);
        hold on
        yl=get(gca,'ylim');
        plot( [x(kk) x(kk)], yl, 'c', 'LineWidth', 1)
        NaN_index = find(isnan(y(:,1)));
        zNaN = z(NaN_index);
        yNaN = yl(1)*ones(size(NaN_index));
        plot(zNaN,yNaN,'o','MarkerEdgeColor','r','MarkerFaceColor','r','MarkerSize',6);
        title(name,'interpreter','none')
        axis tight
        if binding_lower_bound || binding_upper_bound
            xl=get(gca,'xlim');
            plot( [bound_value bound_value], yl, 'r--', 'LineWidth', 1)
            xlim([xl(1)-0.5*binding_lower_bound*(xl(2)-xl(1)) xl(2)+0.5*binding_upper_bound*(xl(2)-xl(1))])
        end
        hold off
        drawnow
    end
    if DynareOptions.mode_check.nolik==0,
        if isoctave
            axes('outerposition',[0.3 0.93 0.42 0.07],'box','on'),
        else
            axes('position',[0.3 0.01 0.42 0.05],'box','on'),
        end
        line_color=get(fighandle,'color');
        plot([0.48 0.68],[0.5 0.5],'color',line_color{2})
        hold on, plot([0.04 0.24],[0.5 0.5],'color',line_color{1})
        set(gca,'xlim',[0 1],'ylim',[0 1],'xtick',[],'ytick',[])
        text(0.25,0.5,'log-post')
        text(0.69,0.5,'log-lik kernel')
    end
    dyn_saveas(hh,[ Model.fname '_CheckPlots' int2str(plt) ],DynareOptions.nodisplay,DynareOptions.graph_format);
    if TeX && any(strcmp('eps',cellstr(DynareOptions.graph_format)))
        % TeX eps loader file
        fprintf(fidTeX,'\\begin{figure}[H]\n');
        for jj = 1:min(nstar,length(x)-(plt-1)*nstar)
            fprintf(fidTeX,'\\psfrag{%s}[1][][0.5][0]{%s}\n',deblank(NAMES(jj,:)),deblank(TeXNAMES(jj,:)));
        end
        fprintf(fidTeX,'\\centering \n');
        fprintf(fidTeX,'\\includegraphics[width=%2.2f\\textwidth]{%s_CheckPlots%s}\n',DynareOptions.figures.textwidth*min(k/nc,1),Model.fname,int2str(plt));
        fprintf(fidTeX,'\\caption{Check plots.}');
        fprintf(fidTeX,'\\label{Fig:CheckPlots:%s}\n',int2str(plt));
        fprintf(fidTeX,'\\end{figure}\n');
        fprintf(fidTeX,' \n');
    end
end
if TeX && any(strcmp('eps',cellstr(DynareOptions.graph_format)))
    fclose(fidTeX);
end

OutputDirectoryName = CheckPath('modecheck',Model.dname);
save([OutputDirectoryName '/check_plot_data.mat'],'mcheck');
