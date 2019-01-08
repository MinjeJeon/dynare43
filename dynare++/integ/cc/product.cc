// Copyright 2005, Ondra Kamenik

#include "product.hh"
#include "symmetry.hh"

prodpit::prodpit()
  : prodq(NULL), level(0), npoints(0), jseq(NULL),
    end_flag(true), sig(NULL), p(NULL)
{
}

/* This constructs a product iterator corresponding to index $(j0,0\ldots,0)$. */

prodpit::prodpit(const ProductQuadrature &q, int j0, int l)
  : prodq(&q), level(l), npoints(q.uquad.numPoints(l)), jseq(new IntSequence(q.dimen(), 0)),
    end_flag(false), sig(new ParameterSignal(q.dimen())), p(new Vector(q.dimen()))
{
  if (j0 < npoints)
    {
      (*jseq)[0] = j0;
      setPointAndWeight();
    }
  else
    {
      end_flag = true;
    }
}

prodpit::prodpit(const prodpit &ppit)
  : prodq(ppit.prodq), level(ppit.level), npoints(ppit.npoints),
    end_flag(ppit.end_flag), w(ppit.w)
{
  if (ppit.jseq)
    jseq = new IntSequence(*(ppit.jseq));
  else
    jseq = NULL;
  if (ppit.sig)
    sig = new ParameterSignal(*(ppit.sig));
  else
    sig = NULL;
  if (ppit.p)
    p = new Vector(*(ppit.p));
  else
    p = NULL;
}

prodpit::~prodpit()
{
  if (jseq)
    delete jseq;
  if (sig)
    delete sig;
  if (p)
    delete p;
}

bool
prodpit::operator==(const prodpit &ppit) const
{
  bool ret = true;
  ret = ret & prodq == ppit.prodq;
  ret = ret & end_flag == ppit.end_flag;
  ret = ret & ((jseq == NULL && ppit.jseq == NULL)
               || (jseq != NULL && ppit.jseq != NULL && *jseq == *(ppit.jseq)));
  return ret;
}

const prodpit &
prodpit::operator=(const prodpit &ppit)
{
  prodq = ppit.prodq;
  end_flag = ppit.end_flag;
  w = ppit.w;

  if (jseq)
    delete jseq;
  if (sig)
    delete sig;
  if (p)
    delete p;

  if (ppit.jseq)
    jseq = new IntSequence(*(ppit.jseq));
  else
    jseq = NULL;
  if (ppit.sig)
    sig = new ParameterSignal(*(ppit.sig));
  else
    sig = NULL;
  if (ppit.p)
    p = new Vector(*(ppit.p));
  else
    p = NULL;

  return *this;
}

prodpit &
prodpit::operator++()
{
  // todo: throw if |prodq==NULL| or |jseq==NULL| or |sig==NULL| or |end_flag==true|
  int i = prodq->dimen()-1;
  (*jseq)[i]++;
  while (i >= 0 && (*jseq)[i] == npoints)
    {
      (*jseq)[i] = 0;
      i--;
      if (i >= 0)
        (*jseq)[i]++;
    }
  sig->signalAfter(std::max(i, 0));

  if (i == -1)
    end_flag = true;

  if (!end_flag)
    setPointAndWeight();

  return *this;
}

/* This calculates the weight and sets point coordinates from the indices. */

void
prodpit::setPointAndWeight()
{
  // todo: raise if |prodq==NULL| or |jseq==NULL| or |sig==NULL| or
  // |p==NULL| or |end_flag==true|
  w = 1.0;
  for (int i = 0; i < prodq->dimen(); i++)
    {
      (*p)[i] = (prodq->uquad).point(level, (*jseq)[i]);
      w *= (prodq->uquad).weight(level, (*jseq)[i]);
    }
}

/* Debug print. */

void
prodpit::print() const
{
  printf("j=[");
  for (int i = 0; i < prodq->dimen(); i++)
    printf("%2d ", (*jseq)[i]);
  printf("] %+4.3f*(", w);
  for (int i = 0; i < prodq->dimen()-1; i++)
    printf("%+4.3f ", (*p)[i]);
  printf("%+4.3f)\n", (*p)[prodq->dimen()-1]);
}

ProductQuadrature::ProductQuadrature(int d, const OneDQuadrature &uq)
  : QuadratureImpl<prodpit>(d), uquad(uq)
{
  // todo: check |d>=1|
}

/* This calls |prodpit| constructor to return an iterator which points
   approximatelly at |ti|-th portion out of |tn| portions. First we find
   out how many points are in the level, and then construct an interator
   $(j0,0,\ldots,0)$ where $j0=$|ti*npoints/tn|. */

prodpit
ProductQuadrature::begin(int ti, int tn, int l) const
{
  // todo: raise is |l<dimen()|
  // todo: check |l<=uquad.numLevels()|
  int npoints = uquad.numPoints(l);
  return prodpit(*this, ti*npoints/tn, l);
}

/* This just starts at the first level and goes to a higher level as
   long as a number of evaluations (which is $n_k^d$ for $k$ being the
   level) is less than the given number of evaluations. */

void
ProductQuadrature::designLevelForEvals(int max_evals, int &lev, int &evals) const
{
  int last_evals;
  evals = 1;
  lev = 1;
  do
    {
      lev++;
      last_evals = evals;
      evals = numEvals(lev);
    }
  while (lev < uquad.numLevels()-2 && evals < max_evals);
  lev--;
  evals = last_evals;

}
