/*
 * Copyright © 2006-2011 Ondra Kamenik
 * Copyright © 2019 Dynare Team
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
 * along with Dynare.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PLANNER_BUILDER_H
#define PLANNER_BUILDER_H

#include <unordered_set>
#include <map>
#include <vector>
#include <memory>
#include <algorithm>

#include "parser/cc/static_fine_atoms.hh"
#include "dynare_atoms.hh"
#include "GeneralMatrix.hh"

namespace ogdyn
{
  using std::unordered_set;
  using std::map;
  using std::vector;

  /** This is a two dimensional array of integers. Nothing
   * difficult. */
  class IntegerMatrix
  {
  protected:
    /** Number of rows. */
    int nr;
    /** Number of columns. */
    int nc;
    /** The pointer to the data. */
    std::unique_ptr<int[]> data;
  public:
    /** Construct uninitialized array. */
    IntegerMatrix(int nrr, int ncc)
      : nr(nrr), nc(ncc), data(std::make_unique<int[]>(nr*nc))
    {
    }
    /** Copy constructor. */
    IntegerMatrix(const IntegerMatrix &im)
      : nr(im.nr), nc(im.nc), data(std::make_unique<int[]>(nr*nc))
    {
      std::copy_n(im.data.get(), nr*nc, data.get());
    }
    /** Assignment operator. It can only assing array with the
     * same dimensions. */
    const IntegerMatrix &operator=(const IntegerMatrix &im);
    int &
    operator()(int i, int j)
    {
      return data[i+j*nr];
    }
    const int &
    operator()(int i, int j) const
    {
      return data[i+j*nr];
    }
    int
    nrows() const
    {
      return nr;
    }
    int
    ncols() const
    {
      return nc;
    }
  };

  /** The three dimensional array of integers. Nothing difficult. */
  class IntegerArray3
  {
  protected:
    /** First dimension. */
    int n1;
    /** Second dimension. */
    int n2;
    /** Third dimension. */
    int n3;
    /** The data. */
    std::unique_ptr<int[]> data;
  public:
    /** Constrcut unitialized array. */
    IntegerArray3(int nn1, int nn2, int nn3)
      : n1(nn1), n2(nn2), n3(nn3), data(std::make_unique<int[]>(n1*n2*n3))
    {
    }
    /** Copy constructor. */
    IntegerArray3(const IntegerArray3 &ia3)
      : n1(ia3.n1), n2(ia3.n2), n3(ia3.n3), data(std::make_unique<int[]>(n1*n2*n3))
    {
      std::copy_n(ia3.data.get(), n1*n2*n3, data.get());
    }
    /** Assignment operator assigning the arrays with the same dimensions. */
    const IntegerArray3 &operator=(const IntegerArray3 &ia3);
    int &
    operator()(int i, int j, int k)
    {
      return data[i+j*n1+k*n1*n2];
    }
    const int &
    operator()(int i, int j, int k) const
    {
      return data[i+j*n1+k*n1*n2];
    }
    int
    dim1() const
    {
      return n1;
    }
    int
    dim2() const
    {
      return n2;
    }
    int
    dim3() const
    {
      return n3;
    }
  };

  /** This struct encapsulates information about the building of a
   * planner's problem. */
  struct PlannerInfo
  {
    int num_lagrange_mults{0};
    int num_aux_variables{0};
    int num_new_terms{0};
  };

  class MultInitSS;
  class DynareModel;

  /** This class builds the first order conditions of the social
   * planner problem with constraints being the equations in the
   * model. The model is non-const parameter to the constructor
   * which adds appropriate FOCs to the system. It also allows for
   * an estimation of the lagrange multipliers given all other
   * endogenous variables of the static system. For this purpose we
   * need to create static atoms and static versions of all the tree
   * index matrices. The algorithm and algebra are documented in
   * dynare++-ramsey.pdf. */
  class PlannerBuilder
  {
    friend class MultInitSS;
  public:
    /** Type for a set of variable names. */
    using Tvarset = unordered_set<string>;
    /** Type for a set of equations. An equation is identified by
     * an index to an equation in the equation vector given by
     * DynareModel::eqs. The tree index of the i-th formula is
     * retrieved as DynareModel::egs.formula(i). */
    using Teqset = vector<int>;
  protected:
    /** This is a set of variables wrt which the planner
     * optimizes. These could be all endogenous variables, but it
     * is beneficial to exclude all variables which are
     * deterministic transformations of past exogenous variables,
     * since the planner cannot influence them. This could save a
     * few equations. This is not changed after it is constructed,
     * but it is constructed manually, so it cannot be declared as
     * const. */
    Tvarset yset;
    /** These are the equation indices constituing the constraints
     * for the planner. Again, it is beneficial to exclude all
     * equations defining exogenous variables excluded from
     * yset. */
    const Teqset fset;
    /** Reference to the model. */
    ogdyn::DynareModel &model;
    /** Tree index of the planner objective. */
    int tb;
    /** Tree index of the planner discount parameter. */
    int tbeta;
    /** The maximum lead in the model including the planner's
     * objective before building the planner's FOCs. */
    const int maxlead;
    /** The minimum lag in the model including the planner's objective
     * before building the planner's FOCs. */
    const int minlag;
    /** Tree indices of formulas in the planner FOCs involving
     * derivatives of the planner's objective. Rows correspond to the
     * endogenous variables, columns correspond to lags in the
     * objective function. The contents of the matrix will evolve as
     * the algorithm proceeds. */
    IntegerMatrix diff_b;
    /** Tree indices of formulas in the planner FOCs involving
     * derivatives of the model equations (constraints). The first
     * dimension corresponds to endogenous variables, the second to
     * the constraints, the third to lags or leads of endogenous
     * variables in the constraints. The contents of the array will
     * evolve as the algorithm proceeds.*/
    IntegerArray3 diff_f;
    /** Static version of the model atoms. It is needed to build
     * static version of diff_b and diff_f. */
    ogp::StaticFineAtoms static_atoms;
    /** Static version of all the trees of diff_b and diff_f build
     * over static_atoms. */
    ogp::OperationTree static_tree;
    /** Tree indices of static version of diff_b over static_atoms and static_tree. */
    IntegerMatrix diff_b_static;
    /** Tree indices of static version of diff_f over static_atoms
     * and static_tree. This member is created before calling
     * lagrange_mult_f(), so it does not contain the
     * multiplication with the lagrange multipliers. */
    IntegerArray3 diff_f_static;
    /** Auxiliary variables mapping. During the algorithm, some
     * auxiliary variables for the terms might be created, so we
     * remember their names and tree indices of the terms. This
     * maps a name to the tree index of an expression equal to the
     * auxiliary variable at time zero. The auxiliary variables
     * names point to the dynamic atoms storage, tree inidices to
     * the dynamic model tree. */
    Tsubstmap aux_map;
    /** Static version of aux_map. The names point to static_atoms
     * storage, the tree indices to the static_tree. */
    Tsubstmap static_aux_map;
    /** Information about the number of various things. */
    PlannerInfo info;
  public:
    /** Build the planner problem for the given model optimizing
     * through the given endogenous variables with the given
     * constraints. We allow for a selection of a subset of
     * equations and variables in order to eliminate exogenous
     * predetermined process which cannot be influenced by the
     * social planner. */
    PlannerBuilder(ogdyn::DynareModel &m, const Tvarset &yyset,
                   Teqset ffset);
    /** Construct a copy of the builder with provided model, which
     * is supposed to be the copy of the model in the builder. */
    PlannerBuilder(const PlannerBuilder &pb, ogdyn::DynareModel &m);
    /** Avoid copying from only PlannerBuilder. */
    PlannerBuilder(const PlannerBuilder &pb) = delete;
    /** Return the information. */
    const PlannerInfo &
    get_info() const
    {
      return info;
    }
  protected:
    /** Differentiate the planner objective wrt endogenous
     * variables with different lags. */
    void add_derivatives_of_b();
    /** Differentiate the constraints wrt endogenous variables
     * with different lags and leads. */
    void add_derivatives_of_f();
    /** Shift derivatives of diff_b. */
    void shift_derivatives_of_b();
    /** Shift derivatives of diff_ff. */
    void shift_derivatives_of_f();
    /** Multiply with the discount factor terms in diff_b. */
    void beta_multiply_b();
    /** Multiply with the discount factor terms in diff_f. */
    void beta_multiply_f();
    /** Fill static_atoms and static_tree and build diff_b_static,
     * diff_f_static and aux_map_static with static versions of diff_b,
     * diff_f and aux_map. */
    void make_static_version();
    /** Multiply diff_f with Langrange multipliers. */
    void lagrange_mult_f();
    /** Add the equations to the mode, including equation for auxiliary variables. */
    void form_equations();
  private:
    /** Fill yset for a given yyset and given name storage. */
    void fill_yset(const ogp::NameStorage &ns, const Tvarset &yyset);
    /** Fill aux_map and aux_map_static for a given aaux_map and
     * aaux_map_static for a given storage of dynamic atoms (used
     * for aux_map) and static atoms storage from this object for
     * aux_map_static. */
    void fill_aux_map(const ogp::NameStorage &ns, const Tsubstmap &aaux_map,
                      const Tsubstmap &astatic_aux_map);
  };

  /** This class only calculates for the given initial guess of
   * endogenous variables, initial guess of the Langrange
   * multipliers of the social planner problem yielding the least
   * square error. It is used by just calling its constructor. The
   * constructor takes non-const reference to the vector of
   * endogenous variables, calculates lambdas and put the values of
   * lambdas to the vector. The algbera is found in
   * dynare++-ramsey.pdf.
   *
   * The code can be run only after the parsing has been finished in
   * atoms. */
  class MultInitSS : public ogp::FormulaEvalLoader
  {
  protected:
    /** The constant reference to the builder. */
    const PlannerBuilder &builder;
    /** The constant term of the problem. Its length is the number
     * of endogenous variable wrt the planner optimizes. */
    Vector b;
    /** The matrix of the overdetermined problem. The number of
     * rows is equal to the number of endogenous variables wrt
     * which the planner optimizes, the number of columns is equal
     * to the number of Langrange multipliers which is equal to
     * the number of constraints which is smaller than the number
     * of endogenous variables. Hence the system b+F*lambda=0 is
     * overdetermined. */
    GeneralMatrix F;
  public:
    /** The constructor of the object which does everything. Its
     * main goal is to update yy. Note that if an item of yy
     * corresponding to a lagrange multiplier is already set, it
     * is not reset. */
    MultInitSS(const PlannerBuilder &pb, const Vector &pvals, Vector &yy);
    /** This loads evaluated parts of b or F and decodes i and
     * advances b or F depending on the decoded i. The decoding is
     * dependent on the way how the terms of builder.diff_b and
     * builder.diff_f_save have been put the the
     * ogp::FormulaCustomEvaluator. This is documented in the code
     * of the constructor. */
    void load(int i, double res) override;
  };
};

#endif
