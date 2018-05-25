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
#include "reflection.h"
#include "class.h"
#include "method.h"
#include "function.h"
#include "instruction.h"

zend_class_entry *php_inspector_method_ce;

ZEND_BEGIN_ARG_INFO(InspectorMethod_getDeclaringClass_arginfo, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(InspectorMethod, getDeclaringClass)
{
	zend_function *function = 
		php_reflection_object_function(getThis());

	if (function->common.scope) {
		php_inspector_class_factory(
			function->common.scope, return_value);
	}
}

static zend_function_entry php_inspector_method_methods[] = {
	PHP_ME(InspectorFunction, onResolve, InspectorFunction_onResolve_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getInstruction, InspectorFunction_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getInstructionCount, InspectorFunction_getInstructionCount_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getEntryInstruction, InspectorFunction_getEntryInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, findFirstInstruction, InspectorFunction_find_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, findLastInstruction, InspectorFunction_find_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, flushInstructionCache, InspectorFunction_flush_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorMethod, getDeclaringClass, InspectorMethod_getDeclaringClass_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_method) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorMethod", php_inspector_method_methods);

	php_inspector_method_ce = 
		zend_register_internal_class_ex(&ce, reflection_method_ptr);

	zend_class_implements(php_inspector_method_ce, 1, php_inspector_instruction_interface_ce);

	zend_declare_property_null(
		php_inspector_method_ce, 
		ZEND_STRL("instructionCache"), ZEND_ACC_PROTECTED);
	return SUCCESS;
} /* }}} */
#endif
