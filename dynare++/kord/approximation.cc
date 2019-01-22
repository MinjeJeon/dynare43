// Copyright 2005, Ondra Kamenik

#include "kord_exception.hh"
#include "approximation.hh"
#include "first_order.hh"
#include "korder_stoch.hh"

ZAuxContainer::ZAuxContainer(const _Ctype *gss, int ngss, int ng, int ny, int nu)
  : StackContainer<FGSTensor>(4, 1)
{
  stack_sizes[0] = ngss; stack_sizes[1] = ng;
  stack_sizes[2] = ny; stack_sizes[3] = nu;
  conts[0] = gss;
  calculateOffsets();
}

/* The |getType| method corresponds to
   $f(g^{**}(y^*,u',\sigma),0,0,0)$. For the first argument we return
   |matrix|, for other three we return |zero|. */

ZAuxContainer::itype
ZAuxContainer::getType(int i, const Symmetry &s) const
{
  if (i == 0)
    if (s[2] > 0)
      return zero;
    else
      return matrix;
  return zero;
}

Approximation::Approximation(DynamicModel &m, Journal &j, int ns, bool dr_centr, double qz_crit)
  : model(m), journal(j), rule_ders(nullptr), rule_ders_ss(nullptr), fdr(nullptr), udr(nullptr),
    ypart(model.nstat(), model.npred(), model.nboth(), model.nforw()),
    mom(UNormalMoments(model.order(), model.getVcov())), nvs(4), steps(ns),
    dr_centralize(dr_centr), qz_criterium(qz_crit), ss(ypart.ny(), steps+1)
{
  nvs[0] = ypart.nys(); nvs[1] = model.nexog();
  nvs[2] = model.nexog(); nvs[3] = 1;

  ss.nans();
}

Approximation::~Approximation()
{
  if (rule_ders_ss)
    delete rule_ders_ss;
  if (rule_ders)
    delete rule_ders;
  if (fdr)
    delete fdr;
  if (udr)
    delete udr;
}

/* This just returns |fdr| with a check that it is created. */
const FoldDecisionRule &
Approximation::getFoldDecisionRule() const
{
  KORD_RAISE_IF(fdr == nullptr,
                "Folded decision rule has not been created in Approximation::getFoldDecisionRule");
  return *fdr;
}

/* This just returns |udr| with a check that it is created. */
const UnfoldDecisionRule &
Approximation::getUnfoldDecisionRule() const
{
  KORD_RAISE_IF(udr == nullptr,
                "Unfolded decision rule has not been created in Approximation::getUnfoldDecisionRule");
  return *udr;
}

/* This methods assumes that the deterministic steady state is
   |model.getSteady()|. It makes an approximation about it and stores the
   derivatives to |rule_ders| and |rule_ders_ss|. Also it runs a |check|
   for $\sigma=0$. */
void
Approximation::approxAtSteady()
{
  model.calcDerivativesAtSteady();
  FirstOrder fo(model.nstat(), model.npred(), model.nboth(), model.nforw(),
                model.nexog(), *(model.getModelDerivatives().get(Symmetry(1))),
                journal, qz_criterium);
  KORD_RAISE_IF_X(!fo.isStable(),
                  "The model is not Blanchard-Kahn stable",
                  KORD_MD_NOT_STABLE);

  if (model.order() >= 2)
    {
      KOrder korder(model.nstat(), model.npred(), model.nboth(), model.nforw(),
                    model.getModelDerivatives(), fo.getGy(), fo.getGu(),
                    model.getVcov(), journal);
      korder.switchToFolded();
      for (int k = 2; k <= model.order(); k++)
        korder.performStep<KOrder::fold>(k);

      saveRuleDerivs(korder.getFoldDers());
    }
  else
    {
      FirstOrderDerivs<KOrder::fold> fo_ders(fo);
      saveRuleDerivs(fo_ders);
    }
  check(0.0);
}

