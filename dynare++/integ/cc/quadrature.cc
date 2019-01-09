// Copyright 2005, Ondra Kamenik

#include "quadrature.hh"
#include "precalc_quadrature.dat"

#include <cmath>

void
OneDPrecalcQuadrature::calcOffsets()
{
  offsets[0] = 0;
  for (int i = 1; i < num_levels; i++)
    offsets[i] = offsets[i-1] + num_points[i-1];
}

GaussHermite::GaussHermite()
  : OneDPrecalcQuadrature(gh_num_levels, gh_num_points, gh_weights, gh_points)
{
}

GaussLegendre::GaussLegendre()
  : OneDPrecalcQuadrature(gl_num_levels, gl_num_points, gl_weights, gl_points)
{
}

/* Here we transform a draw from univariate $\langle 0,1\rangle$ to the
   draw from Gaussina $N(0,1)$. This is done by a table lookup, the table
   is given by |normal_icdf_step|, |normal_icfd_data|, |normal_icdf_num|,
   and a number |normal_icdf_end|. In order to avoid wrong tails for lookups close
   to zero or one, we rescale input |x| by $(1-2*(1-end))=2*end-1$. */

double
NormalICDF::get(double x)
{
  double xx = (2*normal_icdf_end-1)*std::abs(x-0.5);
  auto i = (int) floor(xx/normal_icdf_step);
  double xx1 = normal_icdf_step*i;
  double yy1 = normal_icdf_data[i];
  double y;
  if (i < normal_icdf_num-1)
    {
      double yy2 = normal_icdf_data[i+1];
      y = yy1 + (yy2-yy1)*(xx-xx1)/normal_icdf_step;
    }
  else // this should never happen
    {
      y = yy1;
    }
  if (x > 0.5)
    return y;
  else
    return -y;
}
