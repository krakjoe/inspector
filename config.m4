dnl $Id$
dnl config.m4 for extension inspector

PHP_ARG_ENABLE(inspector, whether to enable inspector support,
[  --enable-inspector           Enable inspector support])

if test "$PHP_INSPECTOR" != "no"; then
  PHP_NEW_EXTENSION(inspector, php_inspector.c src/scope.c src/closure.c src/file.c src/global.c src/method.c src/opline.c src/operand.c src/entry.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
  PHP_ADD_BUILD_DIR($ext_builddir/src, 1)
  PHP_ADD_EXTENSION_DEP(inspector, SPL)
fi
