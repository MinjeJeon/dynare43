// Copyright © 2006, Ondra Kamenik

// $Id: static_atoms.h 1218 2007-03-19 21:52:49Z kamenik $

#ifndef OGP_STATIC_ATOMS
#define OGP_STATIC_ATOMS

#include "dynamic_atoms.hh"

namespace ogp
{
  class StaticAtoms : public Atoms, public Constants
  {
  protected:
    using Tvarmap = map<string, int>;
    using Tinvmap = map<int, string>;
    /** Storage for names. */
    NameStorage varnames;
    /** Outer order of variables. */
    vector<string> varorder;
    /** This is the map mapping a variable name to the tree
     * index. */
    Tvarmap vars;
    /** This is the inverse mapping. It maps a tree index to the
     * variable name. */
    Tinvmap indices;
  public:
    StaticAtoms() = default;
    /** Conversion from DynamicAtoms. This takes all atoms from
     * the DynamicAtoms and adds its static version. The new tree
     * indices are allocated in the passed OperationTree. Whole
     * the process is traced in the map mapping old tree indices
     * to new tree indices. */
    StaticAtoms(const DynamicAtoms &da, OperationTree &otree, Tintintmap &tmap)
      : Atoms(), Constants(), varnames(), varorder(), vars()
    {
      import_atoms(da, otree, tmap);
    }
    /* Destructor. */
    ~StaticAtoms() override = default;
    /** This imports atoms from dynamic atoms inserting the new
     * tree indices to the given tree (including constants). The
     * mapping from old atoms to new atoms is traced in tmap. */
    void import_atoms(const DynamicAtoms &da, OperationTree &otree,
                      Tintintmap &tmap);
    /** If the name is constant, it returns its tree index if the
     * constant is registered in Constants, it returns -1
     * otherwise. If the name is not constant, it returns result
     * from check_variable, which is implemented by a subclass. */
    int check(const string &name) const override;
    /** This assigns a given tree index to the variable name. The
     * name should have been checked before the call. */
    void assign(const string &name, int t) override;
    int
    nvar() const override
    {
      return varnames.num();
    }
    /** This returns a vector of all variables. */
    vector<int> variables() const override;
    /** This returns a tree index of the given variable. */
    int index(const string &name) const;
    /** This returns a name in a outer ordering. (There is no other ordering.) */
    const string &
    name(int i) const
    {
      return varorder[i];
    }
    /** Debug print. */
    void print() const override;
    /** This registers a variable. A subclass can reimplement
     * this, for example, to ensure uniqueness of the
     * name. However, this method should be always called in
     * overriding methods to do the registering job. */
    virtual void register_name(string name);
    /** Return the name storage to allow querying to other
     * classes. */
    const NameStorage &
    get_name_storage() const
    {
      return varnames;
    }
  protected:
    /** This checks the variable. The implementing subclass might
     * want to throw an exception if the variable has not been
     * registered. */
    virtual int check_variable(const string &name) const = 0;
  };
};

#endif
