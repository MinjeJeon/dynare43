// Copyright (C) 2005-2011, Ondra Kamenik

// This is the mexFunction providing interface to
// DecisionRule<>::simulate(). It takes the following input
// parameters:
//      order    the order of approximation, needs order+1 derivatives
//      nstat
//      npred
//      nboth
//      nforw
//      nexog
//      ystart   starting value (full vector of endogenous)
//      shocks   matrix of shocks (nexog x number of period)
//      vcov     covariance matrix of shocks (nexog x nexog)
//      seed     integer seed
//      ysteady  full vector of decision rule's steady
//      ...      order+1 matrices of derivatives

// output:
//      res      simulated results

#include "dynmex.h"
#include "mex.h"

#include "decision_rule.hh"
#include "fs_tensor.hh"
#include "SylvException.hh"

#include <string>

extern "C" {
  void
  mexFunction(int nlhs, mxArray *plhs[],
              int nhrs, const mxArray *prhs[])
  {
    if (nhrs < 12 || nlhs != 2)
      DYN_MEX_FUNC_ERR_MSG_TXT("dynare_simul_ must have at least 12 input parameters and exactly 2 output arguments.\n");

    int order = static_cast<int>(mxGetScalar(prhs[0]));
    if (nhrs != 12 + order)
      DYN_MEX_FUNC_ERR_MSG_TXT("dynare_simul_ must have exactly 11+order input parameters.\n");

    int nstat = static_cast<int>(mxGetScalar(prhs[1]));
    int npred = static_cast<int>(mxGetScalar(prhs[2]));
    int nboth = static_cast<int>(mxGetScalar(prhs[3]));
    int nforw = static_cast<int>(mxGetScalar(prhs[4]));
    int nexog = static_cast<int>(mxGetScalar(prhs[5]));

    const mxArray *const ystart = prhs[6];
    const mxArray *const shocks = prhs[7];
    const mxArray *const vcov = prhs[8];
    int seed = static_cast<int>(mxGetScalar(prhs[9]));
    const mxArray *const ysteady = prhs[10];
    const mwSize *const ystart_dim = mxGetDimensions(ystart);
    const mwSize *const shocks_dim = mxGetDimensions(shocks);
    const mwSize *const vcov_dim = mxGetDimensions(vcov);
    const mwSize *const ysteady_dim = mxGetDimensions(ysteady);

    int ny = nstat + npred + nboth + nforw;
    if (ny != static_cast<int>(ystart_dim[0]))
      DYN_MEX_FUNC_ERR_MSG_TXT("ystart has wrong number of rows.\n");
    if (1 != ystart_dim[1])
      DYN_MEX_FUNC_ERR_MSG_TXT("ystart has wrong number of cols.\n");
    int nper = shocks_dim[1];
    if (nexog != static_cast<int>(shocks_dim[0]))
      DYN_MEX_FUNC_ERR_MSG_TXT("shocks has a wrong number of rows.\n");
    if (nexog != static_cast<int>(vcov_dim[0]))
      DYN_MEX_FUNC_ERR_MSG_TXT("vcov has a wrong number of rows.\n");
    if (nexog != static_cast<int>(vcov_dim[1]))
      DYN_MEX_FUNC_ERR_MSG_TXT("vcov has a wrong number of cols.\n");
    if (ny != static_cast<int>(ysteady_dim[0]))
      DYN_MEX_FUNC_ERR_MSG_TXT("ysteady has wrong number of rows.\n");
    if (1 != ysteady_dim[1])
      DYN_MEX_FUNC_ERR_MSG_TXT("ysteady has wrong number of cols.\n");

    mxArray *res = mxCreateDoubleMatrix(ny, nper, mxREAL);

    try
      {
        // initialize tensor library
        TLStatic::init(order, npred+nboth+nexog);

        // form the polynomial
        UTensorPolynomial pol(ny, npred+nboth+nexog);
        for (int dim = 0; dim <= order; dim++)
          {
            const mxArray *gk = prhs[11+dim];
            const mwSize *const gk_dim = mxGetDimensions(gk);
            FFSTensor ft(ny, npred+nboth+nexog, dim);
            if (ft.ncols() != static_cast<int>(gk_dim[1]))
              DYN_MEX_FUNC_ERR_MSG_TXT("Wrong number of columns for folded tensor: got " + std::to_string(gk_dim[1]) + " but i want " + std::to_string(ft.ncols()) + '\n');
            if (ft.nrows() != static_cast<int>(gk_dim[0]))
              DYN_MEX_FUNC_ERR_MSG_TXT("Wrong number of rows for folded tensor: got " + std::to_string(gk_dim[0]) + " but i want " + std::to_string(ft.nrows()) + '\n');
            ft.zeros();
            ConstTwoDMatrix gk_mat(ft.nrows(), ft.ncols(), ConstVector{gk});
            ft.add(1.0, gk_mat);
            pol.insert(std::make_unique<UFSTensor>(ft));
          }
        // form the decision rule
        UnfoldDecisionRule dr(pol, PartitionY(nstat, npred, nboth, nforw),
                              nexog, ConstVector{ysteady});
        // form the shock realization
        ConstTwoDMatrix shocks_mat(nexog, nper, ConstVector{shocks});
        ConstTwoDMatrix vcov_mat(nexog, nexog, ConstVector{vcov});
        GenShockRealization sr(vcov_mat, shocks_mat, seed);
        // simulate and copy the results
        TwoDMatrix res_mat{dr.simulate(DecisionRule::emethod::horner, nper,
                                       ConstVector{ystart}, sr)};
        TwoDMatrix res_tmp_mat{ny, nper, Vector{res}};
        res_tmp_mat = const_cast<const TwoDMatrix &>(res_mat);
        plhs[1] = res;
      }
    catch (const KordException &e)
      {
        DYN_MEX_FUNC_ERR_MSG_TXT("Caugth Kord exception.");
      }
    catch (const TLException &e)
      {
        DYN_MEX_FUNC_ERR_MSG_TXT("Caugth TL exception.");
      }
    catch (SylvException &e)
      {
        DYN_MEX_FUNC_ERR_MSG_TXT("Caught Sylv exception.");
      }
    plhs[0] = mxCreateDoubleScalar(0);
  }
};
