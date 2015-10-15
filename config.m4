dnl $Id$
dnl config.m4 for extension inspector

PHP_ARG_ENABLE(inspector, whether to enable inspector support,
[  --enable-inspector           Enable inspector support])

if test "$PHP_INSPECTOR" != "no"; then
  PHP_NEW_EXTENSION(inspector, php_inspector.c inspector.c iterator.c node.c operand.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
