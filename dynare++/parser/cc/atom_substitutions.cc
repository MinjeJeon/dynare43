// Copyright © 2006, Ondra Kamenik

// $Id: atom_substitutions.cpp 42 2007-01-22 21:53:24Z ondra $

#include "atom_substitutions.hh"
#include "utils/cc/exception.hh"

using namespace ogp;

AtomSubstitutions::AtomSubstitutions(const AtomSubstitutions &as, const FineAtoms &oa,
                                     FineAtoms &na)
  : old_atoms(oa), new_atoms(na)
{
  const NameStorage &ns = na.get_name_storage();

  // fill new2old
  for (const auto & it : as.new2old)
    new2old.emplace(ns.query(it.first),
                    Tshiftname(ns.query(it.second.first),
                               it.second.second));
  // fill old2new
  for (const auto & it : as.old2new)
    {
      Tshiftnameset sset;
      for (const auto & itt : it.second)
        sset.emplace(ns.query(itt.first), itt.second);
      old2new.emplace(ns.query(it.first), sset);
    }
}

void
AtomSubstitutions::add_substitution(const char *newname, const char *oldname, int tshift)
{
  // make sure the storage is from the new_atoms
  newname = new_atoms.get_name_storage().query(newname);
  oldname = new_atoms.get_name_storage().query(oldname);
  if (!newname || !oldname)
    throw ogu::Exception(__FILE__, __LINE__,
                         "Bad newname or oldname in AtomSubstitutions::add_substitution");

  // insert to new2old map
  new2old.emplace(newname, Tshiftname(oldname, tshift));
  // insert to old2new map
  auto it = old2new.find(oldname);
  if (it != old2new.end())
    it->second.emplace(newname, -tshift);
  else
    {
      Tshiftnameset snset;
      snset.emplace(newname, -tshift);
      old2new.emplace(oldname, snset);
    }

  // put to info
  info.num_substs++;
}

void
AtomSubstitutions::substitutions_finished(VarOrdering::ord_type ot)
{
  // create an external ordering of new_atoms from old_atoms
  const vector<const char *> &oa_ext = old_atoms.get_allvar();
  vector<const char *> na_ext;
  for (auto oname : oa_ext)
    {
      // add the old name itself
      na_ext.push_back(oname);
      // add all new names derived from the old name
      Toldnamemap::const_iterator it = old2new.find(oname);
      if (it != old2new.end())
        for (const auto & itt : it->second)
          na_ext.push_back(itt.first);
    }

  // call parsing finished for the new_atoms
  new_atoms.parsing_finished(ot, na_ext);
}

const char *
AtomSubstitutions::get_new4old(const char *oldname, int tshift) const
{
  auto it = old2new.find(oldname);
  if (it != old2new.end())
    {
      const Tshiftnameset &sset = it->second;
      for (const auto & itt : sset)
        if (itt.second == -tshift)
          return itt.first;
    }
  return nullptr;
}

void
AtomSubstitutions::print() const
{
  std::cout << u8"Atom Substitutions:\nOld ⇒ New:\n";
  for (const auto & it : old2new)
    for (const auto &itt : it.second)
      std::cout << "    " << it.first << u8" ⇒ [" << itt.first << ", " << itt.second << "]\n";

  std::cout << u8"Old ⇐ New:\n";
  for (const auto & it : new2old)
    std::cout << "    [" << it.second.first << ", " << it.second.second << "] ⇐ " << it.first << '\n';
}

void
SAtoms::substituteAllLagsAndLeads(FormulaParser &fp, AtomSubstitutions &as)
{
  const char *name;

  int mlead, mlag;
  endovarspan(mlead, mlag);

  // substitute all endo lagged more than 1
  while (name = findEndoWithLeadInInterval(mlag, -2))
    makeAuxVariables(name, -1, -2, mlag, fp, as);
  // substitute all endo leaded more than 1
  while (name = findEndoWithLeadInInterval(2, mlead))
    makeAuxVariables(name, 1, 2, mlead, fp, as);

  exovarspan(mlead, mlag);

  // substitute all lagged exo
  while (name = findExoWithLeadInInterval(mlag, -1))
    makeAuxVariables(name, -1, -1, mlag, fp, as);
  // substitute all leaded exo
  while (name = findExoWithLeadInInterval(1, mlead))
    makeAuxVariables(name, 1, 1, mlead, fp, as);

  // notify that substitution have been finished
  as.substitutions_finished(order_type);
}

