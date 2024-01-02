#  Copyright 1999-2017 ImageMagick Studio LLC, a non-profit organization
#  dedicated to making software imaging solutions freely available.
#
#  You may not use this file except in compliance with the License.  You may
#  obtain a copy of the License at
#
#    http://www.imagemagick.org/script/license.php
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  Top-Level Makefile for building MagickCoder.
#

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir)

ACLOCAL_AMFLAGS = -I m4

MAGICK_CODER_SRCS = mgk.c

# Additional files to distribute
EXTRA_DIST = LICENSE README.txt

# Headers which are not installed but which are distributed
noinst_HEADERS = filter/blob-private.h \
  filter/delegate-private.h \
  filter/exception-private.h \
  filter/image-private.h \
  filter/monitor-private.h \
  filter/nt-base-private.h \
  filter/pixel-accessor.h \
  filter/quantum-private.h

# Where modules are installed
pkgdir = $(CODER_PATH)

# Architecture-dependent data files installed with modules
# pkg_DATA = 

# Architecture independent data files installed in the package's data directory
# pkgdata_DATA = 

# Modules which are to be built
pkg_LTLIBRARIES   = mgk.la

# MGK module
mgk_la_SOURCES   = mgk.c
mgk_la_LDFLAGS   = -no-undefined -module -avoid-version
mgk_la_LIBADD    = 
