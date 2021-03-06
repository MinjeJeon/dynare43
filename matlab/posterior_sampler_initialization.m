function [ ix2, ilogpo2, ModelName, MetropolisFolder, FirstBlock, FirstLine, npar, NumberOfBlocks, nruns, NewFile, MAX_nruns, d, bayestopt_] = ...
    posterior_sampler_initialization(TargetFun, xparam1, vv, mh_bounds,dataset_,dataset_info,options_,M_,estim_params_,bayestopt_,oo_)
% function [ ix2, ilogpo2, ModelName, MetropolisFolder, FirstBlock, FirstLine, npar, NumberOfBlocks, nruns, NewFile, MAX_nruns, d, bayestopt_] = ...
%     metropolis_hastings_initialization(TargetFun, xparam1, vv, mh_bounds,dataset_,dataset_info,options_,M_,estim_params_,bayestopt_,oo_)
% Metropolis-Hastings initialization.
%
% INPUTS
%   o TargetFun  [char]     string specifying the name of the objective
%                           function (posterior kernel).
%   o xparam1    [double]   (p*1) vector of parameters to be estimated (initial values).
%   o vv         [double]   (p*p) matrix, posterior covariance matrix (at the mode).
%   o mh_bounds  [double]   (p*2) matrix defining lower and upper bounds for the parameters.
%   o dataset_              data structure
%   o dataset_info          dataset info structure
%   o options_              options structure
%   o M_                    model structure
%   o estim_params_         estimated parameters structure
%   o bayestopt_            estimation options structure
%   o oo_                   outputs structure
%
% OUTPUTS
%   o ix2                   [double]   (NumberOfBlocks*npar) vector of starting points for different chains
%   o ilogpo2               [double]   (NumberOfBlocks*1) vector of initial posterior values for different chains
%   o ModelName             [string]    name of the mod-file
%   o MetropolisFolder      [string]    path to the Metropolis subfolder
%   o FirstBlock            [scalar]    number of the first MH chain to be run (not equal to 1 in case of recovery)
%   o FirstLine             [double]   (NumberOfBlocks*1) vector of first draw in each chain (not equal to 1 in case of recovery)
%   o npar                  [scalar]    number of parameters estimated
%   o NumberOfBlocks        [scalar]    Number of MCM chains requested
%   o nruns                 [double]   (NumberOfBlocks*1) number of draws in each chain
%   o NewFile               [scalar]    (NumberOfBlocks*1) vector storing the number
%                                       of the first MH-file to created for each chain when saving additional
%                                       draws
%   o MAX_nruns             [scalar]    maximum number of draws in each MH-file on the harddisk
%   o d                     [double]   (p*p) matrix, Cholesky decomposition of the posterior covariance matrix (at the mode).
%   o bayestopt_            [structure] estimation options structure
%
% SPECIAL REQUIREMENTS
%   None.

% Copyright (C) 2006-2017 Dynare Team
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
% along with Dynare.  If not, see <https://www.gnu.org/licenses/>.

%Initialize outputs
ix2 = [];
ilogpo2 = [];
ModelName = [];
MetropolisFolder = [];
FirstBlock = [];
FirstLine = [];
npar = [];
NumberOfBlocks = [];
nruns = [];
NewFile = [];
MAX_nruns = [];
d = [];

ModelName = M_.fname;
if ~isempty(M_.bvar)
    ModelName = [ModelName '_bvar'];
end

MetropolisFolder = CheckPath('metropolis',M_.dname);
BaseName = [MetropolisFolder filesep ModelName];

NumberOfBlocks = options_.mh_nblck;
nruns = ones(NumberOfBlocks,1)*options_.mh_replic;
npar  = length(xparam1);
MAX_nruns = ceil(options_.MaxNumberOfBytes/(npar+2)/8);
d = chol(vv);

