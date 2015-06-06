/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the `dl' library (-ldl). */
#cmakedefine HAVE_LIBDL 1

/* Define to 1 if you have the `GL' library (-lGL). */
#cmakedefine HAVE_LIBGL 1

/* Define to 1 if you have the `GLU' library (-lGLU). */
#cmakedefine HAVE_LIBGLU 1

/* Define to 1 if you have the `glut' library (-lglut). */
#cmakedefine HAVE_LIBGLUT 1

/* Define to 1 if you have the `ICE' library (-lICE). */
#cmakedefine HAVE_LIBICE 1

/* Define to 1 if you have the `m' library (-lm). */
#cmakedefine HAVE_LIBM 1

/* Define to 1 if you have the `openal' library (-lopenal). */
#cmakedefine HAVE_LIBOPENAL 1

/* Define to 1 if you have the `plibsg' library (-lplibsg). */
#cmakedefine HAVE_LIBPLIBSG 1

/* Define to 1 if you have the `plibsl' library (-lplibsl). */
#cmakedefine HAVE_LIBPLIBSL 1

/* Define to 1 if you have the `plibsm' library (-lplibsm). */
#cmakedefine HAVE_LIBPLIBSM 1

/* Define to 1 if you have the `plibssg' library (-lplibssg). */
#cmakedefine HAVE_LIBPLIBSSG 1

/* Define to 1 if you have the `plibssgaux' library (-lplibssgaux). */
#cmakedefine HAVE_LIBPLIBSSGAUX 1

/* Define to 1 if you have the `plibul' library (-lplibul). */
#cmakedefine HAVE_LIBPLIBUL 1

/* Define to 1 if you have the `png' library (-lpng). */
#cmakedefine HAVE_LIBPNG 1

/* Define to 1 if you have the `SM' library (-lSM). */
#cmakedefine HAVE_LIBSM 1

/* Define to 1 if you have the `X11' library (-lX11). */
#cmakedefine HAVE_LIBX11 1

/* Define to 1 if you have the `Xext' library (-lXext). */
#cmakedefine HAVE_LIBXEXT 1

/* Define to 1 if you have the `Xi' library (-lXi). */
#cmakedefine HAVE_LIBXI 1

/* Define to 1 if you have the `Xmu' library (-lXmu). */
#cmakedefine HAVE_LIBXMU 1

/* Define to 1 if you have the `Xrandr' library (-lXrandr). */
#cmakedefine HAVE_LIBXRANDR 1

/* Define to 1 if you have the `Xrender' library (-lXrender). */
#cmakedefine HAVE_LIBXRENDER 1

/* Define to 1 if you have the `Xt' library (-lXt). */
#cmakedefine HAVE_LIBXT 1

/* Define to 1 if you have the `Xxf86vm' library (-lXxf86vm). */
#cmakedefine HAVE_LIBXXF86VM 1

/* Define to 1 if you have the `z' library (-lz). */
#cmakedefine HAVE_LIBZ 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the `strndup' function. */
#cmakedefine HAVE_STRNDUP 1

/* Define to 1 if you have the `strtok_r' function. */
#cmakedefine HAVE_STRTOK_R 1

/* Define to 1 if you have the `isnan' function. */
#cmakedefine HAVE_ISNAN 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Name of package */
#define PACKAGE "speed-dreams"

/* Define the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define the full name of this package. */
#define PACKAGE_NAME ""

/* Define the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define the version of this package. */
#define PACKAGE_VERSION ""

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#cmakedefine TIME_WITH_SYS_TIME 1

/* Version numbers for package */
#ifndef VERSION
#define VERSION "${VERSION}"
#endif //VERSION

#ifndef VERSION_LONG
#define VERSION_LONG "${VERSION_LONG}"
#endif //VERSION_LONG

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
#cmakedefine WORDS_BIGENDIAN 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Build system / configuration information */
#define SD_BUILD_INFO_SYSTEM "${CMAKE_SYSTEM}"
#define SD_BUILD_INFO_CMAKE_VERSION "${CMAKE_VERSION}"
#define SD_BUILD_INFO_CMAKE_GENERATOR "${CMAKE_GENERATOR}"
#define SD_BUILD_INFO_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}"
#if defined(_MSC_VER)
# define SD_BUILD_INFO_CONFIGURATION CMAKE_INTDIR
#else
# define SD_BUILD_INFO_CONFIGURATION "${CMAKE_BUILD_TYPE}"
#endif

#if defined(_MSC_VER)
# define SD_BUILD_INFO_COMPILER_NAME "MSC"
#elif defined(__GNUC__)
# if defined(__MINGW32__)
#  define SD_BUILD_INFO_COMPILER_NAME "MinGW GCC"
# elif defined(__INTEL_COMPILER)
#  define SD_BUILD_INFO_COMPILER_NAME "Intel"
# else
#  define SD_BUILD_INFO_COMPILER_NAME "GCC"
# endif
#else
# define SD_BUILD_INFO_COMPILER_NAME "Unkown"
#endif

/* Run-time directories */
#define SD_DATADIR "${SD_DATADIR}/"
#define SD_DATADIR_SRC "${PROJECT_SOURCE_DIR}/data/"
#define SD_LIBDIR "${SD_LIBDIR}/"
#define SD_BINDIR "${SD_BINDIR}/"
#define SD_LOCALDIR "${SD_LOCALDIR}/"

