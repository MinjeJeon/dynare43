function [pars, cosnJ] = ident_bruteforce(J, max_dim_cova_group, TeX, name_tex, tittxt, tol_deriv)
% function [pars, cosnJ] = ident_bruteforce(J,n,TeX, pnames_TeX,tittxt)
% -------------------------------------------------------------------------
% given the Jacobian matrix J of moment derivatives w.r.t. parameters
% computes, for  each column of J, the groups of columns from 1 to n that
% can replicate at best the derivatives of that column
% =========================================================================
% INPUTS
%  J                  [double] (normalized) Jacobian matrix of moment derivatives
%  max_dim_cova_group [scalar] maximum size of covariance groups tested
%  TeX                [scalar] Indicator whether TeX-output is requested
%  pnames_TeX         [char] list of tex names
%  tittxt             [string]  string indicating the title text for
%                               graphs and figures
% -------------------------------------------------------------------------
% OUTPUTS
%  pars  : cell array with group of params for each column of J for 1 to n
%  cosnJ : cosn of each column with the selected group of columns
% -------------------------------------------------------------------------
% This function is called by
%   * identification_analysis.m
% =========================================================================
% Copyright (C) 2009-2019 Dynare Team
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
% =========================================================================

global M_ options_

OutputDirectoryName = CheckPath('identification',M_.dname);

totparam_nbr = size(J,2); % number of parameters

if nargin<2 || isempty(max_dim_cova_group)
    max_dim_cova_group = 4; % max n-tuple
end
if nargin<3 || isempty(TeX)
    TeX = 0; % no Tex output
    tittxt='';
end
if nargin < 6
    tol_deriv = 1.e-8;
end

tittxt1=regexprep(tittxt, ' ', '_');
tittxt1=strrep(tittxt1, '.', '');

cosnJ = zeros(totparam_nbr,max_dim_cova_group); %initialize
pars{totparam_nbr,max_dim_cova_group}=[];       %initialize
for ll = 1:max_dim_cova_group
    h = dyn_waitbar(0,['Brute force collinearity for ' int2str(ll) ' parameters.']);
    for ii = 1:totparam_nbr
        tmp = find([1:totparam_nbr]~=ii);
        tmp2  = nchoosek(tmp,ll); %find all possible combinations, ind16 could speed this up
        % One could also use a mex version of nchoosek to speed this up, e.g.VChooseK from https://de.mathworks.com/matlabcentral/fileexchange/26190-vchoosek
        cosnJ2=zeros(size(tmp2,1),1);
        b=[];
        for jj = 1:size(tmp2,1)
            [cosnJ2(jj,1), b(:,jj)] = cosn([J(:,ii),J(:,tmp2(jj,:))]);
        end
        cosnJ(ii,ll) = max(cosnJ2(:,1));
        if cosnJ(ii,ll)>tol_deriv
            if ll>1 && ((cosnJ(ii,ll)-cosnJ(ii,ll-1))<tol_deriv)
                pars{ii,ll} = [pars{ii,ll-1} NaN];
                cosnJ(ii,ll) = cosnJ(ii,ll-1);
            else
                tmp3 = tmp2(find(cosnJ2(:,1)==max(cosnJ2(:,1))),:);
                pars{ii,ll} = tmp3(1,:);
            end
        else
            pars{ii,ll} = NaN(1,ll);
        end
        dyn_waitbar(ii/totparam_nbr,h)
    end
    dyn_waitbar_close(h);
    if TeX
        filename = [OutputDirectoryName '/' M_.fname '_collin_patterns_',tittxt1,'_' int2str(ll) '.tex'];
        fidTeX = fopen(filename,'w');
        fprintf(fidTeX,'%% TeX-table generated by ident_bruteforce (Dynare).\n');
        fprintf(fidTeX,['%% Collinearity patterns with ',int2str(ll),' parameter(s): ',tittxt,'\n']);
        fprintf(fidTeX,['%% ' datestr(now,0)]);
        fprintf(fidTeX,' \n');
        fprintf(fidTeX,' \n');

        fprintf(fidTeX,'{\\tiny \n');
        fprintf(fidTeX,'\\begin{longtable}{llc} \n');
        fprintf(fidTeX,['\\caption{Collinearity patterns with ',int2str(ll),' parameter(s): ',tittxt,'}\n ']);
        fprintf(fidTeX,['\\label{Table:CollinearityPatterns:',tittxt1,':',int2str(ll),'}\\\\\n']);
        fprintf(fidTeX,'\\toprule \n');
        fprintf(fidTeX,'  Parameter & Explanatory & cosn \\\\ \n');
        fprintf(fidTeX,'            & parameter(s)   &  \\\\ \n');
        fprintf(fidTeX,'\\midrule \\endfirsthead \n');
        fprintf(fidTeX,'\\caption{(continued)}\\\\\n ');
        fprintf(fidTeX,'\\bottomrule \n');
        fprintf(fidTeX,'  Parameter & Explanatory & cosn \\\\ \n');
        fprintf(fidTeX,'            & parameter(s)   &  \\\\ \n');
        fprintf(fidTeX,'\\midrule \\endhead \n');
        fprintf(fidTeX,'\\bottomrule \\multicolumn{3}{r}{(Continued on next page)}\\endfoot \n');
        fprintf(fidTeX,'\\bottomrule\\endlastfoot \n');
        for i=1:totparam_nbr
            plist='';
            for ii=1:ll
                if ~isnan(pars{i,ll}(ii))
                    plist = [plist ' $' name_tex{pars{i,ll}(ii)} '\;\; $ '];
                else
                    plist = [plist ' ---- '];
                end
            end
            fprintf(fidTeX,'$%s$ & [%s] & %7.3f \\\\ \n',...
                    name_tex{i},...
                    plist,...
                    cosnJ(i,ll));
        end
        fprintf(fidTeX,'\\bottomrule \n');
        fprintf(fidTeX,'\\end{longtable}\n');
        fprintf(fidTeX,'} \n');
        fprintf(fidTeX,'%% End of TeX file.\n');
        fclose(fidTeX);
    end
end
