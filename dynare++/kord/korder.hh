// Copyright 2004, Ondra Kamenik

// Higher order at deterministic steady

/*
  The main purpose of this file is to implement a perturbation method
  algorithm for an SDGE model for higher order approximations. The input
  of the algorithm are sparse tensors as derivatives of the dynamic
  system, then dimensions of vector variables, then the first order
  approximation to the decision rule and finally a covariance matrix of
  exogenous shocks. The output are higher order derivatives of decision
  rule $y_t=g(y^*_{t-1},u_t,\sigma)$. The class provides also a method
  for checking a size of residuals of the solved equations.

  The algorithm is implemented in |KOrder| class. The class contains
  both unfolded and folded containers to allow for switching (usually
  from unfold to fold) during the calculations. The algorithm is
  implemented in a few templated methods. To do this, we need some
  container type traits, which are in |ctraits| struct. Also, the
  |KOrder| class contains some information encapsulated in other
  classes, which are defined here. These include: |PartitionY|,
  |MatrixA|, |MatrixS| and |MatrixB|.
*/

#ifndef KORDER_H
#define KORDER_H

#include "int_sequence.hh"
#include "fs_tensor.hh"
#include "gs_tensor.hh"
#include "t_container.hh"
#include "stack_container.hh"
#include "normal_moments.hh"
#include "t_polynomial.hh"
#include "faa_di_bruno.hh"
#include "journal.hh"
#include "pascal_triangle.hh"

#include "kord_exception.hh"
#include "GeneralSylvester.hh"

#include <dynlapack.h>

#include <cmath>
#include <type_traits>

// The enum class passed as template parameter for many data structures
enum class Storage { fold, unfold };

/* In |ctraits| we define a number of types. We have a type for tensor
   |Ttensor|, and types for each pair of folded/unfolded containers used as a
   member in |KOrder|. */

class FoldedZXContainer;
class UnfoldedZXContainer;
class FoldedGXContainer;
class UnfoldedGXContainer;

template <Storage type>
class ctraits
{
public:
  using Ttensor = std::conditional_t<type == Storage::fold, FGSTensor, UGSTensor>;
  using Ttensym = std::conditional_t<type == Storage::fold, FFSTensor, UFSTensor>;
  using Tg = std::conditional_t<type == Storage::fold, FGSContainer, UGSContainer>;
  using Tgs = std::conditional_t<type == Storage::fold, FGSContainer, UGSContainer>;
  using Tgss = std::conditional_t<type == Storage::fold, FGSContainer, UGSContainer>;
  using TG = std::conditional_t<type == Storage::fold, FGSContainer, UGSContainer>;
  using TZstack = std::conditional_t<type == Storage::fold, FoldedZContainer, UnfoldedZContainer>;
  using TGstack = std::conditional_t<type == Storage::fold, FoldedGContainer, UnfoldedGContainer>;
  using Tm = std::conditional_t<type == Storage::fold, FNormalMoments, UNormalMoments>;
  using Tpol = std::conditional_t<type == Storage::fold, FTensorPolynomial, UTensorPolynomial>;
  using TZXstack = std::conditional_t<type == Storage::fold, FoldedZXContainer, UnfoldedZXContainer>;
  using TGXstack = std::conditional_t<type == Storage::fold, FoldedGXContainer, UnfoldedGXContainer>;
};

/* The |PartitionY| class defines the partitioning of state variables
   $y$. The vector $y$, and subvector $y^*$, and $y^{**}$ are defined.
   $$y=\left[\matrix{\hbox{static}\cr\hbox{predeter}\cr\hbox{both}\cr
   \hbox{forward}}\right],\quad
   y^*=\left[\matrix{\hbox{predeter}\cr\hbox{both}}\right],\quad
   y^{**}=\left[\matrix{\hbox{both}\cr\hbox{forward}}\right],$$
   where ``static'' means variables appearing only at time $t$,
   ``predeter'' means variables appearing at time $t-1$, but not at
   $t+1$, ``both'' means variables appearing both at $t-1$ and $t+1$
   (regardless appearance at $t$), and ``forward'' means variables
   appearing at $t+1$, but not at $t-1$.

   The class maintains the four lengths, and returns the whole length,
   length of $y^s$, and length of $y^{**}$.
*/

