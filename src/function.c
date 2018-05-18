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
#ifndef HAVE_INSPECTOR_FUNCTION
#define HAVE_INSPECTOR_FUNCTION

#include "php.h"
#include "zend_exceptions.h"

#include "reflection.h"
#include "function.h"
#include "method.h"
#include "instruction.h"

zend_class_entry *php_inspector_function_ce;

void php_inspector_function_factory(zend_function *function, zval *return_value) {
	php_reflection_object_t *reflection;

	if (function->common.scope) {
		object_init_ex(return_value, php_inspector_method_ce);
	} else {
		object_init_ex(return_value, php_inspector_function_ce);
	}

	reflection = php_reflection_object_fetch(return_value);
	reflection->ptr = function;
	reflection->ref_type = PHP_REF_TYPE_OTHER;
}

PHP_METHOD(InspectorFunction, getInstruction)
{
	zend_function *function = 
		php_reflection_object_function(getThis());
	zend_long num = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|l", &num) != SUCCESS) {
		return;
	}

	if (function->type != ZEND_USER_FUNCTION) {
		zend_throw_exception_ex(reflection_exception_ptr, 0, 
			"cannot get instruction from internal code");
		return;
	}

	if (num < 0 || num > function->op_array.last) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"instruction %d is out of bounds", num);
		return;
	}

	php_inspector_instruction_factory(getThis(), &function->op_array.opcodes[num], return_value);
}

static zend_function_entry php_inspector_function_methods[] = {
	PHP_ME(InspectorFunction, getInstruction, InspectorFunction_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_function) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorFunction", php_inspector_function_methods);
	php_inspector_function_ce = zend_register_internal_class_ex(&ce, reflection_function_ptr);

	return SUCCESS;
}  /* }}} */
#endif
