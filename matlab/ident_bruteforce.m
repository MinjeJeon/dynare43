function [pars, cosnJ] = ident_bruteforce(J,n,TeX, pnames_TeX,tittxt)
% function [pars, cosnJ] = ident_bruteforce(J,n,TeX, pnames_TeX,tittxt)
%
% given the Jacobian matrix J of moment derivatives w.r.t. parameters
% computes, for  each column of  J, the groups of columns from 1 to n that
% can repliate at best the derivatives of that column
%
% INPUTS
%  J                  [double] (normalized) Jacobian matrix of moment derivatives
%  n                  [scalar] maximum size of covariance groups tested
%  TeX                [scalar] Indicator whether TeX-output is requested
%  pnames_TeX         [char] list of tex names
%  tittxt             [string]  string indicating the title text for
%                               graphs and figures
% 
% OUTPUTS
%  pars  : cell array with groupf of params for each column of J for 1 to n
%  cosnJ : the cosn of each column with the selected group of columns

% Copyright (C) 2009-2016 Dynare Team
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
% along with Dynare.  If not, see <http://www.gnu.org/licen
global M_ options_

OutputDirectoryName = CheckPath('Identification',M_.dname);

k = size(J,2); % number of parameters

if nargin<2 || isempty(n)
    n = 4; % max n-tuple
end
if nargin<3 || isempty(TeX)
    TeX = 0; % max n-tuple
    tittxt='';
end

tittxt1=regexprep(tittxt, ' ', '_');
tittxt1=strrep(tittxt1, '.', '');

cosnJ=zeros(k,n);
pars{k,n}=[];
for ll = 1:n,
    h = dyn_waitbar(0,['Brute force collinearity for ' int2str(ll) ' parameters.']);
    for ii = 1:k
        tmp = find([1:k]~=ii);
        tmp2  = nchoosek(tmp,ll);
        cosnJ2=zeros(size(tmp2,1),1);
        b=[];
        for jj = 1:size(tmp2,1)
            [cosnJ2(jj,1), b(:,jj)] = cosn([J(:,ii),J(:,tmp2(jj,:))]);
        end
        cosnJ(ii,ll) = max(cosnJ2(:,1));
        if cosnJ(ii,ll)>1.e-8,
            if ll>1 && ((cosnJ(ii,ll)-cosnJ(ii,ll-1))<1.e-8),
                pars{ii,ll} = [pars{ii,ll-1} NaN];
                cosnJ(ii,ll) = cosnJ(ii,ll-1);
            else
                tmp3 = tmp2(find(cosnJ2(:,1)==max(cosnJ2(:,1))),:);
                pars{ii,ll} = tmp3(1,:);
            end
        else
            pars{ii,ll} = NaN(1,ll);
        end
        dyn_waitbar(ii/k,h)
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
        for i=1:k,
            plist='';
            for ii=1:ll,
                if ~isnan(pars{i,ll}(ii)),
                    plist = [plist ' $' pnames_TeX(pars{i,ll}(ii),:) '\;\; $ '];
                else
                    plist = [plist ' ---- '];
                end
            end
            fprintf(fidTeX,'$%s$ & [%s] & %7.3f \\\\ \n',...
                    pnames_TeX(i,:),...
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