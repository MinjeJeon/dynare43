// Copyright © 2006, Ondra Kamenik

// $Id: dynare_atoms.cpp 1765 2008-03-31 14:32:08Z kamenik $

#include "parser/cc/parser_exception.hh"
#include "utils/cc/exception.hh"

#include "dynare_atoms.hh"

#include <string>
#include <cmath>

using namespace ogdyn;
using std::string;

void
DynareStaticAtoms::register_name(const char *name)
{
  if (varnames.query(name))
    throw ogp::ParserException(string("The name ")+name+" is not unique.", 0);
  StaticAtoms::register_name(name);
}

int
DynareStaticAtoms::check_variable(const char *name) const
{
  if (nullptr == varnames.query(name))
    throw ogp::ParserException(std::string("Unknown name <")+name+">", 0);
  auto it = vars.find(name);
  if (it == vars.end())
    return -1;
  else
    return it->second;
}

DynareDynamicAtoms::DynareDynamicAtoms(const DynareDynamicAtoms &dda)
  : SAtoms(dda)
{
  // fill atom_type
  for (auto it : dda.atom_type)
    atom_type.emplace(varnames.query(it.first), it.second);
}

void
DynareDynamicAtoms::parse_variable(const char *in, std::string &out, int &ll) const
{
  ll = 0;
  std::string str = in;
  auto left = str.find_first_of("({");
  if (left != string::npos)
    {
      out = str.substr(0, left);
      left++;
      auto right = str.find_first_of(")}", left);
      if (string::npos == right)
        throw ogp::ParserException(string("Syntax error when parsing Dynare atom <")+in+">.", 0);
      ll = std::stoi(str.substr(left, right-left));
    }
  else
    out = in;
}

void
DynareDynamicAtoms::register_uniq_endo(const char *name)
{
  FineAtoms::register_uniq_endo(name);
  atom_type.emplace(varnames.query(name), atype::endovar);
}

void
DynareDynamicAtoms::register_uniq_exo(const char *name)
{
  FineAtoms::register_uniq_exo(name);
  atom_type.emplace(varnames.query(name), atype::exovar);
}

void
DynareDynamicAtoms::register_uniq_param(const char *name)
{
  FineAtoms::register_uniq_param(name);
  atom_type.emplace(varnames.query(name), atype::param);
}

bool
DynareDynamicAtoms::is_type(const char *name, atype tp) const
{
  auto it = atom_type.find(name);
  if (it != atom_type.end() && it->second == tp)
    return true;
  else
    return false;
}

void
DynareDynamicAtoms::print() const
{
  SAtoms::print();
  printf("Name types:\n");
  for (auto it : atom_type)
    printf("name=%s type=%s\n", it.first,
           it.second == atype::endovar ? "endovar" : it.second == atype::exovar ? "exovar" : "param");
}

std::string
DynareDynamicAtoms::convert(int t) const
{
  if (t < ogp::OperationTree::num_constants)
    {
      throw ogu::Exception(__FILE__, __LINE__,
                           "Tree index is a built-in constant in DynareDynamicAtoms::convert");
      return {};
    }
  if (is_constant(t))
    {
      double v = get_constant_value(t);
      char buf[100];
      sprintf(buf, "%20.16g", v);
      const char *s = buf;
      while (*s == ' ')
        ++s;
      return s;
    }

  const char *s = name(t);
  if (is_type(s, atype::endovar))
    {
      int ll = lead(t);
      if (ll)
        return std::string{s} + '(' + std::to_string(ll) + ')';
      else
        return s;
    }

  return s;
}

