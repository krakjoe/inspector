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
#ifndef HAVE_INSPECTOR_METHOD
#define HAVE_INSPECTOR_METHOD

#include "php.h"
#include "ext/spl/spl_exceptions.h"
#include "scope.h"

zend_class_entry *php_inspector_method_ce;

/* {{{ */
PHP_METHOD(Method, __construct)
{
	zend_class_entry *clazz = NULL;
	zend_string *method = NULL;
	zend_function *function = NULL;

	if (0) {
InvalidArgumentException:
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
			"%s expects (string class, string method)",
			ZSTR_VAL(EX(func)->common.function_name));
		return;
	}

	switch (ZEND_NUM_ARGS()) {
		case 1: if (zend_parse_parameters(ZEND_NUM_ARGS(), "CS", &clazz, &method) != SUCCESS) {
			return;
		} break;
		
		default:
			goto InvalidArgumentException;
	}

	if (!(function = php_inspector_scope_find(clazz, method))) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot find function %s::%s",
			ZSTR_VAL(clazz->name), ZSTR_VAL(method));
		return;
	}

	php_inspector_scope_construct(getThis(), function);
}

static zend_function_entry php_inspector_method_methods[] = {
	PHP_ME(Method, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; 

PHP_MINIT_FUNCTION(method) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Method", php_inspector_method_methods);
	php_inspector_method_ce = 
		zend_register_internal_class_ex(&ce, php_inspector_scope_ce);

	return SUCCESS;
} /* }}} */
#endif