if ~options_.load_mh_file && ~options_.mh_recover
    % Here we start a new Metropolis-Hastings, previous draws are discarded.
    if NumberOfBlocks > 1
        disp('Estimation::mcmc: Multiple chains mode.')
    else
        disp('Estimation::mcmc: One Chain mode.')
    end
    % Delete old mh files if any...
    files = dir([BaseName '_mh*_blck*.mat']);
    if length(files)
        delete([BaseName '_mh*_blck*.mat']);
        disp('Estimation::mcmc: Old mh-files successfully erased!')
    end
    % Delete old Metropolis log file.
    file = dir([ MetropolisFolder '/metropolis.log']);
    if length(file)
        delete([ MetropolisFolder '/metropolis.log']);
        disp('Estimation::mcmc: Old metropolis.log file successfully erased!')
        disp('Estimation::mcmc: Creation of a new metropolis.log file.')
    end
    fidlog = fopen([MetropolisFolder '/metropolis.log'],'w');
    fprintf(fidlog,'%% MH log file (Dynare).\n');
    fprintf(fidlog,['%% ' datestr(now,0) '.\n']);
    fprintf(fidlog,' \n\n');
    fprintf(fidlog,'%% Session 1.\n');
    fprintf(fidlog,' \n');
    fprintf(fidlog,['  Number of blocks...............: ' int2str(NumberOfBlocks) '\n']);
    fprintf(fidlog,['  Number of simulations per block: ' int2str(nruns(1)) '\n']);
    fprintf(fidlog,' \n');
    if isempty(d)
        prior_draw(bayestopt_,options_.prior_trunc);
    end
    if options_.mh_initialize_from_previous_mcmc.status
        PreviousFolder0 = options_.mh_initialize_from_previous_mcmc.directory;
        RecordFile0 = options_.mh_initialize_from_previous_mcmc.record;
        PriorFile0 = options_.mh_initialize_from_previous_mcmc.prior;
        if ~isempty(RecordFile0)
            %% check for proper filesep char in user defined paths
            RecordFile0=strrep(RecordFile0,'\',filesep);
            if isempty(dir(RecordFile0))
                disp('Estimation::mcmc: wrong value for mh_initialize_from_previous_mcmc_record option')
                error('Estimation::mcmc: path to record file is not found')
            else
                record0=load(RecordFile0);
            end
            record0=record0.record;
            MetropolisFolder0 = fileparts(RecordFile0);
            PreviousFolder0=fileparts(MetropolisFolder0);
            [~, ModelName0]=fileparts(PreviousFolder0);
        else            
            %% check for proper filesep char in user defined paths
            PreviousFolder0=strrep(PreviousFolder0,'\',filesep);
            MetropolisFolder0 = [PreviousFolder0 filesep 'metropolis'];
            [~, ModelName0]=fileparts(PreviousFolder0);
            record0=load_last_mh_history_file(MetropolisFolder0, ModelName0);
        end
        if ~isnan(record0.MCMCConcludedSuccessfully) && ~record0.MCMCConcludedSuccessfully
            error('Estimation::mcmc: You are trying to load an MCMC that did not finish successfully. Please use mh_recover.')
        end
%         mh_files = dir([ MetropolisFolder0 filesep ModelName0 '_mh*.mat']);
%         if ~length(mh_files)
%             error('Estimation::mcmc: I cannot find any MH file to load here!')
%         end
        disp('Estimation::mcmc: Initializing from past Metropolis-Hastings simulations...')
        disp(['Estimation::mcmc: Past MH path ' MetropolisFolder0 ])
        disp(['Estimation::mcmc: Past model name ' ModelName0 ])
        fprintf(fidlog,'  Loading initial values from previous MH\n');
        fprintf(fidlog,'  Past MH path: %s\n', MetropolisFolder0 );
        fprintf(fidlog,'  Past model name: %s\n', ModelName0);
        fprintf(fidlog,' \n');
        past_number_of_blocks = record0.Nblck;
        if past_number_of_blocks ~= NumberOfBlocks
            disp('Estimation::mcmc: The specified number of blocks doesn''t match with the previous number of blocks!')
            disp(['Estimation::mcmc: You declared ' int2str(NumberOfBlocks) ' blocks, but the previous number of blocks was ' int2str(past_number_of_blocks) '.'])
            disp(['Estimation::mcmc: I will run the Metropolis-Hastings with ' int2str(past_number_of_blocks) ' blocks.' ])
            NumberOfBlocks = past_number_of_blocks;
            options_.mh_nblck = NumberOfBlocks;
        end
        if ~isempty(PriorFile0)
            if isempty(dir(PriorFile0))
                disp('Estimation::mcmc: wrong value for mh_initialize_from_previous_mcmc_prior option')
                error('Estimation::mcmc: path to prior file is not found')
            else
                bayestopt0 = load(PriorFile0);
            end
        else
            bayestopt0 = load([PreviousFolder0 filesep 'prior' filesep 'definition.mat']);
        end
        [common_parameters,IA,IB] = intersect(bayestopt_.name,bayestopt0.bayestopt_.name);
        new_estimated_parameters = ~ismember(bayestopt_.name,bayestopt0.bayestopt_.name);
        ix2 = zeros(NumberOfBlocks,npar);
        ilogpo2 = zeros(NumberOfBlocks,1);
        ilogpo2(:,1) = record0.LastLogPost;
        ix2(:,IA) = record0.LastParameters(:,IB);
    else
        new_estimated_parameters = true(1,npar);
    end
    % Find initial values for the NumberOfBlocks chains...
    if NumberOfBlocks > 1 || options_.mh_initialize_from_previous_mcmc.status% Case 1: multiple chains
        set_dynare_seed('default');
        fprintf(fidlog,['  Initial values of the parameters:\n']);
        disp('Estimation::mcmc: Searching for initial values...')
        if ~options_.mh_initialize_from_previous_mcmc.status
            ix2 = zeros(NumberOfBlocks,npar);
            ilogpo2 = zeros(NumberOfBlocks,1);
        end
        for j=1:NumberOfBlocks
            validate    = 0;
            init_iter   = 0;
            trial = 1;
            while validate == 0 && trial <= 10
                if isempty(d)
                    candidate = prior_draw();
                else
                    candidate = rand_multivariate_normal( transpose(xparam1), d * options_.mh_init_scale, npar);
                end
                if all(candidate(:) >= mh_bounds.lb) && all(candidate(:) <= mh_bounds.ub)
                    ix2(j,new_estimated_parameters) = candidate(new_estimated_parameters);
                    ilogpo2(j) = - feval(TargetFun,ix2(j,:)',dataset_,dataset_info,options_,M_,estim_params_,bayestopt_,mh_bounds,oo_);
                    if ~isfinite(ilogpo2(j)) % if returned log-density is
                                             % Inf or Nan (penalized value)
                        validate = 0;
                    else
                        fprintf(fidlog,['    Blck ' int2str(j) ':\n']);
                        for i=1:length(ix2(1,:))
                            fprintf(fidlog,['      params:' int2str(i) ': ' num2str(ix2(j,i)) '\n']);
                        end
                        fprintf(fidlog,['      logpo2: ' num2str(ilogpo2(j)) '\n']);
                        j = j+1;
                        validate = 1;
                    end
                end
                init_iter = init_iter + 1;
                if init_iter > 100 && validate == 0
                    disp(['Estimation::mcmc: I couldn''t get a valid initial value in 100 trials.'])
                    if options_.nointeractive
                        disp(['Estimation::mcmc: I reduce mh_init_scale by 10 percent:'])
                        options_.mh_init_scale = .9*options_.mh_init_scale;
                        disp(sprintf('Estimation::mcmc: Parameter mh_init_scale is now equal to %f.',options_.mh_init_scale))
                    else
                        disp(['Estimation::mcmc: You should reduce mh_init_scale...'])
                        disp(sprintf('Estimation::mcmc: Parameter mh_init_scale is equal to %f.',options_.mh_init_scale))
                        options_.mh_init_scale = input('Estimation::mcmc: Enter a new value...  ');
                    end
                    trial = trial+1;
                end
            end
            if trial > 10 && ~validate
                disp(['Estimation::mcmc: I''m unable to find a starting value for block ' int2str(j)])
                fclose(fidlog);
                return
            end
        end
        fprintf(fidlog,' \n');
        disp('Estimation::mcmc: Initial values found!')
        skipline()
    else% Case 2: one chain (we start from the posterior mode)
        fprintf(fidlog,['  Initial values of the parameters:\n']);
        candidate = transpose(xparam1(:));%
        if all(candidate(:) >= mh_bounds.lb) && all(candidate(:) <= mh_bounds.ub)
            ix2 = candidate;
            ilogpo2 = - feval(TargetFun,ix2',dataset_,dataset_info,options_,M_,estim_params_,bayestopt_,mh_bounds,oo_);
            disp('Estimation::mcmc: Initialization at the posterior mode.')
            skipline()
            fprintf(fidlog,['    Blck ' int2str(1) 'params:\n']);
            for i=1:length(ix2(1,:))
                fprintf(fidlog,['      ' int2str(i)  ':' num2str(ix2(1,i)) '\n']);
            end
            fprintf(fidlog,['    Blck ' int2str(1) 'logpo2:' num2str(ilogpo2) '\n']);
        else
            disp('Estimation::mcmc: Initialization failed...')
            disp('Estimation::mcmc: The posterior mode lies outside the prior bounds.')
            fclose(fidlog);
            return
        end
        fprintf(fidlog,' \n');
    end
    fprintf(fidlog,' \n');
    FirstBlock = 1;
    FirstLine = ones(NumberOfBlocks,1);
    NewFile = ones(NumberOfBlocks,1);
    % Delete the mh-history files
    delete_mh_history_files(MetropolisFolder, ModelName);
    %  Create a new record structure
    fprintf(['Estimation::mcmc: Write details about the MCMC... ']);
    AnticipatedNumberOfFiles = ceil(nruns(1)/MAX_nruns);
    AnticipatedNumberOfLinesInTheLastFile = nruns(1) - (AnticipatedNumberOfFiles-1)*MAX_nruns;
    record.Sampler = options_.posterior_sampler_options.posterior_sampling_method;
    record.Nblck = NumberOfBlocks;
    record.MhDraws = zeros(1,3);
    record.MhDraws(1,1) = nruns(1);
    record.MhDraws(1,2) = AnticipatedNumberOfFiles;
    record.MhDraws(1,3) = AnticipatedNumberOfLinesInTheLastFile;
    record.MAX_nruns=MAX_nruns;
    record.AcceptanceRatio = zeros(1,NumberOfBlocks);
    record.FunctionEvalPerIteration = NaN(1,NumberOfBlocks);
    if options_.mh_initialize_from_previous_mcmc.status
        record.InitialSeeds = record0.LastSeeds;
    else
        
        for j=1:NumberOfBlocks
            % we set a different seed for the random generator for each block then we record the corresponding random generator state (vector)
            set_dynare_seed(options_.DynareRandomStreams.seed+j);
            % record.Seeds keeps a vector of the random generator state and not the scalar seed despite its name
            [record.InitialSeeds(j).Unifor,record.InitialSeeds(j).Normal] = get_dynare_random_generator_state();
        end
    end
    record.InitialParameters = ix2;
    record.InitialLogPost = ilogpo2;
    record.LastParameters = zeros(NumberOfBlocks,npar);
    record.LastLogPost = zeros(NumberOfBlocks,1);
    record.LastFileNumber = AnticipatedNumberOfFiles ;
    record.LastLineNumber = AnticipatedNumberOfLinesInTheLastFile;
    record.MCMCConcludedSuccessfully = 0;
    record.ProposalScaleVec=bayestopt_.jscale;
    fprintf('Ok!\n');
    id = write_mh_history_file(MetropolisFolder, ModelName, record);
    disp(['Estimation::mcmc: Details about the MCMC are available in ' BaseName '_mh_history_' num2str(id) '.mat'])
    skipline()
    fprintf(fidlog,['  CREATION OF THE MH HISTORY FILE!\n\n']);
    fprintf(fidlog,['    Expected number of files per block.......: ' int2str(AnticipatedNumberOfFiles) '.\n']);
    fprintf(fidlog,['    Expected number of lines in the last file: ' int2str(AnticipatedNumberOfLinesInTheLastFile) '.\n']);
    fprintf(fidlog,['\n']);
    for j = 1:NumberOfBlocks
        fprintf(fidlog,['    Initial state of the Gaussian random number generator for chain number ',int2str(j),':\n']);
        for i=1:length(record.InitialSeeds(j).Normal)
            fprintf(fidlog,['      ' num2str(record.InitialSeeds(j).Normal(i)') '\n']);
        end
        fprintf(fidlog,['    Initial state of the Uniform random number generator for chain number ',int2str(j),':\n']);
        for i=1:length(record.InitialSeeds(j).Unifor)
            fprintf(fidlog,['      ' num2str(record.InitialSeeds(j).Unifor(i)') '\n']);
        end
    end
    fprintf(fidlog,' \n');
    fclose(fidlog);
elseif options_.load_mh_file && ~options_.mh_recover
    % Here we consider previous mh files (previous mh did not crash).
    disp('Estimation::mcmc: I am loading past Metropolis-Hastings simulations...')
    load_last_mh_history_file(MetropolisFolder, ModelName);
    if ~isnan(record.MCMCConcludedSuccessfully) && ~record.MCMCConcludedSuccessfully
        error('Estimation::mcmc: You are trying to load an MCMC that did not finish successfully. Please use mh_recover.')
    end
    record.MCMCConcludedSuccessfully=0; %reset indicator for this run
    mh_files = dir([ MetropolisFolder filesep ModelName '_mh*.mat']);
    if ~length(mh_files)
        error('Estimation::mcmc: I cannot find any MH file to load here!')
    end
    fidlog = fopen([MetropolisFolder '/metropolis.log'],'a');
    fprintf(fidlog,'\n');
    fprintf(fidlog,['%% Session ' int2str(length(record.MhDraws(:,1))+1) '.\n']);
    fprintf(fidlog,' \n');
    fprintf(fidlog,['  Number of blocks...............: ' int2str(NumberOfBlocks) '\n']);
    fprintf(fidlog,['  Number of simulations per block: ' int2str(nruns(1)) '\n']);
    fprintf(fidlog,' \n');
    past_number_of_blocks = record.Nblck;
    if past_number_of_blocks ~= NumberOfBlocks
        disp('Estimation::mcmc: The specified number of blocks doesn''t match with the previous number of blocks!')
        disp(['Estimation::mcmc: You declared ' int2str(NumberOfBlocks) ' blocks, but the previous number of blocks was ' int2str(past_number_of_blocks) '.'])
        disp(['Estimation::mcmc: I will run the Metropolis-Hastings with ' int2str(past_number_of_blocks) ' blocks.' ])
        NumberOfBlocks = past_number_of_blocks;
        options_.mh_nblck = NumberOfBlocks;
    end
    % I read the last line of the last mh-file for initialization of the new Metropolis-Hastings simulations:
    LastFileNumber = record.LastFileNumber;
    LastLineNumber = record.LastLineNumber;
    if LastLineNumber < MAX_nruns
        NewFile = ones(NumberOfBlocks,1)*LastFileNumber;
        FirstLine = ones(NumberOfBlocks,1)*(LastLineNumber+1);
    else
        NewFile = ones(NumberOfBlocks,1)*(LastFileNumber+1);
        FirstLine = ones(NumberOfBlocks,1);
    end
    ilogpo2 = record.LastLogPost;
    ix2 = record.LastParameters;
    [d,bayestopt_,record]=set_proposal_density_to_previous_value(record,options_,bayestopt_,d);
    FirstBlock = 1;
    NumberOfPreviousSimulations = sum(record.MhDraws(:,1),1);
    fprintf('Estimation::mcmc: I am writing a new mh-history file... ');
    record.MhDraws = [record.MhDraws;zeros(1,3)];
    NumberOfDrawsWrittenInThePastLastFile = MAX_nruns - LastLineNumber;
    NumberOfDrawsToBeSaved = nruns(1) - NumberOfDrawsWrittenInThePastLastFile;
    AnticipatedNumberOfFiles = ceil(NumberOfDrawsToBeSaved/MAX_nruns);
    AnticipatedNumberOfLinesInTheLastFile = NumberOfDrawsToBeSaved - (AnticipatedNumberOfFiles-1)*MAX_nruns;
    record.LastFileNumber = LastFileNumber + AnticipatedNumberOfFiles;
    record.LastLineNumber = AnticipatedNumberOfLinesInTheLastFile;
    record.MhDraws(end,1) = nruns(1);
    record.MhDraws(end,2) = AnticipatedNumberOfFiles;
    record.MhDraws(end,3) = AnticipatedNumberOfLinesInTheLastFile;
    record.MAX_nruns = [record.MAX_nruns;MAX_nruns];
    record.InitialSeeds = record.LastSeeds;
    write_mh_history_file(MetropolisFolder, ModelName, record);
    fprintf('Done.\n')
    disp(['Estimation::mcmc: Ok. I have loaded ' int2str(NumberOfPreviousSimulations) ' simulations.'])
    skipline()
    fclose(fidlog);
elseif options_.mh_recover
    % The previous metropolis-hastings crashed before the end! I try to recover the saved draws...
    disp('Estimation::mcmc: Recover mode!')
    load_last_mh_history_file(MetropolisFolder, ModelName);
    NumberOfBlocks = record.Nblck;% Number of "parallel" mcmc chains.
    options_.mh_nblck = NumberOfBlocks;

    %% check consistency of options
    if record.MhDraws(end,1)~=options_.mh_replic
        fprintf('\nEstimation::mcmc: You cannot specify a different mh_replic than in the chain you are trying to recover\n')
        fprintf('Estimation::mcmc: I am resetting mh_replic to %u\n',record.MhDraws(end,1))
        options_.mh_replic=record.MhDraws(end,1);
        nruns = ones(NumberOfBlocks,1)*options_.mh_replic;
    end

    if ~isnan(record.MAX_nruns(end,1)) %field exists
        if record.MAX_nruns(end,1)~=MAX_nruns
            fprintf('\nEstimation::mcmc: You cannot specify a different MaxNumberOfBytes than in the chain you are trying to recover\n')
            fprintf('Estimation::mcmc: I am resetting MAX_nruns to %u\n',record.MAX_nruns(end,1))
            MAX_nruns=record.MAX_nruns(end,1);
        end
    end
    %% do tentative initialization as if full last run of MCMC were to be redone
    if size(record.MhDraws,1) == 1
        OldMhExists = 0;% The crashed metropolis was the first session.
    else
        OldMhExists = 1;% The crashed metropolis wasn't the first session.
    end
    % Default initialization:
    if OldMhExists
        ilogpo2 = record.LastLogPost;
        ix2 = record.LastParameters;
    else
        ilogpo2 = record.InitialLogPost;
        ix2 = record.InitialParameters;
    end
    % Set NewFile, a NumberOfBlocks*1 vector of integers, and FirstLine (first line), a NumberOfBlocks*1 vector of integers.
    % Relevant for starting at the correct file and potentially filling a file from the previous run that was not yet full
    if OldMhExists
        LastLineNumberInThePreviousMh = record.MhDraws(end-1,3);% Number of lines in the last mh files of the previous session.
        LastFileNumberInThePreviousMh = sum(record.MhDraws(1:end-1,2),1);% Number of mh files in the the previous sessions.
                                                                         %Test if the last mh files of the previous session were not full yet
        if LastLineNumberInThePreviousMh < MAX_nruns%not full
                                                    %store starting point if whole chain needs to be redone
            NewFile = ones(NumberOfBlocks,1)*LastFileNumberInThePreviousMh;
            FirstLine = ones(NumberOfBlocks,1)*(LastLineNumberInThePreviousMh+1);
            LastFileFullIndicator=0;
        else% The last mh files of the previous session were full
            %store starting point if whole chain needs to be redone
            NewFile = ones(NumberOfBlocks,1)*(LastFileNumberInThePreviousMh+1);
            FirstLine = ones(NumberOfBlocks,1);
            LastFileFullIndicator=1;
        end
    else
        LastLineNumberInThePreviousMh = 0;
        LastFileNumberInThePreviousMh = 0;
        NewFile = ones(NumberOfBlocks,1);
        FirstLine = ones(NumberOfBlocks,1);
        LastFileFullIndicator=1;
    end
    if ~isequal(options_.posterior_sampler_options.posterior_sampling_method,'slice')
        [d,bayestopt_,record]=set_proposal_density_to_previous_value(record,options_,bayestopt_,d);
    end
    %% Now find out what exactly needs to be redone
    % 1. Check if really something needs to be done
    % How many mh files should we have ?
    ExpectedNumberOfMhFilesPerBlock = sum(record.MhDraws(:,2),1);
    ExpectedNumberOfMhFiles = ExpectedNumberOfMhFilesPerBlock*NumberOfBlocks;
    % How many mh files do we actually have ?
    AllMhFiles = dir([BaseName '_mh*_blck*.mat']);
    TotalNumberOfMhFiles = length(AllMhFiles)-length(dir([BaseName '_mh_tmp*_blck*.mat']));
    % Quit if no crashed mcmc chain can be found as there are as many files as expected
    if (TotalNumberOfMhFiles==ExpectedNumberOfMhFiles)
        if isnumeric(options_.parallel)
            disp('Estimation::mcmc: It appears that you don''t need to use the mh_recover option!')
            disp('                  You have to edit the mod file and remove the mh_recover option')
            disp('                  in the estimation command')
            error('Estimation::mcmc: mh_recover option not required!')
        end
    end
    % 2. Something needs to be done; find out what
    % Count the number of saved mh files per block.
    NumberOfMhFilesPerBlock = zeros(NumberOfBlocks,1);
    for b = 1:NumberOfBlocks
        BlckMhFiles = dir([BaseName '_mh*_blck' int2str(b) '.mat']);
        NumberOfMhFilesPerBlock(b) = length(BlckMhFiles)-length(dir([BaseName '_mh_tmp*_blck' int2str(b) '.mat']));
    end
    % Find FirstBlock (First block), an integer targeting the crashed mcmc chain.
    FirstBlock = 1; %initialize
    FBlock = zeros(NumberOfBlocks,1);
    while FirstBlock <= NumberOfBlocks
        if  NumberOfMhFilesPerBlock(FirstBlock) < ExpectedNumberOfMhFilesPerBlock
            disp(['Estimation::mcmc: Chain ' int2str(FirstBlock) ' is not complete!'])
            FBlock(FirstBlock)=1;
        else
            disp(['Estimation::mcmc: Chain ' int2str(FirstBlock) ' is complete!'])
        end
        FirstBlock = FirstBlock+1;
    end

    %% 3. Overwrite default settings for
    % How many mh-files are saved in this block?
    ExistingDrawsInLastMCFile=zeros(NumberOfBlocks,1); %initialize: no MCMC draws of current MCMC are in file from last run
    % Check whether last present file is a file included in the last MCMC run
    
    update_record=0;
    for k=1:NumberOfBlocks
        FirstBlock = k;
        if FBlock(k)
            NumberOfSavedMhFilesInTheCrashedBlck=NumberOfMhFilesPerBlock(k);
            if ~LastFileFullIndicator
                if NumberOfSavedMhFilesInTheCrashedBlck==NewFile(FirstBlock) %only that last file exists, but no files from current MCMC
                    loaded_results=load([BaseName '_mh' int2str(NewFile(FirstBlock)) '_blck' int2str(FirstBlock) '.mat']);
                    %check whether that last file was filled
                    if size(loaded_results.x2,1)==MAX_nruns %file is full
                        NewFile(FirstBlock)=NewFile(FirstBlock)+1; %set first file to be created to next one
                        FirstLine(FirstBlock) = 1; %use first line of next file
                        ExistingDrawsInLastMCFile(FirstBlock)=MAX_nruns-record.MhDraws(end-1,3);
                    else
                        ExistingDrawsInLastMCFile(FirstBlock)=0;
                    end
                end
            elseif LastFileFullIndicator
                ExistingDrawsInLastMCFile(FirstBlock)=0;
                if NumberOfSavedMhFilesInTheCrashedBlck==NewFile(FirstBlock) %only the last file exists, but no files from current MCMC
                    NewFile(FirstBlock)=NewFile(FirstBlock)+1; %set first file to be created to next one
                end
            end
            %     % Correct the number of saved mh files if the crashed Metropolis was not the first session (so
            %     % that NumberOfSavedMhFilesInTheCrashedBlck is the number of saved mh files in the crashed chain
            %     % of the current session).
            %     if OldMhExists
            %         NumberOfSavedMhFilesInTheCrashedBlck = NumberOfSavedMhFilesInTheCrashedBlck - LastFileNumberInThePreviousMh;
            %     end
            %     NumberOfSavedMhFiles = NumberOfSavedMhFilesInTheCrashedBlck+LastFileNumberInThePreviousMh;

            % Correct initial conditions.
            if NumberOfSavedMhFilesInTheCrashedBlck>0 && NumberOfSavedMhFilesInTheCrashedBlck<ExpectedNumberOfMhFilesPerBlock
                loaded_results=load([BaseName '_mh' int2str(NumberOfSavedMhFilesInTheCrashedBlck) '_blck' int2str(FirstBlock) '.mat']);
                ilogpo2(FirstBlock) = loaded_results.logpo2(end);
                ix2(FirstBlock,:) = loaded_results.x2(end,:);
                nruns(FirstBlock)=nruns(FirstBlock)-ExistingDrawsInLastMCFile(FirstBlock)-(NumberOfSavedMhFilesInTheCrashedBlck-LastFileNumberInThePreviousMh)*MAX_nruns;
                %reset seed if possible
                if isfield(loaded_results,'LastSeeds')
                    record.InitialSeeds(FirstBlock).Unifor=loaded_results.LastSeeds.(['file' int2str(NumberOfSavedMhFilesInTheCrashedBlck)]).Unifor;
                    record.InitialSeeds(FirstBlock).Normal=loaded_results.LastSeeds.(['file' int2str(NumberOfSavedMhFilesInTheCrashedBlck)]).Normal;
                else
                    fprintf('Estimation::mcmc: You are trying to recover a chain generated with an older Dynare version.\n')
                    fprintf('Estimation::mcmc: I am using the default seeds to continue the chain.\n')
                end
                if update_record
                    update_last_mh_history_file(MetropolisFolder, ModelName, record);
                else
                    write_mh_history_file(MetropolisFolder, ModelName, record);
                end
                update_record=1;
            end
        else
            loaded_results=load([BaseName '_mh' int2str(ExpectedNumberOfMhFilesPerBlock) '_blck' int2str(FirstBlock) '.mat']);
            ilogpo2(FirstBlock) = loaded_results.logpo2(end);
            ix2(FirstBlock,:) = loaded_results.x2(end,:);
            nruns(FirstBlock)=0;
            %reset seed if possible
            if isfield(loaded_results,'LastSeeds')
                record.LastSeeds(FirstBlock).Unifor=loaded_results.LastSeeds.(['file' int2str(ExpectedNumberOfMhFilesPerBlock)]).Unifor;
                record.LastSeeds(FirstBlock).Normal=loaded_results.LastSeeds.(['file' int2str(ExpectedNumberOfMhFilesPerBlock)]).Normal;
            else
                fprintf('Estimation::mcmc: You are trying to recover a chain generated with an older Dynare version.\n')
                fprintf('Estimation::mcmc: I am using the default seeds to continue the chain.\n')
            end
            if isfield(loaded_results,'accepted_draws_this_chain')
                record.AcceptanceRatio(FirstBlock)=loaded_results.accepted_draws_this_chain/record.MhDraws(end,1);
                record.FunctionEvalPerIteration(FirstBlock)=loaded_results.accepted_draws_this_chain/record.MhDraws(end,1);
            end
            record.LastLogPost(FirstBlock)=loaded_results.logpo2(end);
            record.LastParameters(FirstBlock,:)=loaded_results.x2(end,:);
            update_last_mh_history_file(MetropolisFolder, ModelName, record);
        end
    end
    FirstBlock = find(FBlock==1,1);
end

function [d,bayestopt_,record]=set_proposal_density_to_previous_value(record,options_,bayestopt_,d)
if ~options_.use_mh_covariance_matrix
    if isfield(record,'ProposalCovariance') && isfield(record,'ProposalScaleVec')
        fprintf('Estimation::mcmc: Recovering the previous proposal density.\n')
        d=record.ProposalCovariance;
        bayestopt_.jscale=record.ProposalScaleVec;
    else
        if ~isequal(options_.posterior_sampler_options.posterior_sampling_method,'slice')
            % pass through input d unaltered
            if options_.mode_compute~=0
                fprintf('Estimation::mcmc: No stored previous proposal density found, continuing with the one implied by mode_compute\n.');
            elseif ~isempty(options_.mode_file)
                fprintf('Estimation::mcmc: No stored previous proposal density found, continuing with the one implied by the mode_file\n.');
            else
                error('Estimation::mcmc: No stored previous proposal density found, no mode-finding conducted, and no mode-file provided. I don''t know how to continue')
            end
        end
    end
else
    % pass through input d unaltered
    fprintf('Estimation::mcmc: use_mh_covariance_matrix specified, continuing with proposal density implied by the previous MCMC run.\n.');
end

if isfield(record,'Sampler')
    if ~strcmp(record.Sampler,options_.posterior_sampler_options.posterior_sampling_method)
        warning('Estimation::mcmc: The posterior_sampling_method %s selected differs from the %s of the original chain. This may create problems with the convergence diagnostics.',options_.posterior_sampler_options.posterior_sampling_method,record.Sampler)
        record.Sampler=options_.posterior_sampler_options.posterior_sampling_method; %update sampler used
    end
end
