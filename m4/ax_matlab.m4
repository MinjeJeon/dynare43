dnl ax_matlab.m4 --- check for MATLAB.
dnl
dnl Copyright © 2000-2003 Ralph Schleicher
dnl Copyright © 2009 Dynare Team
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
dnl with this program. If not, see <https://www.gnu.org/licenses/>.
dnl
dnl As a special exception to the GNU General Public License, if
dnl you distribute this file as part of a program that contains a
dnl configuration script generated by GNU Autoconf, you may include
dnl it under the same distribution terms that you use for the rest
dnl of that program.
dnl
dnl Code:

# AX_MATLAB
# ---------
# Check for MATLAB.
AC_DEFUN([AX_MATLAB],
[dnl
AC_PREREQ([2.50])
ax_enable_matlab=
AC_ARG_WITH([matlab], AC_HELP_STRING([--with-matlab=ARG], [check for MATLAB [[yes]]]),
[case $withval in
  yes | no)
    # Explicitly enable or disable MATLAB but determine
    # MATLAB prefix automatically.
    ax_enable_matlab=$withval
    ;;
  *)
    # Enable MATLAB and use ARG as the MATLAB prefix.
    # ARG must be an existing directory.
    ax_enable_matlab=yes
    MATLAB=$(cd "${withval-/}" > /dev/null 2>&1 && pwd)
    if test -z "$MATLAB" ; then
	AC_MSG_ERROR([invalid value '$withval' for --with-matlab])
    fi
    ;;
esac])
AC_CACHE_CHECK([for MATLAB prefix], [ax_cv_matlab],
[if test "${MATLAB+set}" = set ; then
    ax_cv_matlab=$(cd "${MATLAB-/}" > /dev/null 2>&1 && pwd)
else
    ax_cv_matlab=
    IFS=${IFS= 	} ; ax_ifs=$IFS ; IFS=:
    for ax_dir in ${PATH-/opt/bin:/usr/local/bin:/usr/bin:/bin} ; do
	if test -z "$ax_dir" ; then
	    ax_dir=.
	fi
	if test -x "$ax_dir/matlab" ; then
	    ax_dir=$(echo "$ax_dir" | sed 's,/bin$,,')
	    # Directory sanity check.
	    ax_cv_matlab=$(cd "${ax_dir-/}" > /dev/null 2>&1 && pwd)
	    if test -n "$ax_cv_matlab" ; then
		break
	    fi
	fi
    done
    IFS=$ax_ifs
fi
if test -z "$ax_cv_matlab" ; then
    ax_cv_matlab="not found"
fi])
if test "$ax_cv_matlab" = "not found" ; then
    unset MATLAB
else
    # Strip trailing dashes.
    MATLAB=$(echo "$ax_cv_matlab" | sed 's,/*$,,')
fi
AC_MSG_CHECKING([whether to enable MATLAB support])
if test "$ax_enable_matlab" != no; then
    if test "${MATLAB+set}" = set && test -d "$MATLAB/extern/include" ; then
	ax_enable_matlab=yes
    elif test -z "$ax_enable_matlab"; then
	ax_enable_matlab=no
    else
	# Fail if MATLAB was explicitly enabled.
	AC_MSG_RESULT([failure])
	AC_MSG_ERROR([check your MATLAB setup])
    fi
fi
AC_MSG_RESULT([$ax_enable_matlab])
if test "$ax_enable_matlab" = yes; then
    AC_DEFINE([HAVE_MATLAB], [1], [Define if you have MATLAB.])
fi
AC_SUBST([MATLAB])
])

# AX_REQUIRE_MATLAB
# -----------------
# Like AX_MATLAB but fail if MATLAB support is disabled.
AC_DEFUN([AX_REQUIRE_MATLAB],
[dnl
AC_PREREQ([2.50])
AC_REQUIRE([AX_MATLAB])
if test "$ax_enable_matlab" = no; then
    AC_MSG_ERROR([can not enable MATLAB support])
fi
])

# AX_MATLAB_CONDITIONAL
# ---------------------
# Define MATLAB conditional for GNU Automake.
AC_DEFUN([AX_MATLAB_CONDITIONAL],
[dnl
AC_PREREQ([2.50])
AC_REQUIRE([AX_MATLAB])
AM_CONDITIONAL([MATLAB], [test "$ax_enable_matlab" = yes])
])

dnl ax_matlab.m4 ends here

dnl Local variables:
dnl tab-width: 8
dnl End:
