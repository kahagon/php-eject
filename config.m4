dnl
dnl $ Id: $
dnl

PHP_ARG_ENABLE(eject, whether to enable eject functions,
[  --enable-eject         Enable eject support])

if test "$PHP_EJECT" != "no"; then
  export OLD_CPPFLAGS="$CPPFLAGS"
  export CPPFLAGS="$CPPFLAGS $INCLUDES -DHAVE_EJECT"

  AC_MSG_CHECKING(PHP version)
  AC_TRY_COMPILE([#include <php_version.h>], [
#if PHP_VERSION_ID < 40000
#error  this extension requires at least PHP version 4.0.0
#endif
],
[AC_MSG_RESULT(ok)],
[AC_MSG_ERROR([need at least PHP 4.0.0])])

  export CPPFLAGS="$OLD_CPPFLAGS"


  PHP_SUBST(EJECT_SHARED_LIBADD)
  AC_DEFINE(HAVE_EJECT, 1, [ ])

  PHP_NEW_EXTENSION(eject, eject.c , $ext_shared)

fi