/* This is the core routine of |Approximation| class.

   First we solve for the approximation about the deterministic steady
   state. Then we perform |steps| cycles toward the stochastic steady
   state. Each cycle moves the size of shocks by |dsigma=1.0/steps|. At
   the end of a cycle, we have |rule_ders| being the derivatives at
   stochastic steady state for $\sigma=sigma\_so\_far+dsigma$ and
   |model.getSteady()| being the steady state.

   If the number of |steps| is zero, the decision rule |dr| at the bottom
   is created from derivatives about deterministic steady state, with
   size of $\sigma=1$. Otherwise, the |dr| is created from the
   approximation about stochastic steady state with $\sigma=0$.

   Within each cycle, we first make a backup of the last steady (from
   initialization or from a previous cycle), then we calculate the fix
   point of the last rule with $\sigma=dsigma$. This becomes a new steady
   state at the $\sigma=sigma\_so\_far+dsigma$. We calculate expectations
   of $g^{**}(y,\sigma\eta_{t+1},\sigma$ expressed as a Taylor expansion
   around the new $\sigma$ and the new steady state. Then we solve for
   the decision rule with explicit $g^{**}$ at $t+1$ and save the rule.

   After we reached $\sigma=1$, the decision rule is formed.

   The biproduct of this method is the matrix |ss|, whose columns are
   steady states for subsequent $\sigma$s. The first column is the
   deterministic steady state, the last column is the stochastic steady
   state for a full size of shocks ($\sigma=1$). There are |steps+1|
   columns. */

void
Approximation::walkStochSteady()
{
  // initial approximation at deterministic steady
  /* Here we solve for the deterministic steady state, calculate
     approximation at the deterministic steady and save the steady state
     to |ss|. */
  model.solveDeterministicSteady();
  approxAtSteady();
  Vector steady0{ss.getCol(0)};
  steady0 = (const Vector &) model.getSteady();

  double sigma_so_far = 0.0;
  double dsigma = (steps == 0) ? 0.0 : 1.0/steps;
  for (int i = 1; i <= steps; i++)
    {
      JournalRecordPair pa(journal);
      pa << "Approximation about stochastic steady for sigma=" << sigma_so_far+dsigma << endrec;

      Vector last_steady((const Vector &)model.getSteady());

      // calculate fix-point of the last rule for |dsigma|
      /* We form the |DRFixPoint| object from the last rule with
         $\sigma=dsigma$. Then we save the steady state to |ss|. The new steady
         is also put to |model.getSteady()|. */
      DRFixPoint<KOrder::fold> fp(*rule_ders, ypart, model.getSteady(), dsigma);
      bool converged = fp.calcFixPoint(DecisionRule::horner, model.getSteady());
      JournalRecord rec(journal);
      rec << "Fix point calcs: iter=" << fp.getNumIter() << ", newton_iter="
          << fp.getNewtonTotalIter() << ", last_newton_iter=" << fp.getNewtonLastIter() << ".";
      if (converged)
        rec << " Converged." << endrec;
      else
        {
          rec << " Not converged!!" << endrec;
          KORD_RAISE_X("Fix point calculation not converged", KORD_FP_NOT_CONV);
        }
      Vector steadyi{ss.getCol(i)};
      steadyi = (const Vector &) model.getSteady();

      // calculate |hh| as expectations of the last $g^{**}$
      /* We form the steady state shift |dy|, which is the new steady state
         minus the old steady state. Then we create |StochForwardDerivs|
         object, which calculates the derivatives of $g^{**}$ expectations at
         new sigma and new steady. */
      Vector dy((const Vector &)model.getSteady());
      dy.add(-1.0, last_steady);

      StochForwardDerivs<KOrder::fold> hh(ypart, model.nexog(), *rule_ders_ss, mom, dy,
                                          dsigma, sigma_so_far);
      JournalRecord rec1(journal);
      rec1 << "Calculation of g** expectations done" << endrec;

      // form |KOrderStoch|, solve and save
      /* We calculate derivatives of the model at the new steady, form
         |KOrderStoch| object and solve, and save the rule. */
      model.calcDerivativesAtSteady();
      KOrderStoch korder_stoch(ypart, model.nexog(), model.getModelDerivatives(),
                               hh, journal);
      for (int d = 1; d <= model.order(); d++)
        {
          korder_stoch.performStep<KOrder::fold>(d);
        }
      saveRuleDerivs(korder_stoch.getFoldDers());

      check(sigma_so_far+dsigma);
      sigma_so_far += dsigma;
    }

  // construct the resulting decision rules
  if (fdr)
    {
      delete fdr;
      fdr = nullptr;
    }
  if (udr)
    {
      delete udr;
      udr = nullptr;
    }

  fdr = new FoldDecisionRule(*rule_ders, ypart, model.nexog(),
                             model.getSteady(), 1.0-sigma_so_far);
  if (steps == 0 && dr_centralize)
    {
      // centralize decision rule for zero steps
      DRFixPoint<KOrder::fold> fp(*rule_ders, ypart, model.getSteady(), 1.0);
      bool converged = fp.calcFixPoint(DecisionRule::horner, model.getSteady());
      JournalRecord rec(journal);
      rec << "Fix point calcs: iter=" << fp.getNumIter() << ", newton_iter="
          << fp.getNewtonTotalIter() << ", last_newton_iter=" << fp.getNewtonLastIter() << ".";
      if (converged)
        rec << " Converged." << endrec;
      else
        {
          rec << " Not converged!!" << endrec;
          KORD_RAISE_X("Fix point calculation not converged", KORD_FP_NOT_CONV);
        }

      {
        JournalRecordPair recp(journal);
        recp << "Centralizing about fix-point." << endrec;
        FoldDecisionRule *dr_backup = fdr;
        fdr = new FoldDecisionRule(*dr_backup, model.getSteady());
        delete dr_backup;
      }
    }
}