struct PartitionY
{
  const int nstat;
  const int npred;
  const int nboth;
  const int nforw;
  PartitionY(int num_stat, int num_pred,
             int num_both, int num_forw)
    : nstat(num_stat), npred(num_pred),
      nboth(num_both), nforw(num_forw)
  {
  }
  int
  ny() const
  {
    return nstat+npred+nboth+nforw;
  }
  int
  nys() const
  {
    return npred+nboth;
  }
  int
  nyss() const
  {
    return nboth+nforw;
  }
};

/* This is an abstraction for a square matrix with attached PLU
   factorization. It can calculate the PLU factorization and apply the
   inverse with some given matrix.

   We use LAPACK $PLU$ decomposition for the inverse. We store the $L$
   and $U$ in the |inv| array and |ipiv| is the permutation $P$. */

class PLUMatrix : public TwoDMatrix
{
public:
  PLUMatrix(int n)
    : TwoDMatrix(n, n),
      inv(nrows()*ncols()),
      ipiv(nrows())
  {
  }
  void multInv(TwoDMatrix &m) const;
private:
  Vector inv;
  std::vector<lapack_int> ipiv;
protected:
  void calcPLU();
};

/* The class |MatrixA| is used for matrix $\left[f_{y}\right]+ \left[0
   \left[f_{y^{**}_+}\right]\cdot\left[g^{**}_{y^*}\right] 0\right]$,
   which is central for the perturbation method step. */

class MatrixA : public PLUMatrix
{
public:
  MatrixA(const FSSparseTensor &f, const IntSequence &ss,
          const TwoDMatrix &gy, const PartitionY &ypart);
};

/* The class |MatrixS| slightly differs from |MatrixA|. It is used for
   matrix $$\left[f_{y}\right]+ \left[0
   \quad\left[f_{y^{**}_+}\right]\cdot\left[g^{**}_{y^*}\right]\quad
   0\right]+\left[0\quad 0\quad\left[f_{y^{**}_+}\right]\right]$$, which is
   needed when recovering $g_{\sigma^k}$. */

class MatrixS : public PLUMatrix
{
public:
  MatrixS(const FSSparseTensor &f, const IntSequence &ss,
          const TwoDMatrix &gy, const PartitionY &ypart);
};

/* The $B$ matrix is equal to $\left[f_{y^{**}_+}\right]$. We have just
   a constructor. */

class MatrixB : public TwoDMatrix
{
public:
  MatrixB(const FSSparseTensor &f, const IntSequence &ss)
    : TwoDMatrix(FGSTensor(f, ss, IntSequence(1, 0),
                           TensorDimens(ss, IntSequence(1, 0))))
  {
  }
};

/* Here we have the class for the higher order approximations. It
   contains the following data:

   \halign{\kern\parindent\vrule height12pt width0pt
   \vtop{\hsize=4cm\noindent\raggedright #}&\kern0.5cm\vtop{\hsize=10cm\noindent #}\cr
   variable sizes ypart& |PartitionY| struct maintaining partitions of
   $y$, see |@<|PartitionY| struct declaration@>|\cr
   tensor variable dimension |nvs|& variable sizes of all tensors in
   containers, sizes of $y^*$, $u$, $u'$@q'@> and $\sigma$\cr
   tensor containers & folded and unfolded containers for $g$, $g_{y^*}$,
   $g_{y^**}$ (the latter two collect appropriate subtensors of $g$, they
   do not allocate any new space), $G$, $G$ stack, $Z$ stack\cr
   dynamic model derivatives & just a reference to the container of
   sparse tensors of the system derivatives, lives outside the class\cr
   moments & both folded and unfolded normal moment containers, both are
   calculated at initialization\cr
   matrices & matrix $A$, matrix $S$, and matrix $B$, see |@<|MatrixA| class
   declaration@>| and |@<|MatrixB| class declaration@>|\cr
   }

   \kern 0.4cm

   The methods are the following:
   \halign{\kern\parindent\vrule height12pt width0pt
   \vtop{\hsize=4cm\noindent\raggedright #}&\kern0.5cm\vtop{\hsize=10cm\noindent #}\cr
   member access & we declare template methods for accessing containers
   depending on |fold| and |unfold| flag, we implement their
   specializations\cr
   |performStep| & this performs $k$-order step provided that $k=2$ or
   the $k-1$-th step has been run, this is the core method\cr
   |check| & this calculates residuals of all solved equations for
   $k$-order and reports their sizes, it is runnable after $k$-order
   |performStep| has been run\cr
   |insertDerivative| & inserts a $g$ derivative to the $g$ container and
   also creates subtensors and insert them to $g_{y^*}$ and $g_{y^{**}}$
   containers\cr
   |sylvesterSolve| & solve the sylvester equation (templated fold, and
   unfold)\cr
   |faaDiBrunoZ| & calculates derivatives of $F$ by Faa Di Bruno for the
   sparse container of system derivatives and $Z$ stack container\cr
   |faaDiBrunoG| & calculates derivatives of $G$ by Faa Di Bruno for the
   dense container $g^{**}$ and $G$ stack\cr
   |recover_y| & recovers $g_{y^{*i}}$\cr
   |recover_yu| & recovers $g_{y^{*i}u^j}$\cr
   |recover_ys| & recovers $g_{y^{*i}\sigma^j}$\cr
   |recover_yus| & recovers $g_{y^{*i}u^j\sigma^k}$\cr
   |recover_s| & recovers $g_{\sigma^i}$\cr
   |fillG| & calculates specified derivatives of $G$ and inserts them to
   the container\cr
   |calcE_ijk|& calculates $E_{ijk}$\cr
   |calcD_ijk|& calculates $D_{ijk}$\cr
   }

   \kern 0.3cm

   Most of the code is templated, and template types are calculated in
   |ctraits|. So all templated methods get a template argument |T|, which
   can be either |Storage::fold|, or |Storage::unfold|.
*/

