#
# Copyright (c) 2010 Petr Stetiar <ynezz@true.cz>
# Copyright (c) 2010 Tias Guns <tias@ulyssis.org>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#

AC_PREREQ([2.57])
AC_INIT(xinput_calibrator,[0.7.5], [http://github.com/tias/xinput_calibrator/issues],, [http://www.freedesktop.org/wiki/Software/xinput_calibrator])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_PATH_X
AC_CHECK_HEADERS([stdlib.h string.h])
AC_HEADER_STDBOOL
AC_FUNC_STRTOD

PKG_CHECK_MODULES(XINPUT, x11 xext xi inputproto)
AC_SUBST(XINPUT_CFLAGS)
AC_SUBST(XINPUT_LIBS)

PKG_CHECK_MODULES(XI_PROP, [xi >= 1.2] [inputproto >= 1.5],
			AC_DEFINE(HAVE_XI_PROP, 1, [Xinput properties available]), foo="bar")

PKG_CHECK_MODULES(GTK, [gtk+-2.0],, AC_MSG_ERROR([GTK GUI required, but gtk+-2.0 not found]))
AC_SUBST(GTKMM_CFLAGS)
AC_SUBST(GTKMM_LIBS)

AC_SUBST(VERSION)

AC_OUTPUT([Makefile
           src/Makefile])
