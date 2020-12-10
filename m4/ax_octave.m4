dnl ax_octave.m4 --- check for Octave
dnl
dnl Copyright © 2020 Dynare Team
dnl
dnl This program is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License as
dnl published by the Free Software Foundation; either version 2,
dnl or (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License along
dnl with this program. If not, see <http://www.gnu.org/licenses/>.
dnl
dnl As a special exception to the GNU General Public License, if
dnl you distribute this file as part of a program that contains a
dnl configuration script generated by GNU Autoconf, you may include
dnl it under the same distribution terms that you use for the rest
dnl of that program.

# AX_OCTAVE
# ---------
# Checks for Octave.
# Search in the argument given to --with-octave if any.
# Otherwise, search in PATH.
# Sets OCTAVE and MKOCTFILE variables and calls AC_SUBST on them.
# Sets ax_enable_octave=yes if successful, ax_enable_octave=no otherwise
AC_DEFUN([AX_OCTAVE],
[dnl
AC_ARG_WITH([octave], AC_HELP_STRING([--with-octave=PATH], [Path to search for Octave installation]),
[
  if test -n "$withval"; then
    if test -x "$withval/bin/octave" && test -x "$withval/bin/mkoctfile"; then
      OCTAVE="$withval/bin/octave"
      MKOCTFILE="$withval/bin/mkoctfile"
    else
      AC_MSG_ERROR([invalid value '$withval' for --with-octave])
    fi
  fi
])

AC_CHECK_PROG([OCTAVE], [octave], [octave])
AC_CHECK_PROG([MKOCTFILE], [mkoctfile], [mkoctfile])

if test -n "$OCTAVE" && test -n "$MKOCTFILE"; then
    ax_enable_octave=yes
else
    ax_enable_octave=no
fi

AC_SUBST([OCTAVE])
AC_SUBST([MKOCTFILE])
])