void
SAtoms::substituteAllLagsAndExo1Leads(FormulaParser &fp, AtomSubstitutions &as)
{
  const char *name;

  int mlead, mlag;
  endovarspan(mlead, mlag);

  // substitute all endo lagged more than 1
  while (name = findEndoWithLeadInInterval(mlag, -2))
    makeAuxVariables(name, -1, -2, mlag, fp, as);

  exovarspan(mlead, mlag);

  // substitute all lagged exo
  while (name = findExoWithLeadInInterval(mlag, -1))
    makeAuxVariables(name, -1, -1, mlag, fp, as);
  // substitute all leaded exo by 1
  while (name = findExoWithLeadInInterval(1, 1))
    makeAuxVariables(name, 1, 1, 1, fp, as);

  // notify that substitution have been finished
  as.substitutions_finished(order_type);
}

const char *
SAtoms::findNameWithLeadInInterval(const vector<const char *> &names,
                                   int ll1, int ll2) const
{
  for (auto name : names)
    {
      auto it = vars.find(name);
      if (it != vars.end())
        {
          const DynamicAtoms::Tlagmap &lmap = (*it).second;
          for (auto itt : lmap)
            if (itt.first >= ll1 && itt.first <= ll2)
              return name;
        }
    }

  // nothing found
  return nullptr;
}

void
SAtoms::attemptAuxName(const char *str, int ll, string &out) const
{
  char c = (ll >= 0) ? ((ll == 0) ? 'e' : 'p') : 'm';
  string absll = std::to_string(std::abs(ll));
  int iter = 1;
  do
    {
      out = string(str) + '_';
      for (int i = 0; i < iter; i++)
        out += c;
      if (ll != 0)
        out += absll;
      iter++;
    }
  while (varnames.query(out.c_str()));
}

void
SAtoms::makeAuxVariables(const char *name, int step, int start, int limit_lead,
                         FormulaParser &fp, AtomSubstitutions &as)
{
  if (!(step == 1 || step == -1))
    throw ogu::Exception(__FILE__, __LINE__,
                         "Wrong value of step in SAtoms::makeAuxVariables");
  if (step*start > step*limit_lead)
    throw ogu::Exception(__FILE__, __LINE__,
                         "Wrong value of start in SAtoms::makeAuxVariables");

  // make sure that we do not go further than necessary, this is
  // that the limit lead is not behind maxlead or minlag
  int mlead, mlag;
  varspan(name, mlead, mlag);
  if (step == -1)
    limit_lead = std::max(limit_lead, mlag);
  else
    limit_lead = std::min(limit_lead, mlead);

  // Comment to comments: name="a"; start=-3; step=-1;

  // recover tree index of a previous atom, i.e. set tprev to a tree
  // index of atom "a(-2)"
  int tprev = index(name, start-step);
  if (tprev == -1)
    {
      string tmp = string{name} + '(' + std::to_string(start-step) + ')';
      tprev = fp.add_nulary(tmp.c_str());
    }

  int ll = start;
  do
    {
      // either create atom "a_m2(0)" with tree index taux and add
      // equation "a_m2(0)=a(-2)"
      // or
      // check if "a_m2(0)" has not been already created (with
      // different step), in this case do not add equation "a_m2(0)
      // = a(-2)"
      const char *newname;
      string newname_str;
      int taux;
      if (!(newname = as.get_new4old(name, ll-step)))
        {
          attemptAuxName(name, ll-step, newname_str);
          newname = newname_str.c_str();
          register_uniq_endo(newname);
          newname = varnames.query(newname);
          string tmp = string{newname} + "(0)";
          taux = fp.add_nulary(tmp.c_str());
          // add to substitutions
          as.add_substitution(newname, name, ll-step);

          // add equation "a_m2(0) = a(-2)", this is taux = tprev
          fp.add_formula(fp.add_binary(code_t::MINUS, taux, tprev));
        }
      else
        {
          // example: exogenous EPS and occurrence at both EPS(-1)
          // EPS(+1)
          // first call makeAuxVariables("EPS",1,1,...) will make endo EPS_p0 = EPS
          // second call makeAuxVariables("EPS",-1,-1,...) will use this EPS_p0
          //             to substitute for EPS(-1)
          taux = index(newname, 0);
          if (taux < 0)
            throw ogu::Exception(__FILE__, __LINE__,
                                 "Couldn't find tree index of previously substituted variable");
        }

      // create atom "a_m2(-1)" or turn "a(-3)" if any to "a_m2(-1)"; tree index t
      int t = index(name, ll);
      if (t == -1)
        {
          // no "a(-3)", make t <-> a_m2(-1)
          string tmp = string{newname} + '(' + std::to_string(step) + ')';
          t = fp.add_nulary(tmp.c_str());
        }
      else
        {
          // turn a(-3) to a_m2(-1)
          unassign_variable(name, ll, t);
          assign_variable(newname, step, t);
        }

      // next iteration starts with tprev <-> "a_m2(-1)" (this will be made equal to "a_m3(0)")
      tprev = t;

      ll += step;
    }
  while (step*ll <= step*limit_lead);
}