/* Here we simply make a new hardcopy of the given rule |rule_ders|,
   and make a new container of in-place subtensors of the derivatives
   corresponding to forward looking variables. The given container comes
   from a temporary object and will be destroyed. */

void
Approximation::saveRuleDerivs(const FGSContainer &g)
{
  if (rule_ders)
    {
      delete rule_ders;
      delete rule_ders_ss;
    }
  rule_ders = new FGSContainer(g);
  rule_ders_ss = new FGSContainer(4);
  for (auto & run : (*rule_ders))
    {
      auto *ten = new FGSTensor(ypart.nstat+ypart.npred, ypart.nyss(), *(run.second));
      rule_ders_ss->insert(ten);
    }
}

/* This method calculates a shift of the system equations due to
   integrating shocks at a given $\sigma$ and current steady state. More precisely, if
   $$F(y,u,u',\sigma)=f(g^{**}(g^*(y,u,\sigma),u',\sigma),g(y,u,\sigma),y,u)$$
   then the method returns a vector
   $$\sum_{d=1}{1\over d!}\sigma^d\left[F_{u'^d}\right]_{\alpha_1\ldots\alpha_d}
   \Sigma^{\alpha_1\ldots\alpha_d}$$

   For a calculation of $\left[F_{u'^d}\right]$ we use |@<|ZAuxContainer|
   class declaration@>|, so we create its object. In each cycle we
   calculate $\left[F_{u'^d}\right]$@q'@>, and then multiply with the shocks,
   and add the ${\sigma^d\over d!}$ multiple to the result. */

