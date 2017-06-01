/*
 * Copyright (C) 2010-2013 Dynare Team
 *
 * This file is part of Dynare.
 *
 * Dynare is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Dynare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Dynare.  If not, see <http://www.gnu.org/licenses/>.
 */

///////////////////////////////////////////////////////////
//  InitializeKalmanFilter.cpp
//  Implementation of the Class InitializeKalmanFilter
//  Created on:      02-Feb-2010 12:25:28
///////////////////////////////////////////////////////////

#include "InitializeKalmanFilter.hh"
#include "BlasBindings.hh"

InitializeKalmanFilter::~InitializeKalmanFilter()
{
}

InitializeKalmanFilter::InitializeKalmanFilter(const std::string &basename, size_t n_endo_arg, size_t n_exo_arg,
                                               const std::vector<size_t> &zeta_fwrd_arg, const std::vector<size_t> &zeta_back_arg,
                                               const std::vector<size_t> &zeta_mixed_arg, const std::vector<size_t> &zeta_static_arg,
                                               const std::vector<size_t> &zeta_varobs_back_mixed_arg,
                                               const std::vector<size_t> &varobs_arg,
                                               double qz_criterium_arg,
                                               double lyapunov_tol_arg,
                                               bool noconstant_arg) :
  lyapunov_tol(lyapunov_tol_arg),
  zeta_varobs_back_mixed(zeta_varobs_back_mixed_arg),
  detrendData(varobs_arg, noconstant_arg),
  modelSolution(basename, n_endo_arg, n_exo_arg, zeta_fwrd_arg, zeta_back_arg,
                zeta_mixed_arg, zeta_static_arg, qz_criterium_arg),
  discLyapFast(zeta_varobs_back_mixed.size()),
  g_x(n_endo_arg, zeta_back_arg.size() + zeta_mixed_arg.size()),
  g_u(n_endo_arg, n_exo_arg),
  Rt(n_exo_arg, zeta_varobs_back_mixed.size()),
  RQ(zeta_varobs_back_mixed.size(), n_exo_arg)
{
  std::vector<size_t> zeta_back_mixed;
  set_union(zeta_back_arg.begin(), zeta_back_arg.end(),
            zeta_mixed_arg.begin(), zeta_mixed_arg.end(),
            back_inserter(zeta_back_mixed));
  for (size_t i = 0; i < zeta_back_mixed.size(); i++)
    pi_bm_vbm.push_back(find(zeta_varobs_back_mixed.begin(), zeta_varobs_back_mixed.end(),
                             zeta_back_mixed[i]) - zeta_varobs_back_mixed.begin());

}
void
InitializeKalmanFilter::setT(Matrix &T)
{
  // Initialize the empty columns of T to zero
  T.setAll(0.0);
  mat::assignByVectors(T, mat::nullVec, pi_bm_vbm, g_x, zeta_varobs_back_mixed, mat::nullVec);
}

void
InitializeKalmanFilter::setPstar(Matrix &Pstar, Matrix &Pinf, const Matrix &T, const Matrix &RQRt) throw (DiscLyapFast::DLPException)
{

  discLyapFast.solve_lyap(T, RQRt, Pstar, lyapunov_tol, 0);

  Pinf.setAll(0.0);
}
