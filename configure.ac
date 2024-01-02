#  Copyright 1999-2021 ImageMagick Studio LLC, a non-profit organization
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
AC_PREREQ([2.69])

# ==============================================================================
# Initialize Automake
# ==============================================================================
m4_include([m4/version.m4])
m4_define([magick_base_version],
          [magick_major_version.magick_minor_version.magick_micro_version])
m4_define([magick_version],
          [magick_base_version-magick_patchlevel_version])
m4_define([magick_lib_version_number],
          [magick_major_version,magick_minor_version,magick_micro_version,magick_patchlevel_version])
m4_define([magick_git_revision],
          m4_esyscmd([
            c=$(git log --full-history --format=tformat:. HEAD | wc -l)
            h=$(git rev-parse --short HEAD)
            d=$(date +%Y%m%d)
            printf %s "$c:$h:$d"
          ]))
m4_define([magick_release_date],
          m4_esyscmd([
            d=$(date +%F -r ./ChangeLog)
            printf %s "$d"
          ]))

AC_INIT([magick_name],[magick_version],[magick_bugreport],[magick_tarname],[magick_url])

AC_CONFIG_SRCDIR([mgk.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config/config.h])
AC_CANONICAL_TARGET
AC_CONFIG_FILES([Makefile])

AM_INIT_AUTOMAKE([foreign no-define color-tests parallel-tests -Wall -Wno-portability subdir-objects dist-bzip2 dist-lzip dist-xz dist-zip tar-ustar])

AM_SILENT_RULES([yes])
AC_SUBST([CONFIGURE_DEPENDENCIES],["$CONFIGURE_DEPENDENCIES \$(top_srcdir)/ChangeLog \$(top_srcdir)/m4/version.m4"])

# Check for programs
AC_PROG_CC
AC_PROG_CPP
LT_PATH_LD
AC_SUBST(LD)
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S
AM_WITH_DMALLOC

#
# Enable OS features.
#
AC_USE_SYSTEM_EXTENSIONS

#
# Tests for Windows
#
AC_EXEEXT
AC_OBJEXT

#
# Configure libtool
#
LT_INIT([win32-dll dlopen])
LT_LANG([C++])
AC_SUBST(LIBTOOL_DEPS)

# Check to see if building shared libraries
libtool_build_shared_libs='no'
if test "$enable_shared" = 'yes'; then
    libtool_build_shared_libs='yes'
fi

# Check to see if building static libraries
libtool_build_static_libs='no'
if test "$enable_static" = 'yes'; then
    libtool_build_static_libs='yes'
fi

AM_CONDITIONAL(WITH_SHARED_LIBS, test "${libtool_build_shared_libs}" = 'yes')

# Add configure option --enable-maintainer-mode which enables dependency
# checking and generation useful to package maintainers.  This is made an
# option to avoid confusing end users.
AM_MAINTAINER_MODE

#
# Obtain ImageMagick compilation options from installed MagickCore-config script.
#
AC_MSG_CHECKING(pre-processor flags)
CPPFLAGS="$CPPFLAGS "`MagickCore-config --cppflags`
AC_MSG_RESULT("$CPPFLAGS")

AC_MSG_CHECKING(linker flags)
LDFLAGS="$LDFLAGS "`MagickCore-config --ldflags`
AC_MSG_RESULT("$LDFLAGS")

AC_MSG_CHECKING(libraries)
LIBS="$LIBS "`MagickCore-config --libs`
AC_MSG_RESULT("$LIBS")

AC_MSG_CHECKING(coder path)
CODER_PATH=`pkg-config --variable=moduledir MagickCore`/coders
AC_MSG_RESULT("$CODER_PATH")
AC_SUBST(CODER_PATH)

AC_OUTPUT

echo ""
echo "Final Compilation Flags:"
echo "  CC       = $CC"
echo "  CFLAGS   = $CFLAGS"
echo "  CPPFLAGS = $CPPFLAGS"
echo "  LDFLAGS  = $LDFLAGS"
echo "  LIBS     = $LIBS"
echo ""
