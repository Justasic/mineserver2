#ifndef _SYSCONF_H_
#define _SYSCONF_H_

#cmakedefine HAVE_BACKTRACE 1
#cmakedefine HAVE_SYS_UTSNAME_H 1

#define VERSION_FULL "@mineserver_MAJOR_VERSION@.@mineserver_MINOR_VERSION@.@mineserver_PATCH_LEVEL@"

#ifdef HAVE_BACKTRACE
# include <execinfo.h>
#endif

#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h>
#endif

#endif // _SYSCONF_H_