void
Approximation::calcStochShift(Vector &out, double at_sigma) const
{
  KORD_RAISE_IF(out.length() != ypart.ny(),
                "Wrong length of output vector for Approximation::calcStochShift");
  out.zeros();

  ZAuxContainer zaux(rule_ders_ss, ypart.nyss(), ypart.ny(),
                     ypart.nys(), model.nexog());

  int dfac = 1;
  for (int d = 1; d <= rule_ders->getMaxDim(); d++, dfac *= d)
    {
      if (KOrder::is_even(d))
        {
          Symmetry sym(0, d, 0, 0);

          // calculate $F_{u'^d}$ via |ZAuxContainer|
          FGSTensor *ten = new FGSTensor(ypart.ny(), TensorDimens(sym, nvs));
          ten->zeros();
          for (int l = 1; l <= d; l++)
            {
              const FSSparseTensor *f = model.getModelDerivatives().get(Symmetry(l));
              zaux.multAndAdd(*f, *ten);
            }

          // multiply with shocks and add to result
          FGSTensor *tmp = new FGSTensor(ypart.ny(), TensorDimens(Symmetry(0, 0, 0, 0), nvs));
          tmp->zeros();
          ten->contractAndAdd(1, *tmp, *(mom.get(Symmetry(d))));

          out.add(pow(at_sigma, d)/dfac, tmp->getData());
          delete ten;
          delete tmp;
        }
    }
}

/* This method calculates and reports
   $$f(\bar y)+\sum_{d=1}{1\over d!}\sigma^d\left[F_{u'^d}\right]_{\alpha_1\ldots\alpha_d}
   \Sigma^{\alpha_1\ldots\alpha_d}$$
   at $\bar y$, zero shocks and $\sigma$. This number should be zero.

   We evaluate the error both at a given $\sigma$ and $\sigma=1.0$. */

void
Approximation::check(double at_sigma) const
{
  Vector stoch_shift(ypart.ny());
  Vector system_resid(ypart.ny());
  Vector xx(model.nexog());
  xx.zeros();
  model.evaluateSystem(system_resid, model.getSteady(), xx);
  calcStochShift(stoch_shift, at_sigma);
  stoch_shift.add(1.0, system_resid);
  JournalRecord rec1(journal);
  rec1 << "Error of current approximation for shocks at sigma " << at_sigma
       << " is " << stoch_shift.getMax() << endrec;
  calcStochShift(stoch_shift, 1.0);
  stoch_shift.add(1.0, system_resid);
  JournalRecord rec2(journal);
  rec2 << "Error of current approximation for full shocks is " << stoch_shift.getMax() << endrec;
}

/* The method returns unconditional variance of endogenous variables
   based on the first order. The first order approximation looks like
   $$\hat y_t=g_{y^*}\hat y^*_{t-1}+g_uu_t$$
   where $\hat y$ denotes a deviation from the steady state. It can be written as
   $$\hat y_t=\left[0\, g_{y^*}\, 0\right]\hat y_{t-1}+g_uu_t$$
   which yields unconditional covariance $V$ for which
   $$V=GVG^T + g_u\Sigma g_u^T,$$
   where $G=[0\, g_{y^*}\, 0]$ and $\Sigma$ is the covariance of the shocks.

   For solving this Lyapunov equation we use the Sylvester module, which
   solves equation of the type
   $$AX+BX(C\otimes\cdots\otimes C)=D$$
   So we invoke the Sylvester solver for the first dimension with $A=I$,
   $B=-G$, $C=G^T$ and $D=g_u\Sigma g_u^T$. */

TwoDMatrix *
Approximation::calcYCov() const
{
  const TwoDMatrix &gy = *(rule_ders->get(Symmetry(1, 0, 0, 0)));
  const TwoDMatrix &gu = *(rule_ders->get(Symmetry(0, 1, 0, 0)));
  TwoDMatrix G(model.numeq(), model.numeq());
  G.zeros();
  G.place(gy, 0, model.nstat());
  TwoDMatrix B((const TwoDMatrix &)G);
  B.mult(-1.0);
  TwoDMatrix C(G, "transpose");
  TwoDMatrix A(model.numeq(), model.numeq());
  A.zeros();
  for (int i = 0; i < model.numeq(); i++)
    A.get(i, i) = 1.0;

  TwoDMatrix guSigma(gu, model.getVcov());
  TwoDMatrix guTrans(gu, "transpose");
  TwoDMatrix *X = new TwoDMatrix(guSigma, guTrans);

  GeneralSylvester gs(1, model.numeq(), model.numeq(), 0,
                      A.getData(), B.getData(), C.getData(), X->getData());
  gs.solve();

  return X;
}
