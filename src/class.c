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
#ifndef HAVE_INSPECTOR_CLASS
#define HAVE_INSPECTOR_CLASS

#include "php.h"
#include "php_inspector.h"

#include "zend_exceptions.h"

#include "reflection.h"
#include "class.h"
#include "function.h"

zend_class_entry *php_inspector_class_ce;

/* {{{ */
void php_inspector_class_factory(zend_class_entry *ce, zval *return_value) {
	php_reflection_object_t *reflection;

	object_init_ex(return_value, reflection_class_ptr);

	reflection = php_reflection_object_fetch(return_value);
	reflection->ptr = ce;
	reflection->ref_type = PHP_REF_TYPE_OTHER;
} /* }}} */

/* {{{ */
zend_function* php_inspector_method_find(zend_class_entry *class, zend_string *name) {
	zend_string *lower = zend_string_tolower(name);
	zend_function *found = 
		(zend_function*)
			zend_hash_find_ptr(&class->function_table, lower);
	zend_string_release(lower);
	return found;
} /* }}} */

/* {{{ */
static PHP_METHOD(InspectorClass, getMethod) {
	zend_class_entry *ce =
		php_reflection_object_class(getThis());
	zend_string *method;
	zend_function *function;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &method) != SUCCESS) {
		return;
	}

	if (!(function = php_inspector_method_find(ce, method))) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"Method %s does not exist", ZSTR_VAL(method));
		return;
	}

	php_inspector_function_factory(function, return_value);
}
/* }}} */

/* {{{ */
ZEND_BEGIN_ARG_INFO(InspectorClass_getMethod_arginfo, 1)
	ZEND_ARG_INFO(0, method)
ZEND_END_ARG_INFO() /* }}} */

/* {{{ */
static zend_function_entry php_inspector_class_methods[] = {
	PHP_ME(InspectorClass, getMethod, InspectorClass_getMethod_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(inspector_class) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorClass", php_inspector_class_methods);
	php_inspector_class_ce = 
		zend_register_internal_class_ex(&ce, reflection_class_ptr);

	return SUCCESS;
} /* }}} */
#endif