class KOrder
{
protected:
  const PartitionY ypart;
  const int ny;
  const int nu;
  const int maxk;
  IntSequence nvs;

  /* These are containers. The names are not important because they do
     not appear anywhere else since we access them by template functions. */
  UGSContainer _ug;
  FGSContainer _fg;
  UGSContainer _ugs;
  FGSContainer _fgs;
  UGSContainer _ugss;
  FGSContainer _fgss;
  UGSContainer _uG;
  FGSContainer _fG;
  UnfoldedZContainer _uZstack;
  FoldedZContainer _fZstack;
  UnfoldedGContainer _uGstack;
  FoldedGContainer _fGstack;
  UNormalMoments _um;
  FNormalMoments _fm;
  const TensorContainer<FSSparseTensor> &f;

  const MatrixA matA;
  const MatrixS matS;
  const MatrixB matB;

  /* These are the declarations of the template functions accessing the
     containers. */
  template<Storage t>
  typename ctraits<t>::Tg &g();
  template<Storage t>
  const typename ctraits<t>::Tg &g() const;

  template<Storage t>
  typename ctraits<t>::Tgs &gs();
  template<Storage t>
  const typename ctraits<t>::Tgs &gs() const;

  template<Storage t>
  typename ctraits<t>::Tgss &gss();
  template<Storage t>
  const typename ctraits<t>::Tgss &gss() const;

  template<Storage t>
  typename ctraits<t>::TG &G();
  template<Storage t>
  const typename ctraits<t>::TG &G() const;

  template<Storage t>
  typename ctraits<t>::TZstack &Zstack();
  template<Storage t>
  const typename ctraits<t>::TZstack &Zstack() const;

  template<Storage t>
  typename ctraits<t>::TGstack &Gstack();
  template<Storage t>
  const typename ctraits<t>::TGstack &Gstack() const;

  template<Storage t>
  typename ctraits<t>::Tm &m();
  template<Storage t>
  const typename ctraits<t>::Tm &m() const;

  Journal &journal;
public:
  KOrder(int num_stat, int num_pred, int num_both, int num_forw,
         const TensorContainer<FSSparseTensor> &fcont,
         const TwoDMatrix &gy, const TwoDMatrix &gu, const TwoDMatrix &v,
         Journal &jr);
  template <Storage t>
  void performStep(int order);
  template <Storage t>
  double check(int dim) const;
  template <Storage t>
  Vector calcStochShift(int order, double sigma) const;
  void switchToFolded();
  const PartitionY &
  getPartY() const
  {
    return ypart;
  }
  const FGSContainer &
  getFoldDers() const
  {
    return _fg;
  }
  const UGSContainer &
  getUnfoldDers() const
  {
    return _ug;
  }
  static bool
  is_even(int i)
  {
    return i % 2 == 0;
  }
protected:
  template <Storage t>
  void insertDerivative(std::unique_ptr<typename ctraits<t>::Ttensor> der);
  template<Storage t>
  void sylvesterSolve(typename ctraits<t>::Ttensor &der) const;

