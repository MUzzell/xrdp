EXTRA_DIST = xrdp-neutrinordp.h
EXTRA_DEFINES =
 
if XRDP_DEBUG_FREERDP1
EXTRA_DEFINES += -DDEBUG_FREERDP1
endif

AM_CFLAGS = \
  -DXRDP_CFG_PATH=\"${sysconfdir}/xrdp\" \
  -DXRDP_SBIN_PATH=\"${sbindir}\" \
  -DXRDP_SHARE_PATH=\"${datadir}/xrdp\" \
  -DXRDP_PID_PATH=\"${localstatedir}/run\" \
  $(EXTRA_DEFINES)

INCLUDES = \
  -I$(top_srcdir)/common \
  $(FREERDP_CFLAGS)

lib_LTLIBRARIES = \
  libxrdpneutrinordp.la

libxrdpneutrinordp_la_SOURCES = xrdp-neutrinordp.c xrdp-color.c

libxrdpneutrinordp_la_LIBADD = \
  $(top_builddir)/common/libcommon.la \
  $(FREERDP_LIBS)
