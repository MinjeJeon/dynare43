# ===========================================================================
#          http://www.nongnu.org/autoconf-archive/ax_latex_test.html
# ===========================================================================
#
# OBSOLETE MACRO
#
#   Deprecated because of licensing issues. The Lesser GPL imposes licensing
#   restrictions on the generated configure script unless it is augmented
#   with an Autoconf Exception clause.
#
# SYNOPSIS
#
#   AX_TEX_TEST(FILEDATA,VARIABLETOSET,[NOCLEAN])
#
# DESCRIPTION
#
#   This macros execute the pdftex application with FILEDATA as input and set
#   VARIABLETOSET to yes or no depending on the result. If NOCLEAN is set,
#   the folder used for the test is not delete after testing.
#
#   The macro assumes that the variable PDFTEX is set.
#
#   Adapted from the macro AX_LATEX_TEST by Sébastien Villemot.
#
# LICENSE
#
#   Copyright © 2008 Boretti Mathieu <boretti@eig.unige.ch>
#   Copyright © 2009 Dynare Team
#
#   This library is free software; you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published by
#   the Free Software Foundation; either version 2.1 of the License, or (at
#   your option) any later version.
#
#   This library is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
#   General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with this library. If not, see <http://www.gnu.org/licenses/>.

AC_DEFUN([AX_TEX_TEST],[
rm -rf conftest.dir/.acltx
AS_MKDIR_P([conftest.dir/.acltx])
cd conftest.dir/.acltx
m4_ifval([$2],[$2="no"; export $2;])
cat > conftest.tex << ACLEOF
$1
ACLEOF
cat conftest.tex | $PDFTEX 2>&1 1>output m4_ifval([$2],[&& $2=yes])
cd ..
cd ..
sed 's/^/| /' conftest.dir/.acltx/conftest.tex >&5
echo "$as_me:$LINENO: executing cat conftest.tex | $PDFTEX" >&5
sed 's/^/| /' conftest.dir/.acltx/output >&5
m4_ifval([$3],,[rm -rf conftest.dir/.acltx])
])
