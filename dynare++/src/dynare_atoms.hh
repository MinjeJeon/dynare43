/*
 * Copyright © 2006 Ondra Kamenik
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

#ifndef OGDYN_DYNARE_ATOMS_H
#define OGDYN_DYNARE_ATOMS_H

#include "sylv/cc/Vector.hh"

#include "parser/cc/static_atoms.hh"
#include "parser/cc/static_fine_atoms.hh"
#include "parser/cc/atom_substitutions.hh"
#include "parser/cc/tree.hh"

#include <map>
#include <vector>

namespace ogdyn
{
  using std::map;
  using std::vector;
  using std::string;

  /* A definition of a type mapping a string to an integer. Used as a
     substitution map, saying what names are substituted for what expressions
     represented by tree indices. */
  using Tsubstmap = map<string, int>;

  class DynareStaticAtoms : public ogp::StaticAtoms
  {
  public:
    DynareStaticAtoms()
      : StaticAtoms()
    {
    }
    DynareStaticAtoms(const DynareStaticAtoms &a) = default;
    ~DynareStaticAtoms() override = default;
    /* This registers a unique varname identifier. It throws an exception if
       the variable name is duplicate. It checks the uniqueness and then it
       calls StaticAtoms::register_name. */
    void register_name(string name) override;
  protected:
    /* This returns a tree index of the given variable, and if the variable has
       not been registered, it throws an exception. */
    int check_variable(const string &name) const override;
  };

  class DynareDynamicAtoms : public ogp::SAtoms, public ogp::NularyStringConvertor
  {
  public:
    enum class atype { endovar, exovar, param };
  protected:
    using Tatypemap = map<string, atype>;
    /* The map assigining a type to each name. */
    Tatypemap atom_type;
  public:
    DynareDynamicAtoms()
      : ogp::SAtoms()
    {
    }
    /* This parses a variable of the forms: varname(+3), varname(3), varname,
       varname(-3), varname(0), varname(+0), varname(-0). */
    void parse_variable(const string &in, std::string &out, int &ll) const override;
    /* Registers unique name of endogenous variable. */
    void register_uniq_endo(string name) override;
    /* Registers unique name of exogenous variable. */
    void register_uniq_exo(string name) override;
    /* Registers unique name of parameter. */
    void register_uniq_param(string name) override;
    /* Return true if the name is a given type. */
    bool is_type(const string &name, atype tp) const;
    /* Debug print. */
    void print() const override;
    /* Implement NularyStringConvertor::convert. */
    std::string convert(int t) const override;
  };

  /* This class represents the atom values for dynare, where exogenous
     variables can occur only at time t, and endogenous at times t−1, t, and
     t+1. */
  class DynareAtomValues : public ogp::AtomValues
  {
  protected:
    /* Reference to the atoms (we suppose that they are only at t−1,t,t+1. */
    const ogp::FineAtoms &atoms;
    /* De facto reference to the values of parameters. */
    const ConstVector paramvals;
    /* De facto reference to the values of endogenous at time t−1. Only
       predetermined and both part. */
    const ConstVector yym;
    /* De facto reference to the values of endogenous at time t. Ordering given
       by the atoms. */
    const ConstVector yy;
    /* De facto reference to the values of endogenous at time t+1. Only both
       and forward looking part. */
    const ConstVector yyp;
    /* De facto reference to the values of exogenous at time t. */
    const ConstVector xx;
  public:
    DynareAtomValues(const ogp::FineAtoms &a, const Vector &pvals, const Vector &ym,
                     const Vector &y, const Vector &yp, const Vector &x)
      : atoms(a), paramvals(pvals), yym(ym), yy(y), yyp(yp), xx(x)
    {
    }
    DynareAtomValues(const ogp::FineAtoms &a, const Vector &pvals, const ConstVector &ym,
                     const ConstVector &y, const ConstVector &yp, const Vector &x)
      : atoms(a), paramvals(pvals), yym(ym), yy(y), yyp(yp), xx(x)
    {
    }
    void setValues(ogp::EvalTree &et) const override;
  };

  /* This class represents the atom values at the steady state. It makes only
     appropriate subvector yym and yyp of the y vector, makes a vector of zero
     exogenous variables and uses DynareAtomValues with more general
     interface. */
  class DynareSteadyAtomValues : public ogp::AtomValues
  {
  protected:
    /* Subvector of yy. */
    const ConstVector yym;
    /* Subvector of yy. */
    const ConstVector yyp;
    /* Vector of zeros for exogenous variables. */
    Vector xx;
    /* Atom values using this yym, yyp and xx. */
    DynareAtomValues av;
  public:
    DynareSteadyAtomValues(const ogp::FineAtoms &a, const Vector &pvals, const Vector &y)
      : yym(y, a.nstat(), a.nys()),
        yyp(y, a.nstat()+a.npred(), a.nyss()),
        xx(a.nexo()),
        av(a, pvals, yym, y, yyp, xx)
    {
      xx.zeros();
    }
    void
    setValues(ogp::EvalTree &et) const override
    {
      av.setValues(et);
    }
  };

  class DynareStaticSteadyAtomValues : public ogp::AtomValues
  {
  protected:
    /* Reference to static atoms over which the tree, where the values go, is
       defined. */
    const ogp::StaticFineAtoms &atoms_static;
    /* Reference to dynamic atoms for which the class gets input data. */
    const ogp::FineAtoms &atoms;
    /* De facto reference to input data, this is a vector of endogenous
       variables in internal ordering of the dynamic atoms. */
    ConstVector yy;
    /* De facto reference to input parameters corresponding to ordering defined
       by the dynamic atoms. */
    ConstVector paramvals;
  public:
    /* Construct the object. */
    DynareStaticSteadyAtomValues(const ogp::FineAtoms &a, const ogp::StaticFineAtoms &sa,
                                 const Vector &pvals, const Vector &yyy)
      : atoms_static(sa),
        atoms(a),
        yy(yyy),
        paramvals(pvals)
    {
    }
    /* Set the values to the tree defined over the static atoms. */
    void setValues(ogp::EvalTree &et) const override;
  };

  /* This class takes a vector of endogenous variables and a substitution map.
     It supposes that variables at the right hand sides of the substitutions
     are set in the endogenous vector. It evaluates the substitutions and if
     the variables corresponding to left hand sides are not set in the
     endogenous vector it sets them to calculated values. If a variable is
     already set, it does not override its value. It has no methods, everything
     is done in the constructor. */
  class DynareSteadySubstitutions : public ogp::FormulaEvalLoader
  {
  protected:
    const ogp::FineAtoms &atoms;
  public:
    DynareSteadySubstitutions(const ogp::FineAtoms &a, const ogp::OperationTree &tree,
                              const Tsubstmap &subst,
                              const Vector &pvals, Vector &yy);
    void load(int i, double res) override;
  protected:
    Vector &y;
    vector<string> left_hand_sides;
    vector<int> right_hand_sides;
  };

  /* This class is a static version of DynareSteadySustitutions. It works for
     static atoms and static tree and substitution map over the static tree. It
     also needs dynamic version of the atoms, since it defines ordering of the
     vectors pvals, and yy. */
  class DynareStaticSteadySubstitutions : public ogp::FormulaEvalLoader
  {
  protected:
    const ogp::FineAtoms &atoms;
    const ogp::StaticFineAtoms &atoms_static;
  public:
    DynareStaticSteadySubstitutions(const ogp::FineAtoms &a,
                                    const ogp::StaticFineAtoms &sa,
                                    const ogp::OperationTree &tree,
                                    const Tsubstmap &subst,
                                    const Vector &pvals, Vector &yy);
    void load(int i, double res) override;
  protected:
    Vector &y;
    vector<string> left_hand_sides;
    vector<int> right_hand_sides;
  };

};

#endif
