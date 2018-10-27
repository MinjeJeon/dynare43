# Copyright (C) 2018 Dynare Team
#
# This file is part of Dynare.
#
# Dynare is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Dynare is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Dynare.  If not, see <http://www.gnu.org/licenses/>.

all: html pdf

html: src/build/html/index.html

src/build/html/index.html: $(src/source/%.rst) src/source/conf.py
	source python/bin/activate ; make -C src html

pdf: src/build/latex/dynare.pdf

src/build/latex/dynare.pdf: $(src/source/%.rst) src/source/conf.py
	source python/bin/activate ; make -C src latexpdf

python: python/bin/python3

python/bin/python3:
	python3 -m venv python
	source python/bin/activate ; pip install --upgrade pip ; pip install sphinx recommonmark sphinx_rtd_theme
	cp py/pygment/dynare.py python/lib/python3.*/site-packages/pygments/lexers/
	cd python/lib/python3.*/site-packages/pygments/lexers ; python3 _mapping.py
	patch -i py/basic.css_t.patch python/lib/python3.*/site-packages/sphinx/themes/basic/static/basic.css_t