  template <Storage t>
  std::unique_ptr<typename ctraits<t>::Ttensor> faaDiBrunoZ(const Symmetry &sym) const;
  template <Storage t>
  std::unique_ptr<typename ctraits<t>::Ttensor> faaDiBrunoG(const Symmetry &sym) const;

  template<Storage t>
  void recover_y(int i);
  template <Storage t>
  void recover_yu(int i, int j);
  template <Storage t>
  void recover_ys(int i, int j);
  template <Storage t>
  void recover_yus(int i, int j, int k);
  template <Storage t>
  void recover_s(int i);
  template<Storage t>
  void fillG(int i, int j, int k);

  template <Storage t>
  typename ctraits<t>::Ttensor calcD_ijk(int i, int j, int k) const;
  template <Storage t>
  typename ctraits<t>::Ttensor calcD_ik(int i, int k) const;
  template <Storage t>
  typename ctraits<t>::Ttensor calcD_k(int k) const;

  template <Storage t>
  typename ctraits<t>::Ttensor calcE_ijk(int i, int j, int k) const;
  template <Storage t>
  typename ctraits<t>::Ttensor calcE_ik(int i, int k) const;
  template <Storage t>
  typename ctraits<t>::Ttensor calcE_k(int k) const;
};

/* Here we insert the result to the container. Along the insertion, we
   also create subtensors and insert as well. */

template <Storage t>
void
KOrder::insertDerivative(std::unique_ptr<typename ctraits<t>::Ttensor> der)
{
  auto der_ptr = der.get();
  g<t>().insert(std::move(der));
  gs<t>().insert(std::make_unique<typename ctraits<t>::Ttensor>(ypart.nstat, ypart.nys(), *der_ptr));
  gss<t>().insert(std::make_unique<typename ctraits<t>::Ttensor>(ypart.nstat+ypart.npred,
                                                                 ypart.nyss(), *der_ptr));
}

/* Here we implement Faa Di Bruno formula
   $$\sum_{l=1}^k\left[f_{z^l}\right]_{\gamma_1\ldots\gamma_l}
   \sum_{c\in M_{l,k}}\prod_{m=1}^l\left[z_{s(c_m)}\right]^{\gamma_m},
   $$
   where $s$ is a given outer symmetry and $k$ is the dimension of the
   symmetry. */

template <Storage t>
std::unique_ptr<typename ctraits<t>::Ttensor>
KOrder::faaDiBrunoZ(const Symmetry &sym) const
{
  JournalRecordPair pa(journal);
  pa << "Faa Di Bruno Z container for " << sym << endrec;
  auto res = std::make_unique<typename ctraits<t>::Ttensor>(ny, TensorDimens(sym, nvs));
  FaaDiBruno bruno(journal);
  bruno.calculate(Zstack<t>(), f, *res);
  return res;
}

/* The same as |@<|KOrder::faaDiBrunoZ| templated code@>|, but for
   $g^{**}$ and $G$ stack. */

template <Storage t>
std::unique_ptr<typename ctraits<t>::Ttensor>
KOrder::faaDiBrunoG(const Symmetry &sym) const
{
  JournalRecordPair pa(journal);
  pa << "Faa Di Bruno G container for " << sym << endrec;
  TensorDimens tdims(sym, nvs);
  auto res = std::make_unique<typename ctraits<t>::Ttensor>(ypart.nyss(), tdims);
  FaaDiBruno bruno(journal);
  bruno.calculate(Gstack<t>(), gss<t>(), *res);
  return res;
}

/* Here we solve $\left[F_{y^i}\right]=0$. First we calculate
   conditional $G_{y^i}$ (it misses $l=1$ and $l=i$ since $g_{y^i}$ does
   not exist yet). Then calculate conditional $F_{y^i}$ and we have the
   right hand side of equation. Since we miss two orders, we solve by
   Sylvester, and insert the solution as the derivative $g_{y^i}$. Then
   we need to update $G_{y^i}$ running |multAndAdd| for both dimensions
   $1$ and $i$.

   {\bf Requires:} everything at order $\leq i-1$

   {\bf Provides:} $g_{y^i}$, and $G_{y^i}$ */

