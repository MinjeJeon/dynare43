/*
 * Copyright © 2005 Ondra Kamenik
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

#include "utils/cc/pascal_triangle.hh"
#include "utils/cc/exception.hh"

#include "parser_exception.hh"
#include "location.hh"
#include "formula_parser.hh"
#include "formula_tab.hh"

#include <cmath>
#include <algorithm>

using namespace ogp;

extern location_type fmla_lloc;

FormulaParser::FormulaParser(const FormulaParser &fp, Atoms &a)
  : otree(fp.otree), atoms(a), formulas(fp.formulas), ders()
{
  // create derivatives
  for (const auto &der : fp.ders)
    ders.push_back(std::make_unique<FormulaDerivatives>(*der));
}

void
FormulaParser::differentiate(int max_order)
{
  ders.clear();
  vector<int> vars;
  vars = atoms.variables();
  for (int formula : formulas)
    ders.push_back(std::make_unique<FormulaDerivatives>(otree, vars, formula, max_order));
}

const FormulaDerivatives &
FormulaParser::derivatives(int i) const
{
  if (i < static_cast<int>(ders.size()))
    return *(ders[i]);
  else
    throw ogu::Exception(__FILE__, __LINE__,
                         "Wrong formula index in FormulaParser::derivatives");
  return *(ders[0]); // just because of compiler
}

void
FormulaParser::add_formula(int t)
{
  formulas.push_back(t);
}

int
FormulaParser::add_binary(code_t code, int t1, int t2)
{
  return otree.add_binary(code, t1, t2);
}

int
FormulaParser::add_unary(code_t code, int t)
{
  return otree.add_unary(code, t);
}

int
FormulaParser::add_nulary(const string &str)
{
  int t = -1;
  try
    {
      t = atoms.check(str);
    }
  catch (const ParserException &e)
    {
      throw ParserException(e, fmla_lloc.off);
    }
  if (t == -1)
    {
      t = otree.add_nulary();
      atoms.assign(str, t);
    }
  return t;
}

void
FormulaParser::add_subst_formulas(const map<int, int> &subst, const FormulaParser &fp)
{
  for (int i = 0; i < fp.nformulas(); i++)
    {
      int f = add_substitution(fp.formula(i), subst, fp);
      add_formula(f);
    }
}

void
FormulaParser::substitute_formulas(const map<int, int> &smap)
{
  for (int i = 0; i < nformulas(); i++)
    {
      // make substitution and replace the formula for it
      int f = add_substitution(formulas[i], smap);
      formulas[i] = f;
      // update the derivatives if any
      if (i < static_cast<int>(ders.size()) && ders[i])
        {
          int order = ders[i]->get_order();
          ders[i] = std::make_unique<FormulaDerivatives>(otree, atoms.variables(), formulas[i], order);
        }
    }
}

/** Global symbols for passing info to parser. */
FormulaParser *fparser;

/** The declarations of functions defined in formula_ll.cc and
 * formula_tab.cc generated from formula.lex and formula.y */
void *fmla__scan_string(const char *);
void fmla__destroy_buffer(void *);
int fmla_parse();
extern location_type fmla_lloc;

/** This makes own copy of provided data, sets the buffer for the
 * parser with fmla_scan_buffer, and launches fmla_parse(). Note that
 * the pointer returned from fmla_scan_buffer must be freed at the
 * end. */
void
FormulaParser::parse(const string &stream)
{
  fmla_lloc.off = 0;
  fmla_lloc.ll = 0;
  void *p = fmla__scan_string(stream.c_str());
  fparser = this;
  fmla_parse();
  fmla__destroy_buffer(p);
}

void
FormulaParser::error(string mes) const
{
  throw ParserException(std::move(mes), fmla_lloc.off);
}

int
FormulaParser::last_formula() const
{
  int res = -1;
  for (int formula : formulas)
    res = std::max(res, formula);
  return std::max(res, otree.get_last_nulary());
}

int
FormulaParser::pop_last_formula()
{
  if (formulas.size() == 0)
    return -1;
  int t = formulas.back();
  if (formulas.size() == ders.size())
    ders.pop_back();
  formulas.pop_back();
  return t;
}

void
FormulaParser::print() const
{
  atoms.print();
  for (int formula : formulas)
    {
      std::cout << "formula " << formula << ":\n";
      otree.print_operation(formula);
    }
  for (unsigned int i = 0; i < ders.size(); i++)
    {
      std::cout << "derivatives for the formula " << formulas[i] << ":\n";
      ders[i]->print(otree);
    }
}

/** This constructor makes a vector of indices for formulas
 * corresponding to derivatives of the given formula. The formula is
 * supposed to belong to the provided tree, the created derivatives
 * are added to the tree.
 *
 * The algorithm is as follows. todo: update description of the
 * algorithm
 */
