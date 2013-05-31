bin_PROGRAMS=bswap
bswap_SOURCES=bswap.c xmalloc.c system.h aclocal.m4 bswap.1

#  uncomment the following if bswap requires the math library
#bswap_LDADD=-lm

EXTRA_DIST=bswap.lsm.in bswap.spec.in bswap.texinfo

#  if you write a self-test script named `chk', uncomment the
#  following and add `chk' to the EXTRA_DIST list
#TESTS=chk

#  build and install the .info pages
info_TEXINFOS = bswap.texinfo
bswap_TEXINFOS = gpl.texinfo

#  install the man pages
man_MANS=bswap.1
