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

#ifndef HAVE_INSPECTOR_METHOD
#define HAVE_INSPECTOR_METHOD

#include "php.h"

#include "zend_interfaces.h"
#include "zend_exceptions.h"

#include "reflection.h"
#include "class.h"
#include "method.h"
#include "function.h"
#include "instruction.h"

zend_class_entry *php_inspector_method_ce;

ZEND_BEGIN_ARG_INFO(InspectorMethod_getDeclaringClass_arginfo, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(InspectorMethod_construct_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, class)
	ZEND_ARG_INFO(0, method)
ZEND_END_ARG_INFO()

static PHP_METHOD(InspectorMethod, __construct)
{
	php_inspector_function_t *function = php_inspector_function_from(getThis());
	zend_class_entry *class = NULL;
	zend_string *method = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "CS", &class, &method) != SUCCESS) {
		return;
	}

	function->name = zend_string_copy(method);
	function->key  = zend_string_tolower(function->name);

	if (!(function->function = zend_hash_find_ptr(&class->function_table, function->key))) {
		zend_throw_exception_ex(reflection_exception_ptr, 0, 
			"method %s::%s does not exist", ZSTR_VAL(class->name), ZSTR_VAL(method));
		return;
	}

	php_inspector_function_resolve(getThis(), function->function);
}

static PHP_METHOD(InspectorMethod, getDeclaringClass)
{
	zend_function *function = 
		php_inspector_function_from(getThis())->function;

	if (function->common.scope) {
		php_inspector_class_factory(
			function->common.scope, return_value);
	}
}

static PHP_METHOD(InspectorMethod, __call)
{
	php_inspector_function_t *function = php_inspector_function_from(getThis());
	zend_string *method = NULL;
	zend_string *key    = NULL;
	zval        *args   = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "Sa", &method, &args) != SUCCESS) {
		return;
	}

	if (Z_ISUNDEF(function->reflector)) {
		zval cl, m;
		zval rv;

		ZVAL_STR(&cl, function->function->common.scope->name);
		ZVAL_STR(&m,  function->name);

		object_init_ex(&function->reflector, reflection_method_ptr);

		zend_call_method_with_2_params(
			&function->reflector, 
			reflection_method_ptr, 
			&reflection_method_ptr->constructor, "__construct", 
			&rv, &cl, &m);
	}

	php_inspector_reflector_call(&function->reflector, method, args, return_value);
}

static zend_function_entry php_inspector_method_methods[] = {
	PHP_ME(InspectorMethod, __construct, InspectorMethod_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, onResolve, InspectorFunction_onResolve_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getInstruction, InspectorFunction_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getInstructionCount, InspectorFunction_getInstructionCount_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getEntryInstruction, InspectorFunction_getEntryInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, findFirstInstruction, InspectorFunction_find_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, findLastInstruction, InspectorFunction_find_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, flushInstructionCache, InspectorFunction_flush_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorMethod, getDeclaringClass, InspectorMethod_getDeclaringClass_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorMethod, __call, InspectorReflector_call_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_method) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorMethod", php_inspector_method_methods);

	php_inspector_method_ce = 
		zend_register_internal_class_ex(&ce, php_inspector_function_ce);

	return SUCCESS;
} /* }}} */
#endif