FormulaDerivatives::FormulaDerivatives(OperationTree &otree,
                                       const vector<int> &vars, int f, int max_order)
  : nvar(vars.size()), order(max_order)
{
  FoldMultiIndex fmi_zero(nvar);
  tder.push_back(f);
  indices.push_back(fmi_zero);
  unsigned int last_order_beg = 0;
  unsigned int last_order_end = tder.size();

  for (int k = 1; k <= order; k++)
    {
      // interval <last_order_beg,last_order_end) is guaranteed
      // here to contain at least one item
      for (unsigned int run = last_order_beg; run < last_order_end; run++)
        {
          // shift one order from the run
          FoldMultiIndex fmi(indices[run], 1);
          // set starting variable from the run, note that if k=1,
          // the shift order ctor of fmi will set it to zero
          int ivar_start = fmi[k-1];
          for (int ivar = ivar_start; ivar < nvar; ivar++, fmi.increment())
            {
              int der = otree.add_derivative(tder[run], vars[ivar]);
              if (der != OperationTree::zero)
                {
                  tder.push_back(der);
                  indices.push_back(fmi);
                }
            }
        }

      // set new last_order_beg and last_order_end
      last_order_beg = last_order_end;
      last_order_end = tder.size();
      // if there was no new derivative, break out from the loop
      if (last_order_beg >= last_order_end)
        break;
    }

  // build ind2der map
  for (unsigned int i = 0; i < indices.size(); i++)
    ind2der.emplace(indices[i], i);

}

FormulaDerivatives::FormulaDerivatives(const FormulaDerivatives &fd) = default;

int
FormulaDerivatives::derivative(const FoldMultiIndex &mi) const
{
  if (mi.order() > order)
    throw ogu::Exception(__FILE__, __LINE__,
                         "Wrong order of multi-index in FormulaDerivatives::derivative");
  if (mi.nv() != nvar)
    throw ogu::Exception(__FILE__, __LINE__,
                         "Wrong multi-index variables in FormulaDerivatives::derivative");

  auto it = ind2der.find(mi);
  if (it == ind2der.end())
    return OperationTree::zero;
  else
    return tder[it->second];
}

void
FormulaDerivatives::print(const OperationTree &otree) const
{
  for (const auto &it : ind2der)
    {
      std::cout << "derivative ";
      it.first.print();
      std::cout << " is formula " << tder[it.second] << '\n';
      otree.print_operation(tder[it.second]);
    }
}

void
FormulaCustomEvaluator::eval(const AtomValues &av, FormulaEvalLoader &loader)
{
  etree.reset_all();
  av.setValues(etree);
  for (unsigned int i = 0; i < terms.size(); i++)
    {
      double res = etree.eval(terms[i]);
      loader.load(static_cast<int>(i), res);
    }
}

FoldMultiIndex::FoldMultiIndex(int nv)
  : nvar(nv), ord(0), data(std::make_unique<int[]>(ord))
{
}

FoldMultiIndex::FoldMultiIndex(int nv, int ordd, int ii)
  : nvar(nv), ord(ordd), data(std::make_unique<int[]>(ord))
{
  for (int i = 0; i < ord; i++)
    data[i] = ii;
}

/** Note that a monotone sequence mapped by monotone mapping yields a
 * monotone sequence. */
FoldMultiIndex::FoldMultiIndex(int nv, const FoldMultiIndex &mi, const vector<int> &mp)
  : nvar(nv), ord(mi.ord), data(std::make_unique<int[]>(ord))
{
  for (int i = 0; i < ord; i++)
    {
      if (i < ord-1 && mp[i+1] < mp[i])
        throw ogu::Exception(__FILE__, __LINE__,
                             "Mapping not monotone in FoldMultiIndex constructor");
      if (mp[mi[i]] >= nv || mp[mi[i]] < 0)
        throw ogu::Exception(__FILE__, __LINE__,
                             "Mapping out of bounds in FoldMultiIndex constructor");
      data[i] = mp[mi[i]];
    }
}

FoldMultiIndex::FoldMultiIndex(const FoldMultiIndex &fmi, int new_orders)
  : nvar(fmi.nvar),
    ord(fmi.ord+new_orders),
    data(std::make_unique<int[]>(ord))
{
  std::copy_n(fmi.data.get(), fmi.ord, data.get());
  int new_item = (fmi.ord > 0) ? fmi.data[fmi.ord-1] : 0;
  for (int i = fmi.ord; i < ord; i++)
    data[i] = new_item;
}

FoldMultiIndex::FoldMultiIndex(const FoldMultiIndex &fmi)
  : nvar(fmi.nvar),
    ord(fmi.ord),
    data(std::make_unique<int[]>(ord))
{
  std::copy_n(fmi.data.get(), ord, data.get());
}

const FoldMultiIndex &
FoldMultiIndex::operator=(const FoldMultiIndex &fmi)
{
  if (ord != fmi.ord)
    data = std::make_unique<int[]>(fmi.ord);

  ord = fmi.ord;
  nvar = fmi.nvar;
  std::copy_n(fmi.data.get(), ord, data.get());

  return *this;
}

