# Copyright © 2017-2019 Dynare Team
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
# along with Dynare.  If not, see <https://www.gnu.org/licenses/>.

.PHONY: all deps clean-deps clean clean-all install build

NTHREADS=$(shell nproc)

all: build

deps:
	make -C deps -j$(NTHREADS)

clean-deps:
	make -C deps clean-all

clean:
	rm -rf exe/ 7z/ zip/

clean-all: clean-deps clean

install:
	./install-packages.sh

build: deps
	./build.sh
