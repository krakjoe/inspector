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
#include "zend_exceptions.h"
#include "scope.h"

zend_class_entry *php_inspector_method_ce;

/* {{{ */
static PHP_METHOD(Method, __construct)
{
	zend_class_entry *clazz = NULL;
	zend_string *method = NULL;
	zend_function *function = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "CS", &clazz, &method) != SUCCESS) {
		return;
	}

	if (!(function = php_inspector_scope_find(clazz, method))) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot find function %s::%s",
			ZSTR_VAL(clazz->name), ZSTR_VAL(method));
		return;
	}

	php_inspector_scope_construct(getThis(), function);
}

static PHP_METHOD(Method, getName)
{
	php_inspector_scope_t *scope = 
		php_inspector_scope_this();
	zend_string *name = zend_string_alloc(
		ZSTR_LEN(scope->ops->scope->name) +
		ZSTR_LEN(scope->ops->function_name) +
		sizeof("::")-1, 0);

	memcpy(&ZSTR_VAL(name)[0], 
		ZSTR_VAL(scope->ops->scope->name), 
		ZSTR_LEN(scope->ops->scope->name));
	memcpy(&ZSTR_VAL(name)[ZSTR_LEN(scope->ops->scope->name)], 
		"::", 
		sizeof("::")-1);
	memcpy(&ZSTR_VAL(name)[ZSTR_LEN(scope->ops->scope->name) + sizeof("::")-1], 
		ZSTR_VAL(scope->ops->function_name), 
		ZSTR_LEN(scope->ops->function_name));

	ZSTR_VAL(name)[ZSTR_LEN(name)] = 0;

	RETURN_STR(name);
}

ZEND_BEGIN_ARG_INFO_EX(Method_construct_arginfo, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, class, IS_STRING, 0)	
	ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Method_getName_arginfo, 0, 0, IS_STRING, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Method_getName_arginfo, 0, 0, IS_STRING, NULL, 0)
#endif
ZEND_END_ARG_INFO()

static zend_function_entry php_inspector_method_methods[] = {
	PHP_ME(Method, __construct, Method_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Method, getName, Method_getName_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_method) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Method", php_inspector_method_methods);
	php_inspector_method_ce = 
		zend_register_internal_class_ex(&ce, php_inspector_scope_ce);
	php_inspector_method_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
} /* }}} */
#endif
