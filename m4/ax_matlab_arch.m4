dnl matlabarch.m4 --- check for MATLAB machine architecture.
dnl
dnl Copyright © 2002, 2003 Ralph Schleicher
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

# AX_MATLAB_ARCH
# --------------
# Check for MATLAB machine architecture.
AC_DEFUN([AX_MATLAB_ARCH],
[dnl
AC_PREREQ([2.50])
AC_REQUIRE([AX_MEXEXT])
AC_CACHE_CHECK([for MATLAB machine architecture], [ax_cv_matlab_arch],
[if test "${MATLAB_ARCH+set}" = set ; then
    ax_cv_matlab_arch="$MATLAB_ARCH"
else
    case $MEXEXT in
      *dll | *mexw32)
	ax_cv_matlab_arch=win32
	;;
      *mexw64)
	ax_cv_matlab_arch=win64
	;;
      *mexglx)
	ax_cv_matlab_arch=glnx86
	;;
      *mexa64)
	ax_cv_matlab_arch=glnxa64
	;;
      *mexs64)
	ax_cv_matlab_arch=sol64
	;;
      *mexmaci)
	ax_cv_matlab_arch=maci
	;;
      *mexmaci64)
	ax_cv_matlab_arch=maci64
	;;
      *)
	ax_cv_matlab_arch=unknown
	;;
    esac
fi])
MATLAB_ARCH="$ax_cv_matlab_arch"
AC_SUBST([MATLAB_ARCH])
])

dnl matlabarch.m4 ends here

dnl Local variables:
dnl tab-width: 8
dnl End:
