// Copyright 2004, Ondra Kamenik

// Moments of normal distribution.

/* Here we calculate the higher order moments of normally distributed
   random vector u with means equal to zero and given
   variance-covariance matrix V, i.e. u↝𝒩(0,V). The moment
   generating function for such distribution is f(t)=e^½tᵀVt. If
   we derivate it w.r.t. t and unfold the higher dimensional tensors
   row-wise, we obtain terms like:

    ∂f(t)/∂t = f(t)·Vt
    ∂²f(t)/∂t² = f(t)·(Vt⊗v)
    ∂³f(t)/∂t³ = f(t)·(Vt⊗Vt⊗Vt + P(v⊗Vt) + P(Vt⊗v) + v⊗Vt)
    ∂⁴f(t)/∂t⁴ = f(t)·(Vt⊗Vt⊗Vt⊗Vt + S(v⊗Vt⊗Vt) + S(Vt⊗v⊗Vt) + S(Vt⊗Vt⊗v) + S(v⊗v))

   where v is vectorized V (v=vec(V)), and P is a suitable row permutation
   (corresponds to permutation of multidimensional indices) which permutes the
   tensor data, so that the index of a variable being derived would be the
   last. This ensures that all (permuted) tensors can be summed yielding a
   tensor whose indices have some order (in here we chose the order that more
   recent derivating variables are to the right). Finally, S is a suitable sum
   of various P.

   We are interested in S multiplying the Kronecker powers ⊗ⁿv. The S is a
   (possibly) multi-set of permutations of even order. Note that we know the
   number of permutations in S. The above formulas for f(t) derivatives are
   valid also for monomial u, and from literature we know that 2n-th moment is
   (2n!)/(n!2ⁿ)·σ². So there are (2n!)/(n!2ⁿ) permutations in S.

   In order to find the $S_?$ we need to define a couple of
   things. First we define a sort of equivalence between the permutations
   applicable to even number of indices. We write $P_1\equiv P_2$
   whenever $P_1^{-1}\circ P_2$ permutes only whole pairs, or items
   within pairs, but not indices across the pairs. For instance the
   permutations $(0,1,2,3)$ and $(3,2,0,1)$ are equivalent, but
   $(0,2,1,3)$ is not equivalent with the two. Clearly, the $\equiv$ is
   an equivalence.

   This allows to define a relation $\sqsubseteq$ between the permutation
   multi-sets $S$, which is basically the subset relation $\subseteq$ but
   with respect to the equivalence $\equiv$, more formally:
   $$S_1\sqsubseteq S_2\quad\hbox{iff}\quad P\in S_1
   \Rightarrow\exists Q\in S_2:P\equiv Q$$
   This induces an equivalence $S_1\equiv S_2$.

   Now let $F_n$ denote a set of permutations on $2n$ indices which is
   maximal with respect to $\sqsubseteq$, and minimal with respect to
   $\equiv$. (In other words, it contains everything up to the
   equivalence $\equiv$.) It is straightforward to calculate a number of
   permutations in $F_n$. This is a total number of all permutations of
   $2n$ divided by permutations of pairs divided by permutations within
   the pairs. This is ${(2n!)\over n!2^n}$.

   We prove that $S_?\equiv F_n$. Clearly $S_?\sqsubseteq F_n$, since
   $F_n$ is maximal. In order to prove that $F_n\sqsubseteq S_?$, let us
   assert that for any permutation $P$ and for any (semi)positive
   definite matrix $V$ we have $PS_?\otimes^nv=S_?\otimes^nv$. Below we
   show that there is a positive definite matrix $V$ of some dimension
   that for any two permutation multi-sets $S_1$, $S_2$, we have
   $$S_1\not\equiv S_2\Rightarrow S_1(\otimes^nv)\neq S_2(\otimes^nv)$$
   So it follows that for any permutation $P$, we have $PS_?\equiv
   S_?$. For a purpose of contradiction let $P\in F_n$ be a permutation
   which is not equivalent to any permutation from $S_?$. Since $S_?$ is
   non-empty, let us pick $P_0\in S_?$. Now assert that
   $P_0^{-1}S_?\not\equiv P^{-1}S_?$ since the first contains an identity
   and the second does not contain a permutation equivalent to
   identity. Thus we have $(P\circ P_0^{-1})S_?\not\equiv S_?$ which
   gives the contradiction and we have proved that $F_n\sqsubseteq
   S_?$. Thus $F_n\equiv S_?$. Moreover, we know that $S_?$ and $F_n$
   have the same number of permutations, hence the minimality of $S_?$
   with respect to $\equiv$.

   Now it suffices to prove that there exists a positive definite $V$
   such that for any two permutation multi-sets $S_1$, and $S_2$ holds
   $S_1\not\equiv S_2\Rightarrow S_1(\otimes^nv)\neq S_2(\otimes^nv)$. If
   $V$ is $n\times n$ matrix, then $S_1\not\equiv S_2$ implies that there
   is identically nonzero polynomial of elements from $V$ of order $n$
   over integers. If $V=A^TA$ then there is identically non-zero
   polynomial of elements from $A$ of order $2n$. This means, that we
   have to find $n(n+1)/2$ tuple $x$ of real numbers such that all
   identically non-zero polynomials $p$ of order $2n$ over integers yield
   $p(x)\neq 0$.

   The $x$ is constructed as follows: $x_i = \pi^{\log{r_i}}$, where $r_i$
   is $i$-th prime. Let us consider monom $x_1^{j_1}\cdot\ldots\cdot
   x_k^{j_k}$. When the monom is evaluated, we get
   $$\pi^{\log{r_1^{j_1}}+\ldots+\log{r_k^{j_k}}}=
   \pi^{\log{\left(r_1^{j_1}\cdot\ldots\cdot r_k^{j_k}\right)}}$$
   Now it is easy to see that if an integer combination of such terms is
   zero, then the combination must be either trivial or sum to $0$ and
   all monoms must be equal. Both cases imply a polynomial identically
   equal to zero. So, any non-trivial integer polynomial evaluated at $x$
   must be non-zero.

   So, having this result in hand, now it is straightforward to calculate
   higher moments of normal distribution. Here we define a container,
   which does the job. In its constructor, we simply calculate Kronecker
   powers of $v$ and apply $F_n$ to $\otimes^nv$. $F_n$ is, in fact, a
   set of all equivalences in sense of class |Equivalence| over $2n$
   elements, having $n$ classes each of them having exactly 2 elements. */

#ifndef NORMAL_MOMENTS_H
#define NORMAL_MOMENTS_H

#include "t_container.hh"

class UNormalMoments : public TensorContainer<URSingleTensor>
{
public:
  UNormalMoments(int maxdim, const TwoDMatrix &v);
private:
  void generateMoments(int maxdim, const TwoDMatrix &v);
  static bool selectEquiv(const Equivalence &e);
};

class FNormalMoments : public TensorContainer<FRSingleTensor>
{
public:
  FNormalMoments(const UNormalMoments &moms);
};

#endif
