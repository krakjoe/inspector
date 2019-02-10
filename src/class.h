/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2018 Joe Watkins                                       |
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

#ifndef HAVE_INSPECTOR_CLASS_H
#define HAVE_INSPECTOR_CLASS_H

extern zend_class_entry *php_inspector_class_ce;

typedef struct _php_inspector_class_t {
	zend_class_entry *ce;
	zend_string      *name;
	zend_string      *key;
	zend_bool         expired;
	zval              reflector;
	zend_object       std;
} php_inspector_class_t;

static zend_always_inline php_inspector_class_t* php_inspector_class_fetch(zend_object *o) {
	return (php_inspector_class_t*) (((char*) o) - XtOffsetOf(php_inspector_class_t, std));
}

static zend_always_inline php_inspector_class_t* php_inspector_class_from(zval *z) {
	return php_inspector_class_fetch(Z_OBJ_P(z));
}

void php_inspector_class_factory(zend_class_entry *ce, zval *return_value);
int php_inspector_class_resolve(zval *zv, zend_class_entry *class);

PHP_MINIT_FUNCTION(inspector_class);
#endif
