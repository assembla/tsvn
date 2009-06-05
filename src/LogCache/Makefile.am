noinst_LIBRARIES = liblogcache.a
liblogcache_a_SOURCES = \
	CopyFollowingLogIterator.cpp\
	CSVWriter.cpp\
	LogIteratorBase.cpp\
	LogCacheSettings.cpp\
	StrictLogIterator.cpp\
	XMLLogReader.cpp\
	XMLLogWriter.cpp

AM_CPPFLAGS = \
	-I$(srcdir)/../Utils\
	-I$(srcdir)/../UtilsProject\
	-I$(srcdir)/../CompatibilityHeaders\
	-I$(srcdir)/../Subversion/subversion/include\
	-I$(srcdir)/../apr/include\
	-I$(srcdir)/../apr-utils/include

# set the include path found by configure
INCLUDES= $(all_includes)

SUBDIRS = Streams Containers
