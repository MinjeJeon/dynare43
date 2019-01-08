// Copyright (C) 2004-2011, Ondra Kamenik

// Matrix interface.

/* Here we make an interface to 2-dimensional matrix defined in the
   Sylvester module. That abstraction provides an interface to BLAS. The
   main purpose of this file is to only make its subclass in order to
   keep the tensor library and Sylvester module independent. So here is
   mainly renaming of methods.

   Similarly as in the Sylvester module we declare two classes
   |TwoDMatrix| and |ConstTwoDMatrix|. The only purpose of the latter is
   to allow submatrix construction from const reference arguments. */

#ifndef TWOD_MATRIX_H
#define TWOD_MATRIX_H

#include "GeneralMatrix.h"

#include <cstdio>
#include <matio.h>

class TwoDMatrix;

/* We make two obvious constructors, and then a constructor making
   submatrix of subsequent columns. We also rename
   |GeneralMatrix::numRows()| and |GeneralMatrix::numCols()|. */

class ConstTwoDMatrix : public ConstGeneralMatrix
{
public:
  ConstTwoDMatrix(int m, int n, const double *d)
    : ConstGeneralMatrix(d, m, n)
  {
  }
  ConstTwoDMatrix(const TwoDMatrix &m);
  ConstTwoDMatrix(const TwoDMatrix &m, int first_col, int num);
  ConstTwoDMatrix(const ConstTwoDMatrix &m, int first_col, int num);
  ConstTwoDMatrix(int first_row, int num, const TwoDMatrix &m);
  ConstTwoDMatrix(int first_row, int num, const ConstTwoDMatrix &m);
  ConstTwoDMatrix(const ConstTwoDMatrix &m, int first_row, int first_col, int rows, int cols)
    : ConstGeneralMatrix(m, first_row, first_col, rows, cols)
  {
  }
  virtual ~ConstTwoDMatrix()
  {
  }

  int
  nrows() const
  {
    return numRows();
  }
  int
  ncols() const
  {
    return numCols();
  }
  void writeMat(mat_t *fd, const char *vname) const;
};

/* Here we do the same as for |ConstTwoDMatrix| plus define
   methods for copying and adding rows and columns.

   Also we have |save| method which dumps the matrix to a file with a
   given name. The file can be read by Scilab {\tt fscanfMat} function. */

class TwoDMatrix : public GeneralMatrix
{
public:
  TwoDMatrix(int r, int c)
    : GeneralMatrix(r, c)
  {
  }
  TwoDMatrix(int r, int c, double *d)
    : GeneralMatrix(d, r, c)
  {
  }
  TwoDMatrix(int r, int c, const double *d)
    : GeneralMatrix(d, r, c)
  {
  }
  TwoDMatrix(const GeneralMatrix &m)
    : GeneralMatrix(m)
  {
  }
  TwoDMatrix(const GeneralMatrix &m, const char *dummy)
    : GeneralMatrix(m, dummy)
  {
  }
  TwoDMatrix(const TwoDMatrix &m, int first_col, int num)
    : GeneralMatrix(m, 0, first_col, m.numRows(), num)
  {
  }
  TwoDMatrix(TwoDMatrix &m, int first_col, int num)
    : GeneralMatrix(m, 0, first_col, m.numRows(), num)
  {
  }
  TwoDMatrix(int first_row, int num, const TwoDMatrix &m)
    : GeneralMatrix(m, first_row, 0, num, m.ncols())
  {
  }
  TwoDMatrix(int first_row, int num, TwoDMatrix &m)
    : GeneralMatrix(m, first_row, 0, num, m.ncols())
  {
  }
  TwoDMatrix(TwoDMatrix &m, int first_row, int first_col, int rows, int cols)
    : GeneralMatrix(m, first_row, first_col, rows, cols)
  {
  }
  TwoDMatrix(const TwoDMatrix &m, int first_row, int first_col, int rows, int cols)
    : GeneralMatrix(m, first_row, first_col, rows, cols)
  {
  }
  TwoDMatrix(const ConstTwoDMatrix &a, const ConstTwoDMatrix &b)
    : GeneralMatrix(a, b)
  {
  }
  virtual ~TwoDMatrix()
  {
  }

  int
  nrows() const
  {
    return numRows();
  }
  int
  ncols() const
  {
    return numCols();
  }

  // |TwoDMatrix| row methods declarations
  void copyRow(int from, int to);
  void copyRow(const ConstTwoDMatrix &m, int from, int to);
  void
  copyRow(const TwoDMatrix &m, int from, int to)
  {
    copyRow(ConstTwoDMatrix(m), from, to);
  }
  void
  addRow(const ConstTwoDMatrix &m, int from, int to)
  {
    addRow(1.0, m, from, to);
  }
  void
  addRow(const TwoDMatrix &m, int from, int to)
  {
    addRow(1.0, ConstTwoDMatrix(m), from, to);
  }
  void addRow(double d, const ConstTwoDMatrix &m, int from, int to);
  void
  addRow(double d, const TwoDMatrix &m, int from, int to)
  {
    addRow(d, ConstTwoDMatrix(m), from, to);
  }

  // |TwoDMatrix| column methods declarations
  void copyColumn(int from, int to);
  void copyColumn(const ConstTwoDMatrix &m, int from, int to);
  void
  copyColumn(const TwoDMatrix &m, int from, int to)
  {
    copyColumn(ConstTwoDMatrix(m), from, to);
  }
  void
  addColumn(const ConstTwoDMatrix &m, int from, int to)
  {
    addColumn(1.0, ConstTwoDMatrix(m), from, to);
  }
  void
  addColumn(const TwoDMatrix &m, int from, int to)
  {
    addColumn(1.0, ConstTwoDMatrix(m), from, to);
  }
  void addColumn(double d, const ConstTwoDMatrix &m, int from, int to);
  void
  addColumn(double d, const TwoDMatrix &m, int from, int to)
  {
    addColumn(d, ConstTwoDMatrix(m), from, to);
  }

  void save(const char *fname) const;
  void
  writeMat(mat_t *fd, const char *vname) const
  {
    ConstTwoDMatrix(*this).writeMat(fd, vname);
  }
};

#endif
