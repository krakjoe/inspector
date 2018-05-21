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

#ifndef HAVE_INSPECTOR_FUNCTION
#define HAVE_INSPECTOR_FUNCTION

#include "php.h"
#include "zend_exceptions.h"

#include "strings.h"
#include "reflection.h"
#include "function.h"
#include "method.h"
#include "instruction.h"
#include "break.h"

zend_class_entry *php_inspector_function_ce;
zend_class_entry *php_inspector_file_ce;

static zend_always_inline zend_bool php_inspector_instruction_guard(zval *this) {
	php_reflection_object_t *reflection =
		php_reflection_object_fetch(this);
	zend_function *function = (zend_function*) reflection->ptr;

	if (reflection->ref_type == PHP_REF_TYPE_PENDING) {
		zend_throw_exception_ex(reflection_exception_ptr, 0, 
			"InspectorInstructionInterface is pending source");
		return 0;
	}

	if (reflection->ref_type == PHP_REF_TYPE_EXPIRED) {
		zend_throw_exception_ex(reflection_exception_ptr, 0, 
			"InspectorInstructionInterface expired");
		return 0;
	}

	if (function->type != ZEND_USER_FUNCTION) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"InspectorInstructionInterface doesn't work on internal functions");
		return 0;
	}

	return 1;
}

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

	if (function->common.function_name) {
		zval k, v;

		ZVAL_STR(&k, PHP_INSPECTOR_STRING_NAME);
		ZVAL_STR(&v, function->common.function_name);

		zend_std_write_property(return_value, &k, &v, NULL);
	}

	if (function->common.scope && !function->common.fn_flags & ZEND_ACC_CLOSURE) {
		zval k, v;

		ZVAL_STR(&k, PHP_INSPECTOR_STRING_CLASS);
		ZVAL_STR(&v, function->common.scope->name);

		zend_std_write_property(return_value, &k, &v, NULL);
	}
}

PHP_METHOD(InspectorFunction, getInstruction)
{
	zend_function *function = 
		php_reflection_object_function(getThis());
	zend_long num = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|l", &num) != SUCCESS) {
		return;
	}

	if (!php_inspector_instruction_guard(getThis())) {
		return;
	}

	if (num < 0) {
		num += function->op_array.last - 1;
	}

	if (num < 0 || num > function->op_array.last) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"instruction %d is out of bounds", num);
		return;
	}

	php_inspector_instruction_factory(getThis(), &function->op_array.opcodes[num], return_value);
}

PHP_METHOD(InspectorFunction, getInstructionCount)
{
	zend_function *function =
		php_reflection_object_function(getThis());

	if (!php_inspector_instruction_guard(getThis())) {
		return;
	}

	RETURN_LONG(function->op_array.last - 1);
}

PHP_METHOD(InspectorFunction, getEntryInstruction)
{
	zend_function *function =
		php_reflection_object_function(getThis());
	zend_op *op;

	if (!php_inspector_instruction_guard(getThis())) {
		return;
	}

	op = function->op_array.opcodes + function->op_array.num_args;

	if (op < function->op_array.opcodes + (function->op_array.last - 1)) {
		php_inspector_instruction_factory(getThis(), op, return_value);
		return;
	}

	zend_throw_exception_ex(reflection_exception_ptr, 0, "function has no entry point");
}

PHP_METHOD(InspectorFunction, findFirstInstruction)
{
	zend_function *function =
		php_reflection_object_function(getThis());
	zend_long offset = 0;
	zend_long opcode = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "l|l", &opcode, &offset) != SUCCESS) {
		return;
	}

	if (!php_inspector_instruction_guard(getThis())) {
		return;
	}

	if (offset < 0) {
		offset += function->op_array.last - 1;
	}

	if (offset < 0 || offset > function->op_array.last) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"offset %d is out of bounds", offset);
		return;
	}

	{
		zend_op *op = &function->op_array.opcodes[offset],
			*end = function->op_array.opcodes + function->op_array.last;
		zend_uchar find = (zend_uchar) opcode;

		while (op < end) {
			if (op->opcode == find) {
				php_inspector_instruction_factory(getThis(), op, return_value);
				break;
			}
			op++;
		}
	}
}

PHP_METHOD(InspectorFunction, findLastInstruction)
{
	zend_function *function =
		php_reflection_object_function(getThis());
	zend_long offset = 0;
	zend_long opcode = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "l|l", &opcode, &offset) != SUCCESS) {
		return;
	}

	if (!php_inspector_instruction_guard(getThis())) {
		return;
	}

	if (offset <= 0) {
		offset += function->op_array.last - 1;
	}

	if (offset < 0 || offset > function->op_array.last) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"offset %d is out of bounds", offset);
		return;
	}

	{
		zend_op *op = &function->op_array.opcodes[offset],
			*end = function->op_array.opcodes;
		zend_uchar find = (zend_uchar) opcode;

		while (op > end) {
			if (op->opcode == find) {
				php_inspector_instruction_factory(getThis(), op, return_value);
				break;
			}
			op--;
		}
	}
}

PHP_METHOD(InspectorFunction, flushInstructionCache)
{
	php_inspector_instruction_cache_flush(getThis());
}

static zend_function_entry php_inspector_function_methods[] = {
	PHP_ME(InspectorFunction, getInstruction, InspectorFunction_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getInstructionCount, InspectorFunction_getInstructionCount_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getEntryInstruction, InspectorFunction_getEntryInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, findFirstInstruction, InspectorFunction_find_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, findLastInstruction, InspectorFunction_find_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, flushInstructionCache, InspectorFunction_flush_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_function) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorFunction", php_inspector_function_methods);

	php_inspector_function_ce = 
		zend_register_internal_class_ex(&ce, reflection_function_ptr);

	zend_class_implements(php_inspector_function_ce, 1, php_inspector_instruction_interface_ce);

	zend_declare_property_null(
		php_inspector_function_ce, 
		ZEND_STRL("instructionCache"), ZEND_ACC_PROTECTED);

	return SUCCESS;
}  /* }}} */
#endif
