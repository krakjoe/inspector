dnl $Id$
dnl config.m4 for extension inspector

PHP_ARG_ENABLE(inspector, whether to enable inspector support,
[  --enable-inspector           Enable inspector support])
PHP_ARG_ENABLE(inspector-coverage,      whether to enable inspector coverage support,
[  --enable-inspector-coverage          Enable inspector coverage support], no, no)

if test "$PHP_INSPECTOR" != "no"; then
  PHP_NEW_EXTENSION(inspector, php_inspector.c src/strings.c src/file.c src/class.c src/method.c src/function.c src/instruction.c src/operand.c src/break.c src/frame.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
  PHP_ADD_BUILD_DIR($ext_builddir/src, 1)
  PHP_ADD_EXTENSION_DEP(inspector, SPL)

  AC_MSG_CHECKING([inspector coverage])
  if test "$PHP_INSPECTOR_COVERAGE" != "no"; then
    AC_MSG_RESULT([enabled])

    PHP_ADD_MAKEFILE_FRAGMENT
  else
    AC_MSG_RESULT([disabled])
  fi
fi
