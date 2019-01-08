// Copyright 2005, Ondra Kamenik

#include "fine_container.hh"

#include <cmath>

/* Here we construct the vector of new sizes of containers (before
   |nc|) and copy all remaining sizes behind |nc|. */

SizeRefinement::SizeRefinement(const IntSequence &s, int nc, int max)
{
  new_nc = 0;
  for (int i = 0; i < nc; i++)
    {
      int nr = s[i]/max;
      if (s[i] % max != 0)
        nr++;
      int ss = (nr > 0) ? (int) round(((double) s[i])/nr) : 0;
      for (int j = 0; j < nr - 1; j++)
        {
          rsizes.push_back(ss);
          ind_map.push_back(i);
          new_nc++;
        }
      rsizes.push_back(s[i]-(nr-1)*ss);
      ind_map.push_back(i);
      new_nc++;
    }

  for (int i = nc; i < s.size(); i++)
    {
      rsizes.push_back(s[i]);
      ind_map.push_back(i);
    }
}