void
DynareAtomValues::setValues(ogp::EvalTree &et) const
{
  // set constants
  atoms.setValues(et);

  // set parameteres
  for (unsigned int i = 0; i < atoms.get_params().size(); i++)
    if (atoms.is_referenced(atoms.get_params()[i]))
      {
        const ogp::DynamicAtoms::Tlagmap &lmap = atoms.lagmap(atoms.get_params()[i]);
        for (auto it : lmap)
          {
            int t = it.second;
            et.set_nulary(t, paramvals[i]);
          }
      }

  // set endogenous
  for (unsigned int outer_i = 0; outer_i < atoms.get_endovars().size(); outer_i++)
    if (atoms.is_referenced(atoms.get_endovars()[outer_i]))
      {
        const ogp::DynamicAtoms::Tlagmap &lmap = atoms.lagmap(atoms.get_endovars()[outer_i]);
        for (auto it : lmap)
          {
            int ll = it.first;
            int t = it.second;
            int i = atoms.outer2y_endo()[outer_i];
            if (ll == -1)
              et.set_nulary(t, yym[i-atoms.nstat()]);
            else if (ll == 0)
              et.set_nulary(t, yy[i]);
            else
              et.set_nulary(t, yyp[i-atoms.nstat()-atoms.npred()]);
          }
      }

  // set exogenous
  for (unsigned int outer_i = 0; outer_i < atoms.get_exovars().size(); outer_i++)
    if (atoms.is_referenced(atoms.get_exovars()[outer_i]))
      {
        const ogp::DynamicAtoms::Tlagmap &lmap = atoms.lagmap(atoms.get_exovars()[outer_i]);
        for (auto it : lmap)
          {
            int ll = it.first;
            if (ll == 0)   // this is always true because of checks
              {
                int t = it.second;
                int i = atoms.outer2y_exo()[outer_i];
                et.set_nulary(t, xx[i]);
              }
          }
      }
}

void
DynareStaticSteadyAtomValues::setValues(ogp::EvalTree &et) const
{
  // set constants
  atoms_static.setValues(et);

  // set parameters
  for (auto name : atoms_static.get_params())
    {
      int t = atoms_static.index(name);
      if (t != -1)
        {
          int idyn = atoms.name2outer_param(name);
          et.set_nulary(t, paramvals[idyn]);
        }
    }

  // set endogenous
  for (auto name : atoms_static.get_endovars())
    {
      int t = atoms_static.index(name);
      if (t != -1)
        {
          int idyn = atoms.outer2y_endo()[atoms.name2outer_endo(name)];
          et.set_nulary(t, yy[idyn]);
        }
    }

  // set exogenous
  for (auto name : atoms_static.get_exovars())
    {
      int t = atoms_static.index(name);
      if (t != -1)
        et.set_nulary(t, 0.0);
    }
}

DynareSteadySubstitutions::DynareSteadySubstitutions(const ogp::FineAtoms &a,
                                                     const ogp::OperationTree &tree,
                                                     const Tsubstmap &subst,
                                                     const Vector &pvals, Vector &yy)
  : atoms(a), y(yy)
{
  // fill the vector of left and right hand sides
  for (auto it : subst)
    {
      left_hand_sides.push_back(it.first);
      right_hand_sides.push_back(it.second);
    }

  // evaluate right hand sides
  DynareSteadyAtomValues dsav(atoms, pvals, y);
  ogp::FormulaCustomEvaluator fe(tree, right_hand_sides);
  fe.eval(dsav, *this);
}

void
DynareSteadySubstitutions::load(int i, double res)
{
  const char *name = left_hand_sides[i];
  int iouter = atoms.name2outer_endo(name);
  int iy = atoms.outer2y_endo()[iouter];
  if (!std::isfinite(y[iy]))
    y[iy] = res;
}

DynareStaticSteadySubstitutions::
DynareStaticSteadySubstitutions(const ogp::FineAtoms &a, const ogp::StaticFineAtoms &sa,
                                const ogp::OperationTree &tree,
                                const Tsubstmap &subst,
                                const Vector &pvals, Vector &yy)
  : atoms(a), atoms_static(sa), y(yy)
{
  // fill the vector of left and right hand sides
  for (auto it : subst)
    {
      left_hand_sides.push_back(it.first);
      right_hand_sides.push_back(it.second);
    }

  // evaluate right hand sides
  DynareStaticSteadyAtomValues dsav(atoms, atoms_static, pvals, y);
  ogp::FormulaCustomEvaluator fe(tree, right_hand_sides);
  fe.eval(dsav, *this);
}

void
DynareStaticSteadySubstitutions::load(int i, double res)
{
  const char *name = left_hand_sides[i];
  int iouter = atoms.name2outer_endo(name);
  int iy = atoms.outer2y_endo()[iouter];
  if (!std::isfinite(y[iy]))
    y[iy] = res;
}
