// Copyright 2005, Ondra Kamenik

// Approximating model solution

/* The class |Approximation| in this file is a main interface to the
   algorithms calculating approximations to the decision rule about
   deterministic and stochastic steady states.

   The approximation about a deterministic steady state is solved by
   classes |@<|FirstOrder| class declaration@>| and |@<|KOrder| class
   declaration@>|. The approximation about the stochastic steady state is
   solved by class |@<|KOrderStoch| class declaration@>| together with a
   method of |Approximation| class |@<|Approximation::walkStochSteady|
   code@>|.

   The approximation about the stochastic steady state is done with
   explicit expression of forward derivatives of $g^{**}$. More formally,
   we have to solve the decision rule $g$ from the implicit system:
   $$E_t(f(g^{**}(g^*(y^*,u_t,\sigma),u_{t+1},\sigma),g(y^*,u_t,\sigma),y_t,u_t))=0$$
   The term within the expectations can be Taylor expanded, and the
   expectation can be driven into the formula. However, when doing this
   at $\sigma\not=0$, the term $g^{**}$ at $\sigma\not=0$ is dependent on
   $u_{t+1}$ and thus the integral of its approximation includes all
   derivatives wrt. $u$ of $g^{**}$. Note that for $\sigma=0$, the
   derivatives of $g^{**}$ in this context are constant. This is the main
   difference between the approximation at deterministic steady
   ($\sigma=0$), and stochastic steady ($\sigma\not=0$). This means that
   $k$-order derivative of the above equation at $\sigma\not=0$ depends of
   all derivatives of $g^**$ (including those with order greater than
   $k$).

   The explicit expression of the forward $g^{**}$ means that the
   derivatives of $g$ are not solved simultaneously, but that the forward
   derivatives of $g^{**}$ are calculated as an extrapolation based on
   the approximation at lower $\sigma$. This is exactly what does the
   |@<|Approximation::walkStochSteady| code@>|. It starts at the
   deterministic steady state, and in a few steps it adds to $\sigma$
   explicitly expressing forward $g^{**}$ from a previous step.

   Further details on the both solution methods are given in (todo: put
   references here when they exist).

   Very important note: all classes here used for calculation of decision
   rule approximation are folded. For the time being, it seems that faa
   Di Bruno formula is quicker for folded tensors, and that is why we
   stick to folded tensors here. However, when the calcs are done, we
   calculate also its unfolded versions, to be available for simulations
   and so on. */

#ifndef APPROXIMATION_H
#define APPROXIMATION_H

#include "dynamic_model.hh"
#include "decision_rule.hh"
#include "korder.hh"
#include "journal.hh"

/* This class is used to calculate derivatives by faa Di Bruno of the
   $$f(g^{**}(g^*(y^*,u,\sigma),u',\sigma),g(y^*,u,\sigma),y^*,u)$$ with
   respect $u'$. In order to keep it as simple as possible, the class
   represents an equivalent (with respect to $u'$) container for
   $f(g^{**}(y^*,u',\sigma),0,0,0)$. The class is used only for
   evaluation of approximation error in |Approximation| class, which is
   calculated in |Approximation::calcStochShift| method.

   Since it is a folded version, we inherit from
   |StackContainer<FGSTensor>| and |FoldedStackContainer|. To construct
   it, we need only the $g^{**}$ container and size of stacks. */

class ZAuxContainer : public StackContainer<FGSTensor>, public FoldedStackContainer
{
public:
  using _Ctype = StackContainer<FGSTensor>::_Ctype;
  using itype = StackContainer<FGSTensor>::itype;
  ZAuxContainer(const _Ctype *gss, int ngss, int ng, int ny, int nu);
  itype getType(int i, const Symmetry &s) const override;
};

/* This class provides an interface to approximation algorithms. The
   core method is |walkStochSteady| which calculates the approximation
   about stochastic steady state in a given number of steps. The number
   is given as a parameter |ns| of the constructor. If the number is
   equal to zero, the resulted approximation is about the deterministic
   steady state.

   An object is constructed from the |DynamicModel|, and the number of
   steps |ns|. Also, we pass a reference to journal. That's all. The
   result of the core method |walkStochSteady| is a decision rule |dr|
   and a matrix |ss| whose columns are steady states for increasing
   $\sigma$ during the walk. Both can be retrived by public methods. The
   first column of the matrix is the deterministic steady state, the last
   is the stochastic steady state for the full size shocks.

   The method |walkStochSteady| calls the following methods:
   |approxAtSteady| calculates an initial approximation about the
   deterministic steady, |saveRuleDerivs| saves derivatives of a rule for
   the following step in |rule_ders| and |rule_ders_ss| (see
   |@<|Approximation::saveRuleDerivs| code@>| for their description),
   |check| reports an error of the current approximation and
   |calcStochShift| (called from |check|) calculates a shift of the
   system equations due to uncertainity.

   dr\_centralize is a new option. dynare++ was automatically expressing
   results around the fixed point instead of the deterministic steady
   state. dr\_centralize controls this behavior. */

class Approximation
{
  DynamicModel &model;
  Journal &journal;
  FGSContainer *rule_ders;
  FGSContainer *rule_ders_ss;
  FoldDecisionRule *fdr;
  UnfoldDecisionRule *udr;
  const PartitionY ypart;
  const FNormalMoments mom;
  IntSequence nvs;
  int steps;
  bool dr_centralize;
  double qz_criterium;
  TwoDMatrix ss;
public:
  Approximation(DynamicModel &m, Journal &j, int ns, bool dr_centr, double qz_crit);
  virtual
  ~Approximation();

  const FoldDecisionRule&getFoldDecisionRule() const;
  const UnfoldDecisionRule&getUnfoldDecisionRule() const;
  const TwoDMatrix &
  getSS() const
  {
    return ss;
  }
  const DynamicModel &
  getModel() const
  {
    return model;
  }

  void walkStochSteady();
  TwoDMatrix *calcYCov() const;
  const FGSContainer *
  get_rule_ders() const
  {
    return rule_ders;
  }
  const FGSContainer *
  get_rule_ders_ss() const
  {
    return rule_ders;
  }
protected:
  void approxAtSteady();
  void calcStochShift(Vector &out, double at_sigma) const;
  void saveRuleDerivs(const FGSContainer &g);
  void check(double at_sigma) const;
};

#endif