template<Storage t>
void
KOrder::recover_y(int i)
{
  Symmetry sym{i, 0, 0, 0};
  JournalRecordPair pa(journal);
  pa << "Recovering symmetry " << sym << endrec;

  auto G_yi = faaDiBrunoG<t>(sym);
  auto G_yi_ptr = G_yi.get();
  G<t>().insert(std::move(G_yi));

  auto g_yi = faaDiBrunoZ<t>(sym);
  g_yi->mult(-1.0);

  sylvesterSolve<t>(*g_yi);

  insertDerivative<t>(std::move(g_yi));

  auto &gss_y = gss<t>().get(Symmetry{1, 0, 0, 0});
  gs<t>().multAndAdd(gss_y, *G_yi_ptr);
  auto &gss_yi = gss<t>().get(sym);
  gs<t>().multAndAdd(gss_yi, *G_yi_ptr);
}

/* Here we solve $\left[F_{y^iu^j}\right]=0$ to obtain $g_{y^iu^j}$ for
   $j>0$. We calculate conditional $G_{y^iu^j}$ (this misses only $l=1$)
   and calculate conditional $F_{y^iu^j}$ and we have the right hand
   side. It is solved by multiplication of inversion of $A$. Then we insert
   the result, and update $G_{y^iu^j}$ by |multAndAdd| for $l=1$.

   {\bf Requires:} everything at order $\leq i+j-1$, $G_{y^{i+j}}$, and
   $g_{y^{i+j}}$.

   {\bf Provides:} $g_{y^iu^j}$, and $G_{y^iu^j}$ */

template <Storage t>
void
KOrder::recover_yu(int i, int j)
{
  Symmetry sym{i, j, 0, 0};
  JournalRecordPair pa(journal);
  pa << "Recovering symmetry " << sym << endrec;

  auto G_yiuj = faaDiBrunoG<t>(sym);
  auto G_yiuj_ptr = G_yiuj.get();
  G<t>().insert(std::move(G_yiuj));

  auto g_yiuj = faaDiBrunoZ<t>(sym);
  g_yiuj->mult(-1.0);
  matA.multInv(*g_yiuj);
  insertDerivative<t>(std::move(g_yiuj));

  gs<t>().multAndAdd(gss<t>().get(Symmetry{1, 0, 0, 0}), *G_yiuj_ptr);
}

/* Here we solve
   $\left[F_{y^i\sigma^j}\right]+\left[D_{ij}\right]+\left[E_{ij}\right]=0$
   to obtain $g_{y^i\sigma^j}$. We calculate conditional
   $G_{y^i\sigma^j}$ (missing dimensions $1$ and $i+j$), calculate
   conditional $F_{y^i\sigma^j}$. Before we can calculate $D_{ij}$ and
   $E_{ij}$, we have to calculate $G_{y^iu'^m\sigma^{j-m}}$ for
   $m=1,\ldots,j$. Then we add the $D_{ij}$ and $E_{ij}$ to obtain the
   right hand side. Then we solve the sylvester to obtain
   $g_{y^i\sigma^j}$. Then we update $G_{y^i\sigma^j}$ for $l=1$ and
   $l=i+j$.

   {\bf Requires:} everything at order $\leq i+j-1$, $g_{y^{i+j}}$,
   $G_{y^iu'^j}$ and $g_{y^iu^j}$ through $D_{ij}$,
   $g_{y^iu^m\sigma^{j-m}}$ for
   $m=1,\ldots,j-1$ through $E_{ij}$.

   {\bf Provides:} $g_{y^i\sigma^j}$ and $G_{y^i\sigma^j}$, and finally
   $G_{y^iu'^m\sigma^{j-m}}$ for $m=1,\ldots,j$. The latter is calculated
   by |fillG| before the actual calculation. */

template <Storage t>
void
KOrder::recover_ys(int i, int j)
{
  Symmetry sym{i, 0, 0, j};
  JournalRecordPair pa(journal);
  pa << "Recovering symmetry " << sym << endrec;

  fillG<t>(i, 0, j);

  if (is_even(j))
    {
      auto G_yisj = faaDiBrunoG<t>(sym);
      auto G_yisj_ptr = G_yisj.get();
      G<t>().insert(std::move(G_yisj));

      auto g_yisj = faaDiBrunoZ<t>(sym);

      {
        auto D_ij = calcD_ik<t>(i, j);
        g_yisj->add(1.0, D_ij);
      }

      if (j >= 3)
        {
          auto E_ij = calcE_ik<t>(i, j);
          g_yisj->add(1.0, E_ij);
        }

      g_yisj->mult(-1.0);

      sylvesterSolve<t>(*g_yisj);

      insertDerivative<t>(std::move(g_yisj));

      Gstack<t>().multAndAdd(1, gss<t>(), *G_yisj_ptr);
      Gstack<t>().multAndAdd(i+j, gss<t>(), *G_yisj_ptr);
    }
}

