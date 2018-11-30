/*
 * Copyright (C) 2008-2017 Dynare Team
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

#ifndef _DYNAMIC_DLL_HH
#define _DYNAMIC_DLL_HH

#if defined(_WIN32) || defined(__CYGWIN32__)
# ifndef NOMINMAX
#  define NOMINMAX // Do not define "min" and "max" macros
# endif
# include <windows.h>
#else
# include <dlfcn.h> // unix/linux DLL (.so) handling routines
#endif

#include <string>

#include "dynamic_abstract_class.hh"
#include "dynare_exception.h"

using dynamic_tt_fct = void (*)(const double *y, const double *x, int nb_row_x, const double *params, const double *steady_state, int it_, double *T);
using dynamic_resid_fct = void (*) (const double *y, const double *x, int nb_row_x, const double *params, const double *steady_state, int it_, const double *T, double *residual);
using dynamic_g1_fct = void (*)(const double *y, const double *x, int nb_row_x, const double *params, const double *steady_state, int it_, const double *T, double *g1);
using dynamic_g2_fct = void (*)(const double *y, const double *x, int nb_row_x, const double *params, const double *steady_state, int it_, const double *T, double *v2);
using dynamic_g3_fct = void (*)(const double *y, const double *x, int nb_row_x, const double *params, const double *steady_state, int it_, const double *T, double *v3);

/**
 * creates pointer to Dynamic function inside <model>_dynamic.dll
 * and handles calls to it.
 **/
class DynamicModelDLL : public DynamicModelAC
{
private:
  int *ntt;
  dynamic_tt_fct dynamic_resid_tt, dynamic_g1_tt, dynamic_g2_tt, dynamic_g3_tt;
  dynamic_resid_fct dynamic_resid;
  dynamic_g1_fct dynamic_g1;
  dynamic_g2_fct dynamic_g2;
  dynamic_g3_fct dynamic_g3;
#if defined(_WIN32) || defined(__CYGWIN32__)
  HINSTANCE dynamicHinstance;  // DLL instance pointer in Windows
#else
  void *dynamicHinstance; // and in Linux or Mac
#endif

public:
  // construct and load Dynamic model DLL
  explicit DynamicModelDLL(const string &fname) noexcept(false);
  virtual ~DynamicModelDLL();

  void eval(const Vector &y, const Vector &x, const Vector &params, const Vector &ySteady,
            Vector &residual, TwoDMatrix *g1, TwoDMatrix *g2, TwoDMatrix *g3) noexcept(false);
};
#endif
