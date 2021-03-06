# Copyright © 2019-2021 Dynare Team
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

include versions.mk

ROOT_PATH = $(realpath .)

WGET_OPTIONS := --no-verbose --no-use-server-timestamps --retry-connrefused --retry-on-host-error

.PHONY: all build build-slicot build-x13as \
	clean-lib clean-libslicot clean-x13as-bin \
	clean-src clean-slicot-src clean-x13as-src \
	clean-tar clean-slicot-tar clean-x13as-tar \
	clean-all clean-lib clean-src clean-tar

all: build

build: build-slicot build-x13as ln-matio ln-gsl

clean-lib: clean-libslicot clean-x13as-bin

clean-src: clean-slicot-src clean-x13as-src

clean-tar: clean-slicot-tar clean-x13as-tar

clean-all: clean-lib clean-src clean-tar


#
# Matio & GSL
# (done to link only to static Matio and GSL libraries)

# Matio
lib64/matio/lib/libmatio.a: /usr/local/lib/libmatio.a
	mkdir -p $(dir $@) && ln -sf $< $@

lib64/matio/lib/libhdf5.a: /usr/local/lib/libhdf5.a
	mkdir -p $(dir $@) && ln -sf $< $@

lib64/matio/lib/libsz.a: /usr/local/lib/libsz.a
	mkdir -p $(dir $@) && ln -sf $< $@

lib64/matio/include/matio.h: /usr/local/include/matio.h
	mkdir -p $(dir $@) && cd $(dir $@).. && rm -rf include && ln -sf $(dir $<) .

ln-matio: lib64/matio/lib/libmatio.a \
	lib64/matio/lib/libhdf5.a \
	lib64/matio/lib/libsz.a \
	lib64/matio/include/matio.h

clean-matio:
	rm -rf lib64/matio

# GSL
lib64/gsl/lib/libgsl.a: /usr/local/lib/libgsl.a
	mkdir -p $(dir $@) && ln -sf $< $@

lib64/gsl/lib/libgslcblas.a: /usr/local/lib/libgslcblas.a
	mkdir -p $(dir $@) && ln -sf $< $@

lib64/gsl/include/gsl/gsl_blas.h: /usr/local/include/gsl/gsl_blas.h
	mkdir -p $(dir $@) && cd $(dir $@).. && rm -rf gsl && ln -sf $(dir $<) .

ln-gsl: lib64/gsl/lib/libgsl.a \
	lib64/gsl/lib/libgslcblas.a \
	lib64/gsl/include/gsl/gsl_blas.h

clean-gsl:
	rm -rf lib64/gsl

#
# Slicot
#
tarballs/slicot-$(SLICOT_VERSION).tar.gz:
	mkdir -p tarballs
	wget $(WGET_OPTIONS) -O $@ https://deb.debian.org/debian/pool/main/s/slicot/slicot_$(SLICOT_VERSION).orig.tar.gz

sources64/slicot-$(SLICOT_VERSION)-with-32bit-integer-and-underscore: tarballs/slicot-$(SLICOT_VERSION).tar.gz
	rm -rf sources64/slicot-*-with-32bit-integer-and-underscore
	mkdir -p $@
	tar xf $< --directory $@ --strip-components=1
	touch $@

sources64/slicot-$(SLICOT_VERSION)-with-64bit-integer-and-underscore: tarballs/slicot-$(SLICOT_VERSION).tar.gz
	rm -rf sources64/slicot-*-with-64bit-integer-and-underscore
	mkdir -p $@
	tar xf $< --directory $@ --strip-components=1
	touch $@

lib64/Slicot/with-underscore/lib/libslicot_pic.a: sources64/slicot-$(SLICOT_VERSION)-with-32bit-integer-and-underscore
	make -C $< FORTRAN=gfortran LOADER=gfortran SLICOTLIB=../libslicot_pic.a OPTS="-O2 -g" lib
	strip -S $</libslicot_pic.a
	mkdir -p $(dir $@)
	cp $</libslicot_pic.a $@

lib64/Slicot/with-underscore/lib/libslicot64_pic.a: sources64/slicot-$(SLICOT_VERSION)-with-64bit-integer-and-underscore
	make -C $< FORTRAN=gfortran LOADER=gfortran SLICOTLIB=../libslicot64_pic.a OPTS="-O2 -g -fdefault-integer-8" lib
	strip -S $</libslicot64_pic.a
	mkdir -p $(dir $@)
	cp $</libslicot64_pic.a $@

build-slicot: lib64/Slicot/with-underscore/lib/libslicot_pic.a \
	lib64/Slicot/with-underscore/lib/libslicot64_pic.a

clean-slicot-tar:
	rm -f tarballs/slicot-$(SLICOT_VERSION).tar.gz

clean-slicot-src:
	rm -rf sources64/slicot-$(SLICOT_VERSION)-with-64bit-integer-and-underscore

clean-libslicot:
	rm -rf lib64/Slicot

clean-slicot-all: clean-slicot-src clean-slicot-tar clean-libslicot



#
# X13AS
#
tarballs/x13assrc_V$(X13AS_VERSION).tar.gz:
	mkdir -p tarballs
	wget $(WGET_OPTIONS) -O $@ https://www.census.gov/ts/x13as/unix/x13assrc_V$(X13AS_VERSION).tar.gz

sources64/x13as-$(X13AS_VERSION): tarballs/x13assrc_V$(X13AS_VERSION).tar.gz
	rm -rf sources64/x13as-*
	mkdir -p $@
	tar xf $< --directory $@

lib64/x13as/x13as: sources64/x13as-$(X13AS_VERSION)
	cd $< && sed -i '' 's/-static//g' makefile.gf
	make -C $< -f makefile.gf FC=gfortran LINKER=gfortran FFLAGS="-O2 -std=legacy" PROGRAM=x13as
	strip $</x13as
	mkdir -p $(dir $@)
	cp $</x13as $@

build-x13as: lib64/x13as/x13as

clean-x13as-tar:
	rm -f tarballs/x13assrc_V$(X13AS_VERSION).tar.gz

clean-x13as-src:
	rm -rf sources64/x13as-$(X13AS_VERSION)

clean-x13as-bin:
	rm -rf lib64/x13as

clean-x13as-all: clean-x13as-tar clean-x13as-src clean-x13as-bin