/* Here we solve
   $\left[F_{y^iu^j\sigma^k}\right]+\left[D_{ijk}\right]+\left[E_{ijk}\right]=0$
   to obtain $g_{y^iu^j\sigma^k}$. First we calculate conditional
   $G_{y^iu^j\sigma^k}$ (missing only for dimension $l=1$), then we
   evaluate conditional $F_{y^iu^j\sigma^k}$. Before we can calculate
   $D_{ijk}$, and $E_{ijk}$, we need to insert
   $G_{y^iu^ju'^m\sigma^{k-m}}$ for $m=1,\ldots, k$. This is done by
   |fillG|. Then we have right hand side and we multiply by $A^{-1}$ to
   obtain $g_{y^iu^j\sigma^k}$. Finally we have to update
   $G_{y^iu^j\sigma^k}$ by |multAndAdd| for dimension $l=1$.

   {\bf Requires:} everything at order $\leq i+j+k$, $g_{y^{i+j}\sigma^k}$
   through $G_{y^iu^j\sigma^k}$ involved in right hand side, then
   $g_{y^iu^{j+k}}$ through $D_{ijk}$, and $g_{y^iu^{j+m}\sigma^{k-m}}$
   for $m=1,\ldots,k-1$ through $E_{ijk}$.

   {\bf Provides:} $g_{y^iu^j\sigma^k}$, $G_{y^iu^j\sigma^k}$, and
   $G_{y^iu^ju'^m\sigma^{k-m}}$ for $m=1,\ldots, k$ */

template <Storage t>
void
KOrder::recover_yus(int i, int j, int k)
{
  Symmetry sym{i, j, 0, k};
  JournalRecordPair pa(journal);
  pa << "Recovering symmetry " << sym << endrec;

  fillG<t>(i, j, k);

  if (is_even(k))
    {
      auto G_yiujsk = faaDiBrunoG<t>(sym);
      auto G_yiujsk_ptr = G_yiujsk.get();
      G<t>().insert(std::move(G_yiujsk));

      auto g_yiujsk = faaDiBrunoZ<t>(sym);

      {
        auto D_ijk = calcD_ijk<t>(i, j, k);
        g_yiujsk->add(1.0, D_ijk);
      }

      if (k >= 3)
        {
          auto E_ijk = calcE_ijk<t>(i, j, k);
          g_yiujsk->add(1.0, E_ijk);
        }

      g_yiujsk->mult(-1.0);

      matA.multInv(*g_yiujsk);
      insertDerivative<t>(std::move(g_yiujsk));

      Gstack<t>().multAndAdd(1, gss<t>(), *G_yiujsk_ptr);
    }
}

/* Here we solve
   $\left[F_{\sigma^i}\right]+\left[D_i\right]+\left[E_i\right]=0$ to
   recover $g_{\sigma^i}$. First we calculate conditional $G_{\sigma^i}$
   (missing dimension $l=1$ and $l=i$), then we calculate conditional
   $F_{\sigma^i}$. Before we can calculate $D_i$ and $E_i$, we have to
   obtain $G_{u'm\sigma^{i-m}}$ for $m=1,\ldots,i$. Than
   adding $D_i$ and $E_i$ we have the right hand side. We solve by
   $S^{-1}$ multiplication and update $G_{\sigma^i}$ by calling
   |multAndAdd| for dimension $l=1$.

   Recall that the solved equation here is:
   $$
   \left[f_y\right]\left[g_{\sigma^k}\right]+
   \left[f_{y^{**}_+}\right]\left[g^{**}_{y^*}\right]\left[g^*_{\sigma^k}\right]+
   \left[f_{y^{**}_+}\right]\left[g^{**}_{\sigma^k}\right]=\hbox{RHS}
   $$
   This is a sort of deficient sylvester equation (sylvester equation for
   dimension=0), we solve it by $S^{-1}$. See |@<|MatrixS| constructor
   code@>| to see how $S$ looks like.

   {\bf Requires:} everything at order $\leq i-1$, $g_{y^i}$ and
   $g_{y^{i-j}\sigma^j}$, then $g_{u^k}$ through $F_{u'^k}$, and
   $g_{y^mu^j\sigma^k}$ for $j=1,\ldots,i-1$ and $m+j+k=i$ through
   $F_{u'j\sigma^{i-j}}$.

   {\bf Provides:} $g_{\sigma^i}$, $G_{\sigma^i}$, and
   $G_{u'^m\sigma^{i-m}}$ for $m=1,\ldots,i$ */

