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

#ifndef OGP_ATOM_ASSIGNINGS_H
#define OGP_ATOM_ASSIGNINGS_H

#include "static_atoms.hh"
#include "formula_parser.hh"
#include "atom_substitutions.hh"

#include <vector>
#include <map>

namespace ogp
{
  class AtomAsgnEvaluator;

  /** This class represents atom assignments used in parameters
   * settings and initval initialization. It maintains atoms of the
   * all expressions on the right hand side, the parsed formulas of
   * the right hand sides, and the information about the left hand
   * sides. See documentation to the order member below. */
  class AtomAssignings
  {
    friend class AtomAsgnEvaluator;
  protected:
    using Tvarintmap = std::map<string, int>;
    /** All atoms which should be sufficient for formulas at the
     * right hand sides. The atoms should be filled with names
     * (preregistered). This is a responsibility of the caller. */
    StaticAtoms &atoms;
    /** The formulas of right hand sides. */
    FormulaParser expr;
    /** Name storage of the names from left hand sides. */
    NameStorage left_names;
    /** Information on left hand sides. This maps a name to the
     * index of its assigned expression in expr. More than one
     * name may reference to the same expression. */
    Tvarintmap lname2expr;
    /** Information on left hand sides. If order[i] >= 0, then it
     * says that i-th expression in expr is assigned to atom with
     * order[i] tree index. */
    std::vector<int> order;
  public:
    /** Construct the object using the provided static atoms. */
    AtomAssignings(StaticAtoms &a) : atoms(a), expr(atoms)
    {
    }
    /** Make a copy with provided reference to (posibly different)
     * static atoms. */
    AtomAssignings(const AtomAssignings &aa, StaticAtoms &a);
    virtual ~AtomAssignings() = default;
    /** Parse the assignments from the given string. */
    void parse(const string &stream);
    /** Process a syntax error from bison. */
    void error(string mes);
    /** Add an assignment of the given name to the given
     * double. Can be called by a user, anytime. */
    void add_assignment_to_double(string name, double val);
    /** Add an assignment. Called from assign.y. */
    void add_assignment(int asgn_off, const string &str, int name_len,
                        int right_off, int right_len);
    /** This applies old2new map (possibly from atom
     * substitutions) to this object. It registers new variables
     * in the atoms, and adds the expressions to expr, and left
     * names to lname2expr. The information about dynamical part
     * of substitutions is ignored, since we are now in the static
     * world. */
    void apply_subst(const AtomSubstitutions::Toldnamemap &mm);
    /** Debug print. */
    void print() const;
  };

  /** This class basically evaluates the atom assignments
   * AtomAssignings, so it inherits from ogp::FormulaEvaluator. It
   * is also a storage for the results of the evaluation stored as a
   * vector, so the class inherits from std::vector<double> and
   * ogp::FormulaEvalLoader. As the expressions for atoms are
   * evaluated, the results are values for atoms which will be
   * used in subsequent evaluations. For this reason, the class
   * inherits also from AtomValues. */
  class AtomAsgnEvaluator : public FormulaEvalLoader,
                            public AtomValues,
                            protected FormulaEvaluator,
                            public std::vector<double>
  {
  protected:
    using Tusrvalmap = std::map<int, double>;
    Tusrvalmap user_values;
    const AtomAssignings &aa;
  public:
    AtomAsgnEvaluator(const AtomAssignings &a)
      : FormulaEvaluator(a.expr),
        std::vector<double>(a.expr.nformulas()), aa(a)
    {
    }
    ~AtomAsgnEvaluator() override = default;
    /** This sets all initial values to NaNs, all constants and
     * all values set by user by call set_value. This is called by
     * FormulaEvaluator::eval() method, which is called by eval()
     * method passing this argument as AtomValues. So the
     * ogp::EvalTree will be always this->etree. */
    void setValues(EvalTree &et) const override;
    /** User setting of the values. For example in initval,
     * parameters are known and should be set to their values. In
     * constrast endogenous variables are set initially to NaNs by
     * AtomValues::setValues. */
    void set_user_value(const string &name, double val);
    /** This sets the result of i-th expression in aa to res, and
     * also checks whether the i-th expression is an atom. If so,
     * it sets the value of the atom in ogp::EvalTree
     * this->etree. */
    void load(int i, double res) override;
    /** After the user values have been set, the assignments can
     * be evaluated. For this purpose we have eval() method. The
     * result is that this object as std::vector<double> will
     * contain the values. It is ordered given by formulas in
     * expr. */
    void
    eval()
    {
      FormulaEvaluator::eval(*this, *this);
    }
    /** This returns a value for a given name. If the name is not
     * found among atoms, or there is no assignment for the atom,
     * NaN is returned. */
    double get_value(const string &name) const;
  };
};

#endif
