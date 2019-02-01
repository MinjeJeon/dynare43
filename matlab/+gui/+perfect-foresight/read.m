function di_perfect_foresight()

	global M_ options_ oo_ ys0_ ex0_

	%remove this instruction when proper path is found
	addpath jsonlab-1.5

	disp('di_perfect_foresight performing Matlab tasks')

	%loading JSON
	jm = loadjson('perforin.JSON','SimplifyCell',0);
	runflag=1;
	data2json=struct();

	%We test if jsonload loads string or char
	% jsl=length(class(jm.jsontest{1,1}));

	% INITVAL instructions
	%we initialize exogenous shocks to zero and compute initial ss
	options_.initval_file = 0;
	for nexo = 1:jm.exonum
		oo_.exo_steady_state( nexo ) = 0;
	end
	if M_.exo_nbr > 0
		oo_.exo_simul = ones(M_.maximum_lag,1)*oo_.exo_steady_state';
	end
	if M_.exo_det_nbr > 0
		oo_.exo_det_simul = ones(M_.maximum_lag,1)*oo_.exo_det_steady_state';
	end
	steady;
	data2json.steady_state1=oo_.steady_state;


	% ENDVAL instructions
	%we initialize exogenous shocks to zero and compute final ss unless there is a permanent shock
	ys0_= oo_.steady_state;
	ex0_ = oo_.exo_steady_state;
	if jm.permanentshockexist==0
		for nexo = 1:jm.exonum
				oo_.exo_steady_state( nexo ) = 0;
		end
	else
		for exoiter = 1:length(jm.permanentshocksdescription)
			currentshock=jm.permanentshocksdescription(exoiter);
			oo_.exo_steady_state(currentshock{1}.shockindex+1) = currentshock{1}.shockvalue;
			if (currentshock{1}.shockstartperiod)>1
				%in case the permanent shock does not start at the initial period, we add a shocks block to mask the unnecessary periods
				M_.det_shocks = [ M_.det_shocks;struct('exo_det',0,'exo_id',(currentshock{1}.shockindex+1),'multiplicative',0,'periods',1:(currentshock{1}.shockstartperiod-1),'value',0.0) ];
			end
		end
	end
	steady;
	savedpermanentSS=oo_.steady_state;
	data2json.steady_state2=oo_.steady_state;


	if jm.transitoryshockexist==1
		% SHOCKS instructions (for transitory shocks)
		for exotriter = 1:length(jm.shocksdescription)
			currenttrshock=jm.shocksdescription(exotriter);
			% disp(currenttrshock{1}.shockname);
			% disp(class(currenttrshock{1}.shockstartperiod));
			M_.det_shocks = [ M_.det_shocks;struct('exo_det',0,'exo_id',(currenttrshock{1}.shockindex+1),'multiplicative',0,'periods',currenttrshock{1}.shockstartperiod:currenttrshock{1}.shockendperiod,'value',currenttrshock{1}.shockvalue) ];
			%oo_.exo_steady_state(str2num(currentshock{1}.shockindex)+1) = currentshock{1}.shockvalue;
		end
		M_.exo_det_length = 0;
	end

	if ((jm.nonanticipatedshockexist==1) || (jm.delayexist==1))

		nonanticip=jm.nonanticipmatrix;
		rowindex=1;
		firstsimul=0;
		permanentnonanticip=0;

		while nonanticip{rowindex}{1}>0

			currentperiod=nonanticip{rowindex}{1};

			if currentperiod==1
				%there are nonanticipated shocks to add at first period
				if nonanticip{rowindex}{4}==0
					%this is a current nonanticipated shock
					M_.det_shocks = [ M_.det_shocks;struct('exo_det',0,'exo_id',(nonanticip{rowindex}{2}+1),'multiplicative',0,'periods',1:1,'value',nonanticip{rowindex}{7}) ];
				else
					%this is a delayed nonanticipated shock
					M_.det_shocks = [ M_.det_shocks;struct('exo_det',0,'exo_id',(nonanticip{rowindex}{2}+1),'multiplicative',0,'periods',(nonanticip{rowindex}{5}):(nonanticip{rowindex}{6}),'value',nonanticip{rowindex}{7}) ];
				end
				if nonanticip{rowindex+1}{1}~=currentperiod
					%when we have tracked all first period shocks we can simulate
					options_.periods = jm.simperiods;
					yy=oo_.steady_state;
					perfect_foresight_setup;
					[rowexo,colexo]=size(oo_.exo_simul);
					perfect_foresight_solver;

					if nonanticip{rowindex+1}{1}>0
						%we collect all the path from ooendo period 1 to just before the next shock...
						yy=[yy,oo_.endo_simul(:,2:(2+(nonanticip{rowindex+1}{1}-currentperiod-1)))];
					else
						%... or if there are no more shocks we collect the whole path
						yy=[yy,oo_.endo_simul(:,2:end)];
					end

					ooexosaved=oo_.exo_simul;
					firstsimul=1;
				end
			else
				%currentperiod is larger than one: we first perform perfect foresight simulation with initial period 1 conditions
				if firstsimul==0
					%Initializing the first simulation
					options_.periods = jm.simperiods;
					yy=oo_.steady_state;
					perfect_foresight_setup;
					[rowexo,colexo]=size(oo_.exo_simul);
					perfect_foresight_solver;

					%In this because there is at least one shock we did not consider yet in the first period, we only save the path from the beginning up the period just before the current
					yy=[yy,oo_.endo_simul(:,2:currentperiod)];
					ooexosaved=oo_.exo_simul;
					firstsimul=1;
				end



				if nonanticip{rowindex}{3}==1
					%this is a permanent shock
					oo_.exo_steady_state((nonanticip{rowindex}{2}+1)) = nonanticip{rowindex}{7};
					steady;
					savedpermanentSS=oo_.steady_state;
					data2json.steady_state2=oo_.steady_state;
					permanentnonanticip=1;

					if nonanticip{rowindex}{4}==0
						%this is a current permanent nonanticipated shock
						ooexosaved((currentperiod+1):end,(nonanticip{rowindex}{2}+1))=nonanticip{rowindex}{7};
					else
						%this is a delayed permanent nonanticipated shock
						ooexosaved((nonanticip{rowindex}{5}+1):end,(nonanticip{rowindex}{2}+1))=nonanticip{rowindex}{7};
					end

				else
					%this is not a permanent shock
					%we add new shocks in the saved timepath with original time indexes
					if nonanticip{rowindex}{4}==0
						%this is a single current nonanticipated shock
						ooexosaved(currentperiod+1,(nonanticip{rowindex}{2}+1))=nonanticip{rowindex}{7};
						%oo_.exo_simul(2,(nonanticip{rowindex}{2}+1))=nonanticip{rowindex}{7};
					else
						%this is a delayed nonanticipated shock
						ooexosaved((nonanticip{rowindex}{5}+1):(nonanticip{rowindex}{6}+1),(nonanticip{rowindex}{2}+1))=nonanticip{rowindex}{7};
						%oo_.exo_simul((nonanticip{rowindex}{5}+1):(nonanticip{rowindex}{6}+1),(nonanticip{rowindex}{2}+1))=nonanticip{rowindex}{7};
					end
				end

				%we copy only the necessary window in oo_.exo_simul
				%oo_.exo_simul=ooexosaved((currentperiod+1):end,:);
				oo_.exo_simul=[zeros(1,colexo);ooexosaved((currentperiod+1):end,:)];

				[ooexolength,dummy]=size(oo_.exo_simul);
				%we fill oo_.exo_simul until it has the correct size depending on of there are permanent shocks or not
				if jm.permanentshockexist==1
					%if there is a permanent shock we fill with last value of ooexosaved
					%oo_.exo_simul((length(ooexosaved((currentperiod+1):end,:))+1):rowexo,:)=ones(rowexo-(length(ooexosaved((currentperiod+1):end,:))+1)+1,1)*ooexosaved(end,:);
					%oo_.exo_simul((ooexolength+1):rowexo,:)=ones(rowexo-ooexolength,1)*ooexosaved(end,:);
					oo_.exo_simul=[oo_.exo_simul;ones(rowexo-ooexolength,1)*ooexosaved(end,:)];
				else
					%otherwise we fill with zeros
					%oo_.exo_simul((length(ooexosaved((currentperiod+1):end,:))+1):rowexo,:)=zeros(rowexo-(length(ooexosaved((currentperiod+1):end,:))+1)+1,colexo);
					%oo_.exo_simul((ooexolength+1):rowexo,:)=zeros(rowexo-ooexolength,colexo);
					oo_.exo_simul=[oo_.exo_simul;zeros(rowexo-ooexolength,colexo)];
				end

				if nonanticip{rowindex+1}{1}~=currentperiod
					%when we have tracked all the non-anticipated/delayed shocks for the current period, we can simulate


					if jm.permanentshockexist==1
						%if there are permanent shocks we fill oo_.endo with finalSS
						oo_.endo_simul=savedpermanentSS*ones(1,options_.periods+2);
					else
						%no permanent shocks we fill oo_.endo with initialSS
						oo_.endo_simul=oo_.steady_state*ones(1,options_.periods+2);
					end

					%we need to change oo_.endo_simul first value that gives the initial state of the economy
					oo_.endo_simul(:,1)=yy(:,end);


					perfect_foresight_solver;

					if nonanticip{rowindex+1}{1}>0
						%we collect all the path from ooendo period 1 to just before the next shock...
						yy=[yy,oo_.endo_simul(:,2:(2+(nonanticip{rowindex+1}{1}-currentperiod-1)))]
					else
						%... or if there are no more shocks we collect the whole path
						yy=[yy,oo_.endo_simul(:,2:end)];
					end
				end

			end

			rowindex=rowindex+1;
		end %	while jm.nonanticipmatrix{rowindex}{1}>0

		%we copy the endo path back
		oo_.endo_simul=yy;

	else
		%if there are no unanticipated shocks we perform the simulation
		options_.periods = jm.simperiods;
		perfect_foresight_setup;
		perfect_foresight_solver;
	end


	plotlgt=length(oo_.endo_simul);
	data2json.endosimul_length=plotlgt;
	data2json.endo_names=char(M_.endo_names);
	data2json.endo_nbr=M_.endo_nbr;
	for nendo = 1:M_.endo_nbr
		data2json.endo_simul.(strtrim(char(M_.endo_names(nendo,:))))=oo_.endo_simul(nendo,:);
	end
	data2json.endo_simul.plotx=[0:1:plotlgt];
	savejson('',data2json,'perforout.JSON');

return;