template <Storage t>
void
KOrder::recover_s(int i)
{
  Symmetry sym{0, 0, 0, i};
  JournalRecordPair pa(journal);
  pa << "Recovering symmetry " << sym << endrec;

  fillG<t>(0, 0, i);

  if (is_even(i))
    {
      auto G_si = faaDiBrunoG<t>(sym);
      auto G_si_ptr = G_si.get();
      G<t>().insert(std::move(G_si));

      auto g_si = faaDiBrunoZ<t>(sym);

      {
        auto D_i = calcD_k<t>(i);
        g_si->add(1.0, D_i);
      }

      if (i >= 3)
        {
          auto E_i = calcE_k<t>(i);
          g_si->add(1.0, E_i);
        }

      g_si->mult(-1.0);

      matS.multInv(*g_si);
      insertDerivative<t>(std::move(g_si));

      Gstack<t>().multAndAdd(1, gss<t>(), *G_si_ptr);
      Gstack<t>().multAndAdd(i, gss<t>(), *G_si_ptr);
    }
}

/* Here we calculate and insert $G_{y^iu^ju'^m\sigma^{k-m}}$ for
   $m=1,\ldots, k$. The derivatives are inserted only for $k-m$ being
   even. */

template<Storage t>
void
KOrder::fillG(int i, int j, int k)
{
  for (int m = 1; m <= k; m++)
    if (is_even(k-m))
      {
        auto G_yiujupms = faaDiBrunoG<t>(Symmetry{i, j, m, k-m});
        G<t>().insert(std::move(G_yiujupms));
      }
}

/* Here we calculate
   $$\left[D_{ijk}\right]_{\alpha_1\ldots\alpha_i\beta_1\ldots\beta_j}=
   \left[F_{y^iu^ju'^k}\right]
   _{\alpha_1\ldots\alpha_i\beta_1\ldots\beta_j\gamma_1\ldots\gamma_k}
   \left[\Sigma\right]^{\gamma_1\ldots\gamma_k}$$
   So it is non zero only for even $k$. */

template <Storage t>
typename ctraits<t>::Ttensor
KOrder::calcD_ijk(int i, int j, int k) const
{
  typename ctraits<t>::Ttensor res(ny, TensorDimens(Symmetry{i, j, 0, 0}, nvs));
  res.zeros();
  if (is_even(k))
    {
      auto tmp = faaDiBrunoZ<t>(Symmetry{i, j, k, 0});
      tmp->contractAndAdd(2, res, m<t>().get(Symmetry{k}));
    }
  return res;
}

/* Here we calculate
   $$\left[E_{ijk}\right]_{\alpha_1\ldots\alpha_i\beta_1\ldots\beta_j}=
   \sum_{m=1}^{k-1}\left(\matrix{k\cr m}\right)\left[F_{y^iu^ju'^m\sigma^{k-m}}\right]
   _{\alpha_1\ldots\alpha_i\beta_1\ldots\beta_j\gamma_1\ldots\gamma_m}
   \left[\Sigma\right]^{\gamma_1\ldots\gamma_m}$$
   The sum can sum only for even $m$. */

template <Storage t>
typename ctraits<t>::Ttensor
KOrder::calcE_ijk(int i, int j, int k) const
{
  typename ctraits<t>::Ttensor res(ny, TensorDimens(Symmetry{i, j, 0, 0}, nvs));
  res.zeros();
  for (int n = 2; n <= k-1; n += 2)
    {
      auto tmp = faaDiBrunoZ<t>(Symmetry{i, j, n, k-n});
      tmp->mult(static_cast<double>(PascalTriangle::noverk(k, n)));
      tmp->contractAndAdd(2, res, m<t>().get(Symmetry{n}));
    }
  return res;
}

template <Storage t>
typename ctraits<t>::Ttensor
KOrder::calcD_ik(int i, int k) const
{
  return calcD_ijk<t>(i, 0, k);
}

template <Storage t>
typename ctraits<t>::Ttensor
KOrder::calcD_k(int k) const
{
  return calcD_ijk<t>(0, 0, k);
}

template <Storage t>
typename ctraits<t>::Ttensor
KOrder::calcE_ik(int i, int k) const
{
  return calcE_ijk<t>(i, 0, k);
}

