if HAVE_WINDOWS
noinst_LTLIBRARIES = libdc1394-windows.la
endif

AM_CFLAGS = -DDC1394_DLL_EXPORTS -I$(top_srcdir) -I$(srcdir)/..
libdc1394_windows_la_SOURCES = control.c capture.c platform_windows.h