bool
FoldMultiIndex::operator<(const FoldMultiIndex &fmi) const
{
  if (nvar != fmi.nvar)
    ogu::Exception(__FILE__, __LINE__,
                   "Different nvar in FoldMultiIndex::operator<");

  if (ord < fmi.ord)
    return true;
  if (ord > fmi.ord)
    return false;

  int i = 0;
  while (i < ord && data[i] == fmi.data[i])
    i++;
  if (i == ord)
    return false;
  else
    return data[i] < fmi.data[i];
}

bool
FoldMultiIndex::operator==(const FoldMultiIndex &fmi) const
{
  bool res = true;
  res = res && (nvar == fmi.nvar) && (ord == fmi.ord);
  if (res)
    for (int i = 0; i < ord; i++)
      if (data[i] != fmi.data[i])
        return false;
  return res;
}

void
FoldMultiIndex::increment()
{
  if (ord == 0)
    return;

  int k = ord-1;
  data[k]++;
  while (k > 0 && data[k] == nvar)
    {
      data[k] = 0;
      data[--k]++;
    }
  for (int kk = 1; kk < ord; kk++)
    if (data[kk-1] > data[kk])
      data[kk] = data[kk-1];
}

// For description of an algorithm for calculation of folded offset,
// see Tensor Library Documentation, Ondra Kamenik, 2005, description
// of FTensor::getOffsetRecurse().
int
FoldMultiIndex::offset() const
{
  // make copy for the recursions
  auto tmp = std::make_unique<int[]>(ord);
  std::copy_n(data.get(), ord, tmp.get());
  // call the recursive algorithm
  int res = offset_recurse(tmp.get(), ord, nvar);
  return res;
}

void
FoldMultiIndex::print() const
{
  std::cout << "[";
  for (int i = 0; i < ord; i++)
    std::cout << data[i] << ' ';
  std::cout << "]";
}

int
FoldMultiIndex::offset_recurse(int *data, int len, int nv)
{
  if (len == 0)
    return 0;
  // calculate length of initial constant indices
  int prefix = 1;
  while (prefix < len && data[0] == data[prefix])
    prefix++;

  int m = data[0];
  int s1 = PascalTriangle::noverk(nv+len-1, len) - PascalTriangle::noverk(nv-m+len-1, len);

  // cancel m from the rest of the sequence
  for (int i = prefix; i < len; i++)
    data[i] -= m;

  // calculate offset of the remaining sequence
  int s2 = offset_recurse(data+prefix, len-prefix, nv-m);
  // return the sum
  return s1+s2;
}

bool
ltfmi::operator()(const FoldMultiIndex &i1, const FoldMultiIndex &i2) const
{
  return i1 < i2;
}

FormulaDerEvaluator::FormulaDerEvaluator(const FormulaParser &fp)
  : etree(fp.otree, -1)
{
  for (const auto &der : fp.ders)
    ders.push_back(der.get());

  der_atoms = fp.atoms.variables();
}

void
FormulaDerEvaluator::eval(const AtomValues &av, FormulaDerEvalLoader &loader, int order)
{
  if (ders.size() == 0)
    return;
  int maxorder = ders[0]->order;

  if (order > maxorder)
    throw ogu::Exception(__FILE__, __LINE__,
                         "Wrong order in FormulaDerEvaluator::eval");

  etree.reset_all();
  av.setValues(etree);

  auto vars = std::make_unique<int[]>(order);

  for (unsigned int i = 0; i < ders.size(); i++)
    {
      for (const auto &it : ders[i]->ind2der)
        {
          const FoldMultiIndex &mi = it.first;
          if (mi.order() == order)
            {
              // set vars from multiindex mi and variables
              for (int k = 0; k < order; k++)
                vars[k] = der_atoms[mi[k]];
              // evaluate
              double res = etree.eval(ders[i]->tder[it.second]);
              // load
              loader.load(i, order, vars.get(), res);
            }
        }
    }
}

void
FormulaDerEvaluator::eval(const vector<int> &mp, const AtomValues &av,
                          FormulaDerEvalLoader &loader, int order)
{
  etree.reset_all();
  av.setValues(etree);

  int nvar_glob = der_atoms.size();
  int nvar = mp.size();
  auto vars = std::make_unique<int[]>(order);

  for (unsigned int i = 0; i < ders.size(); i++)
    {
      FoldMultiIndex mi(nvar, order);
      do
        {
          // find index of the derivative in the tensor
          FoldMultiIndex mi_glob(nvar_glob, mi, mp);
          int der = ders[i]->derivative(mi_glob);
          if (der != OperationTree::zero)
            {
              // set vars from the global multiindex
              for (int k = 0; k < order; k++)
                vars[k] = der_atoms[mi_glob[k]];
              // evaluate derivative
              double res = etree.eval(der);
              // load
              loader.load(i, order, vars.get(), res);
            }
          mi.increment();
        }
      while (!mi.past_the_end());
    }
}
