/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: krakjoe                                                      |
  +----------------------------------------------------------------------+
*/

/* $Id$ */
#ifndef HAVE_INSPECTOR_OPLINE_H
#define HAVE_INSPECTOR_OPLINE_H
typedef struct _php_inspector_opline_t {
	zval scope;
	zend_op *opline;
	zend_object std;
} php_inspector_opline_t;

extern zend_class_entry *php_inspector_opline_ce;

#define PHP_INSPECTOR_OPLINE_INVALID	0
#define PHP_INSPECTOR_OPLINE_OP1		1
#define PHP_INSPECTOR_OPLINE_OP2		2
#define PHP_INSPECTOR_OPLINE_RESULT		3

#define php_inspector_opline_fetch_from(o) ((php_inspector_opline_t*) (((char*)o) - XtOffsetOf(php_inspector_opline_t, std)))
#define php_inspector_opline_fetch(z) php_inspector_opline_fetch_from(Z_OBJ_P(z))
#define php_inspector_opline_this() php_inspector_opline_fetch(getThis())

void php_inspector_opline_construct(zval *object, zval *scope, zend_op *opline);

PHP_MINIT_FUNCTION(inspector_opline);
#endif