template <Storage t>
typename ctraits<t>::Ttensor
KOrder::calcE_k(int k) const
{
  return calcE_ijk<t>(0, 0, k);
}

/* Here is the core routine. It calls methods recovering derivatives in
   the right order. Recall, that the code, namely Faa Di Bruno's formula,
   is implemented as to be run conditionally on the current contents of
   containers. So, if some call of Faa Di Bruno evaluates derivatives,
   and some derivatives are not present in the container, then it is
   considered to be zero. So, we have to be very careful to put
   everything in the right order. The order here can be derived from
   dependencies, or it is in the paper.

   The method recovers all the derivatives of the given |order|.

   The precondition of the method is that all tensors of order |order-1|,
   which are not zero, exist (including $G$). The postcondition of of the
   method is derivatives of $g$ and $G$ of order |order| are calculated
   and stored in the containers. Responsibility of precondition lays upon
   the constructor (for |order==2|), or upon the previous call of
   |performStep|.

   From the code, it is clear, that all $g$ are calculated. If one goes
   through all the recovering methods, he should find out that also all
   $G$ are provided. */

template <Storage t>
void
KOrder::performStep(int order)
{
  KORD_RAISE_IF(order-1 != g<t>().getMaxDim(),
                "Wrong order for KOrder::performStep");
  JournalRecordPair pa(journal);
  pa << "Performing step for order = " << order << endrec;

  recover_y<t>(order);

  for (int i = 0; i < order; i++)
    recover_yu<t>(i, order-i);

  for (int j = 1; j < order; j++)
    {
      for (int i = j-1; i >= 1; i--)
        recover_yus<t>(order-j, i, j-i);
      recover_ys<t>(order-j, j);
    }

  for (int i = order-1; i >= 1; i--)
    recover_yus<t>(0, i, order-i);

  recover_s<t>(order);
}

/* Here we check for residuals of all the solved equations at the given
   order. The method returns the largest residual size. Each check simply
   evaluates the equation. */

template <Storage t>
double
KOrder::check(int dim) const
{
  KORD_RAISE_IF(dim > g<t>().getMaxDim(),
                "Wrong dimension for KOrder::check");
  JournalRecordPair pa(journal);
  pa << "Checking residuals for order = " << dim << endrec;

  double maxerror = 0.0;

  // check for $F_{y^iu^j}=0
  for (int i = 0; i <= dim; i++)
    {
      Symmetry sym{dim-i, i, 0,  0};
      auto r = faaDiBrunoZ<t>(sym);
      double err = r->getData().getMax();
      JournalRecord(journal) << "\terror for symmetry " << sym << "\tis " << err << endrec;
      if (err > maxerror)
        maxerror = err;
    }

  // check for $F_{y^iu^ju'^k}+D_{ijk}+E_{ijk}=0$
  for (auto &si : SymmetrySet(dim, 3))
    {
      int i = si[0];
      int j = si[1];
      int k = si[2];
      if (i+j > 0 && k > 0)
        {
          Symmetry sym{i, j, 0, k};
          auto r = faaDiBrunoZ<t>(sym);
          auto D_ijk = calcD_ijk<t>(i, j, k);
          r->add(1.0, D_ijk);
          auto E_ijk = calcE_ijk<t>(i, j, k);
          r->add(1.0, E_ijk);
          double err = r->getData().getMax();
          JournalRecord(journal) << "\terror for symmetry " << sym << "\tis " << err << endrec;
          maxerror = std::max(err, maxerror);
        }
    }

  // check for $F_{\sigma^i}+D_i+E_i=0
  auto r = faaDiBrunoZ<t>(Symmetry{0, 0, 0, dim});
  auto D_k = calcD_k<t>(dim);
  r->add(1.0, D_k);
  auto E_k = calcE_k<t>(dim);
  r->add(1.0, E_k);
  double err = r->getData().getMax();
  Symmetry sym{0, 0, 0, dim};
  JournalRecord(journal) << "\terror for symmetry " << sym << "\tis " << err << endrec;
  if (err > maxerror)
    maxerror = err;

  return maxerror;
}

template <Storage t>
Vector
KOrder::calcStochShift(int order, double sigma) const
{
  Vector res(ny);
  res.zeros();
  int jfac = 1;
  for (int j = 1; j <= order; j++, jfac *= j)
    if (is_even(j))
      {
        auto ten = calcD_k<t>(j);
        res.add(std::pow(sigma, j)/jfac, ten.getData());
      }
  return res;
}

#endif
