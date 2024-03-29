ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall ${PCAP_CFLAGS} ${PFRING_CFLAGS}

lib_LTLIBRARIES = libcap_utils-07.la libcap_filter-07.la libcap_stream-07.la libcap_marc-07.la
bin_PROGRAMS = 
man1_MANS =
man3_MANS =             \
	man/libcaputils.3     \
	man/libcap_filter.3   \
	man/stream_read.3
notrans_dist_man_MANS = \
	man/libcaputils.3     \
	man/libcap_filter.3   \
	man/stream_read.3

if BUILD_PCAP
bin_PROGRAMS += pcap2cap cap2pcap
endif

if BUILD_CAPINFO
bin_PROGRAMS += capinfo
endif

if BUILD_CAPDUMP
bin_PROGRAMS += capdump
man1_MANS += man/capdump.1
notrans_dist_man_MANS += man/capdump.1
endif

if BUILD_CAPFILTER
bin_PROGRAMS += capfilter
man1_MANS += man/capfilter.1
notrans_dist_man_MANS += man/capfilter.1
endif

if BUILD_CAPFILTER
bin_PROGRAMS += capmarker
man1_MANS += man/capmarker.1
notrans_dist_man_MANS += man/capmarker.1
endif

if BUILD_CAPSHOW
bin_PROGRAMS += capshow
man1_MANS += man/capshow.1
notrans_dist_man_MANS += man/capshow.1
endif

if BUILD_TESTS
TESTS = tests/filter_argv tests/address tests/endian tests/timepico
else
TESTS = tests/disabled
endif
check_PROGRAMS = ${TESTS}

nobase_include_HEADERS =    \
	caputils/address.h  \
	caputils/picotime.h \
	caputils/file.h     \
	caputils/utils.h    \
	caputils/capture.h  \
	caputils/filter.h   \
	caputils/caputils.h \
	caputils/log.h      \
	caputils/stream.h   \
	caputils/marc.h     \
	caputils/marker.h   \
	caputils/version.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcap_stream-0.7.pc libcap_filter-0.7.pc libcap_utils-0.7.pc libcap_marc-0.7.pc

libcap_stream_07_la_LDFLAGS = -version-info 1:0:1 ${PFRING_LIBS}
libcap_stream_07_la_LIBADD = libcap_filter-07.la
libcap_stream_07_la_SOURCES = stream.c stream.h stream_file.c

if BUILD_PFRING
libcap_stream_07_la_SOURCES += stream_pfring.c
else
libcap_stream_07_la_SOURCES += stream_ethernet.c
endif

libcap_utils_07_la_LDFLAGS = -version-info 0:1:0
libcap_utils_07_la_LIBADD = libcap_stream-07.la libcap_filter-07.la libcap_marc-07.la
libcap_utils_07_la_SOURCES =   \
	caputils_int.h      \
	address.c           \
	error.c             \
	log.c               \
	marker.c            \
	utils.c             \
	picotime.c

if BUILD_BE64TOH
libcap_utils_07_la_SOURCES += be64toh.c be64toh.h
endif

#	stream_udp.c \
#	stream_tcp.c \
#

libcap_filter_07_la_LDFLAGS = -version-info 0:1:0
libcap_filter_07_la_SOURCES = createfilter.c filter.c

libcap_marc_07_la_LDFLAGS = -shared -version-info 0:1:0
libcap_marc_07_la_CFLAGS = ${AM_CFLAGS} ${libcap_filter_CFLAGS}
libcap_marc_07_la_SOURCES = marc.c

pcap2cap_LDADD = libcap_utils-07.la libcap_filter-07.la libcap_stream-07.la ${PCAP_LIBS}
cap2pcap_LDADD = libcap_utils-07.la libcap_filter-07.la libcap_stream-07.la ${PCAP_LIBS}
capinfo_LDADD = libcap_utils-07.la libcap_filter-07.la libcap_stream-07.la
capdump_LDADD = libcap_utils-07.la libcap_filter-07.la libcap_stream-07.la
capfilter_LDADD = libcap_filter-07.la libcap_utils-07.la libcap_stream-07.la
capmarker_LDADD = 
capshow_LDADD = libcap_filter-07.la libcap_utils-07.la libcap_stream-07.la

tests_filter_argv_CXXFLAGS = ${AM_CFLAGS} $(CPPUNIT_CFLAGS)
tests_filter_argv_LDFLAGS = $(CPPUNIT_LIBS)
tests_filter_argv_LDADD = libcap_filter-07.la libcap_utils-07.la
tests_filter_argv_SOURCES = tests/filter_argv.cpp

tests_address_CXXFLAGS = ${AM_CFLAGS} $(CPPUNIT_CFLAGS)
tests_address_LDFLAGS = $(CPPUNIT_LIBS)
tests_address_LDADD = libcap_utils-07.la
tests_address_SOURCES = tests/address.cpp

# -Dfulhack is needed so libtool will become less confused about
# multiple objects of be64toh. By having different cflags it will
# rename the files.
tests_endian_CFLAGS = ${AM_CFLAGS} -Dfulhack
tests_endian_CXXFLAGS = ${AM_CFLAGS} $(CPPUNIT_CFLAGS)
tests_endian_LDFLAGS = $(CPPUNIT_LIBS)
tests_endian_LDADD = libcap_utils-07.la
tests_endian_SOURCES = tests/endian.cpp be64toh.c

tests_timepico_CXXFLAGS = ${AM_CFLAGS} $(CPPUNIT_CFLAGS)
tests_timepico_LDFLAGS = $(CPPUNIT_LIBS)
tests_timepico_LDADD = libcap_utils-07.la
tests_timepico_SOURCES = tests/timepico.cpp

tests_disabled_SOURCES = tests/disabled.cpp

#
# Hooks
#

if BUILD_CAPSHOW
install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	  test -h consumer-ls$(EXEEXT) || $(LN_S) capshow$(EXEEXT) consumer-ls$(EXEEXT)
endif

#
# Relase related targets
#

debpkgname=libcap-utils_@VERSION@_@ARCH@
releasedir=/var/marc/releases

deb: all
	@test "x${prefix}" = "x/usr" || (echo "Error: --prefix must be /usr when creating debian release (currently ${prefix})"; exit 1)
	mkdir -p $(debpkgname)/DEBIAN
	cp dist/deb-control $(debpkgname)/DEBIAN/control
	$(MAKE) install DESTDIR=`pwd`/$(debpkgname)
	dpkg-deb --build $(debpkgname)

# special target for dsvmarc releases
release_marc: dist distcheck deb
	cp $(distdir).tar.gz $(releasedir)
	cp $(debpkgname).deb $(releasedir)
	cat $(top_srcdir)//ChangeLog | awk -vRS= '{print; if ( NR == 2 ){ exit }}' > $(releasedir)/$(distdir).changelog.txt
