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
#include "zend_interfaces.h"
#include "zend_vm.h"

#include "php_inspector.h"

#include "strings.h"
#include "reflection.h"
#include "function.h"
#include "method.h"
#include "instruction.h"
#include "break.h"

zend_class_entry *php_inspector_function_ce;
zend_class_entry *php_inspector_file_ce;

zend_function* php_inspector_function_replace(zend_function *function);

static zend_always_inline zend_bool php_inspector_function_guard(zval *object) {
	php_reflection_object_t *reflection =
		php_reflection_object_fetch(object);
	zend_function *function = (zend_function*) reflection->ptr;

	if (!php_inspector_reflection_guard(object)) {
		return 0;
	}

	if (function->type != ZEND_USER_FUNCTION) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"InspectorInstructionInterface doesn't work on internal functions");
		return 0;
	}

	return 1;
}

void php_inspector_function_factory(zend_function *function, zval *return_value, zend_bool replace) {
	php_reflection_object_t *reflection;

	if (function->common.scope) {
		object_init_ex(return_value, php_inspector_method_ce);
	} else {
		object_init_ex(return_value, php_inspector_function_ce);
	}

	reflection = php_reflection_object_fetch(return_value);
	reflection->ptr = replace ? php_inspector_function_replace(function) : function;
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

PHP_METHOD(InspectorFunction, __construct)
{
	zval *function = NULL;
	zend_string *name = NULL;
	php_reflection_object_t *reflection =
		php_reflection_object_fetch(getThis());

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "z", &function) != SUCCESS) {
		return;
	}

	if (Z_TYPE_P(function) == IS_STRING) {
		name = zend_string_tolower(Z_STR_P(function));

		if (!zend_hash_exists(EG(function_table), name)) {
			/* create pending */
			reflection->ref_type = PHP_REF_TYPE_PENDING;

			php_inspector_table_insert(
				PHP_INSPECTOR_ROOT_PENDING, 
				PHP_INSPECTOR_TABLE_FUNCTION, 
				Z_STR_P(function), getThis());

			zend_string_release(name);
			return;
		}

		zend_string_release(name);
	}

	zend_call_method_with_1_params(getThis(), Z_OBJCE_P(getThis()), &EX(func)->common.scope->parent->constructor, "__construct", return_value, function);
}

PHP_METHOD(InspectorFunction, onResolve)
{
	
}

PHP_METHOD(InspectorFunction, getInstruction)
{
	zend_function *function = 
		php_reflection_object_function(getThis());
	zend_long num = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|l", &num) != SUCCESS) {
		return;
	}

	if (!php_inspector_function_guard(getThis())) {
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

	if (!php_inspector_function_guard(getThis())) {
		return;
	}

	RETURN_LONG(function->op_array.last - 1);
}

PHP_METHOD(InspectorFunction, getEntryInstruction)
{
	zend_function *function =
		php_reflection_object_function(getThis());
	zend_op *op = function->op_array.opcodes;

	if (!php_inspector_function_guard(getThis())) {
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

	if (!php_inspector_function_guard(getThis())) {
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

	if (!php_inspector_function_guard(getThis())) {
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

zend_string** php_inspector_function_copy_vars(zend_string **vars, uint32_t last) {
	zend_string **copy = ecalloc(last, sizeof(zend_string*) * last);
	zend_string **var, **end;

	memcpy(copy, vars, sizeof(zend_string*) * last);

	var = copy;
	end = var + last;

	while (var < end) {
		zend_string_addref(*var);
		var++;
	}

	return copy;
}

zend_op* php_inspector_function_copy_opcodes(zend_op_array *function, zend_op *opcodes, uint32_t last) {
	zend_op *copy = ecalloc(last, sizeof(zend_op));
	zend_op *opline, *end;

	memcpy(copy, opcodes, sizeof(zend_op) * last);

	opline = opcodes;
	end    = opline + last;

	while (opline < end) {
		if (opline->op1_type == IS_CONST) {
			ZEND_PASS_TWO_UNDO_CONSTANT(function, opline->op1);
		}
		if (opline->op2_type == IS_CONST) {
			ZEND_PASS_TWO_UNDO_CONSTANT(function, opline->op2);
		}
		switch (opline->opcode) {
			case ZEND_JMP:
			case ZEND_FAST_CALL:
			case ZEND_DECLARE_ANON_CLASS:
			case ZEND_DECLARE_ANON_INHERITED_CLASS:
				ZEND_PASS_TWO_UNDO_JMP_TARGET(function, opline, opline->op1);
				break;
			case ZEND_JMPZNZ:
				opline->extended_value = ZEND_OFFSET_TO_OPLINE_NUM(function, opline, opline->extended_value);
				/* break omitted intentionally */
			case ZEND_JMPZ:
			case ZEND_JMPNZ:
			case ZEND_JMPZ_EX:
			case ZEND_JMPNZ_EX:
			case ZEND_JMP_SET:
			case ZEND_COALESCE:
			case ZEND_NEW:
			case ZEND_FE_RESET_R:
			case ZEND_FE_RESET_RW:
			case ZEND_ASSERT_CHECK:
				ZEND_PASS_TWO_UNDO_JMP_TARGET(function, opline, opline->op2);
				break;
			case ZEND_FE_FETCH_R:
			case ZEND_FE_FETCH_RW:
				opline->extended_value = ZEND_OFFSET_TO_OPLINE_NUM(function, opline, opline->extended_value);
				break;
		}
		opline++;
	}

	opline = opcodes;
	end    = opline + last;

	while (opline < end) {
		if (opline->op1_type == IS_CONST) {
			ZEND_PASS_TWO_UPDATE_CONSTANT(function, opline->op1);
		}
		if (opline->op2_type == IS_CONST) {
			ZEND_PASS_TWO_UPDATE_CONSTANT(function, opline->op2);
		}
		switch (opline->opcode) {
			case ZEND_JMP:
			case ZEND_FAST_CALL:
			case ZEND_DECLARE_ANON_CLASS:
			case ZEND_DECLARE_ANON_INHERITED_CLASS:
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(function, opline, opline->op1);
				break;
			case ZEND_JMPZNZ:
				opline->extended_value = ZEND_OPLINE_NUM_TO_OFFSET(function, opline, opline->extended_value);
				/* break omitted intentionally */
			case ZEND_JMPZ:
			case ZEND_JMPNZ:
			case ZEND_JMPZ_EX:
			case ZEND_JMPNZ_EX:
			case ZEND_JMP_SET:
			case ZEND_COALESCE:
			case ZEND_NEW:
			case ZEND_FE_RESET_R:
			case ZEND_FE_RESET_RW:
			case ZEND_ASSERT_CHECK:
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(function, opline, opline->op2);
				break;
			case ZEND_FE_FETCH_R:
			case ZEND_FE_FETCH_RW:
				opline->extended_value = ZEND_OPLINE_NUM_TO_OFFSET(function, opline, opline->extended_value);
				break;
		}
		zend_vm_set_opcode_handler(opline);
		opline++;
	}

	return copy;	
}

zval* php_inspector_function_copy_literals(zval *literals, int last) {
	zval *literal, *end;
	zval *copy = (zval*) ecalloc(last, sizeof(zval));

	memcpy(copy, literals, sizeof(zval) * last);

	literal = copy;
	end     = literal + last - 1;

	while (literal < end) {
		Z_TRY_ADDREF_P(literal);
		literal++;
	}

	return copy;
}

zend_arg_info* php_inspector_function_copy_arginfo(zend_function *function, zend_arg_info *arg_info, int last) {
	zend_arg_info *copy;
	
	if (function->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		last++;
		arg_info--;
	}

	copy = ecalloc(last, sizeof(zend_arg_info));

	memcpy(copy, arg_info, sizeof(zend_arg_info) * last);

	if (function->common.fn_flags & ZEND_ACC_HAS_RETURN_TYPE) {
		copy++;
	}

	return copy;
}

zend_function* php_inspector_function_replace(zend_function *function) {
	zend_op_array *copy;
	zend_op *opline, *end;
	zend_string *var;

	if (!ZEND_USER_CODE(function->type)) {
		return function;
	}

	copy = (zend_op_array*) zend_arena_alloc(&CG(arena), sizeof(zend_op_array));

	memcpy(copy, function, sizeof(zend_op_array));

	copy->refcount = ecalloc(1, sizeof(uint32_t));
	(*copy->refcount) = 2;

	copy->opcodes  = php_inspector_function_copy_opcodes(
		copy, function->op_array.opcodes, function->op_array.last);
	copy->vars     = php_inspector_function_copy_vars(
		function->op_array.vars, function->op_array.last_var);
	copy->literals = php_inspector_function_copy_literals(
		function->op_array.literals, function->op_array.last_literal);
	copy->arg_info = php_inspector_function_copy_arginfo(function, function->op_array.arg_info, function->op_array.num_args);

	if (copy->function_name) {
		zend_string *name = zend_string_tolower(copy->function_name);

		if (copy->scope) {
			zend_hash_update_ptr(&copy->scope->function_table, name, copy);
		} else {
			zend_hash_update_ptr(EG(function_table), name, copy);
		}
	} else {
		memcpy(function, copy, sizeof(zend_op_array));
	}

	return copy;
}

int php_inspector_function_resolve(zval *zv, zend_function *ops) {
	php_reflection_object_t *reflector = 
		php_reflection_object_fetch(zv);
	zend_function *oldFunction = 
		php_reflection_object_function(zv);
	zend_function *onResolve = zend_hash_find_ptr(
		&Z_OBJCE_P(zv)->function_table, PHP_INSPECTOR_STRING_ONRESOLVE);

	if (oldFunction) {
		php_inspector_breaks_purge(oldFunction);

		php_inspector_instruction_cache_flush(zv);
	}

	reflector->ptr = php_inspector_function_replace(ops);
	reflector->ref_type = PHP_REF_TYPE_OTHER;

	if (ZEND_USER_CODE(onResolve->type)) {
		zval rv;

		ZVAL_NULL(&rv);

		zend_call_method_with_0_params(zv, Z_OBJCE_P(zv), &onResolve, "onresolve", &rv);

		if (Z_REFCOUNTED(rv)) {
			zval_ptr_dtor(&rv);
		}

		if (ops->common.function_name) {
			php_inspector_table_insert(
				PHP_INSPECTOR_ROOT_REGISTERED, 
				PHP_INSPECTOR_TABLE_FUNCTION, 
				ops->common.function_name, zv);
		} else {
			php_inspector_table_insert(
				PHP_INSPECTOR_ROOT_REGISTERED,
				PHP_INSPECTOR_TABLE_FILE,
				ops->op_array.filename, zv);
		}
	}

	if (instanceof_function(Z_OBJCE_P(zv), php_inspector_file_ce)) {
		reflector->ref_type = PHP_REF_TYPE_EXPIRED;
	}

	return ZEND_HASH_APPLY_REMOVE;
}

static int php_inspector_function_remove(zend_function *function) {
	HashTable *registered = php_inspector_table(
		PHP_INSPECTOR_ROOT_REGISTERED, 
		PHP_INSPECTOR_TABLE_FUNCTION, 
		function->common.function_name, 0);
	zval *object;

	if (!registered) {
		return ZEND_HASH_APPLY_REMOVE;
	}

	ZEND_HASH_FOREACH_VAL(registered, object) {
		php_reflection_object_t *reflection =
			php_reflection_object_fetch(object);
	
		reflection->ref_type = PHP_REF_TYPE_PENDING;

		php_inspector_table_insert(
			PHP_INSPECTOR_ROOT_PENDING, 	
			PHP_INSPECTOR_TABLE_FUNCTION, 
			function->common.function_name, object);
	} ZEND_HASH_FOREACH_END();

	php_inspector_table_drop(
		PHP_INSPECTOR_ROOT_REGISTERED,
		PHP_INSPECTOR_TABLE_FUNCTION, 
		function->common.function_name);

	return ZEND_HASH_APPLY_REMOVE;
}

static int php_inspector_function_purge(zval *zv, HashTable *filters) {
	zval *filter;
	zend_function *function = Z_PTR_P(zv);

	if (!ZEND_USER_CODE(function->type)) {
		return ZEND_HASH_APPLY_KEEP;
	}

	if (!function->common.function_name || !filters) {
		zend_hash_del(
			&EG(included_files), function->op_array.filename);
		return php_inspector_function_remove(function);;
	}

	ZEND_HASH_FOREACH_VAL(filters, filter) {
		if (Z_TYPE_P(filter) != IS_STRING || !Z_STRLEN_P(filter)) {
			continue;
		}

		if (Z_STRLEN_P(filter) <= ZSTR_LEN(function->common.function_name) &&
		   strncasecmp(
			ZSTR_VAL(function->common.function_name), 
			Z_STRVAL_P(filter), 
			ZSTR_LEN(function->common.function_name)) == SUCCESS) {
			return ZEND_HASH_APPLY_KEEP;
		}
	} ZEND_HASH_FOREACH_END();

	zend_hash_del(
		&EG(included_files), function->op_array.filename);

	return php_inspector_function_remove(function);
}

static PHP_METHOD(InspectorFunction, purge)
{
	HashTable *filters = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|H", &filters) != SUCCESS) {
		return;
	}

	zend_hash_apply_with_argument(EG(function_table), (apply_func_arg_t) php_inspector_function_purge, filters);
}

PHP_METHOD(InspectorFunction, __destruct)
{
	zend_op_array* function = 
		php_reflection_object_function(getThis());

	if (!ZEND_USER_CODE(function->type)) {
		return;
	}

	destroy_op_array(function);
}

ZEND_BEGIN_ARG_INFO_EX(InspectorFunction_purge_arginfo, 0, 0, 0)
	ZEND_ARG_TYPE_INFO(0, filter, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(InspectorFunction_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, function)
ZEND_END_ARG_INFO()

static zend_function_entry php_inspector_function_methods[] = {
	PHP_ME(InspectorFunction, __construct, InspectorFunction_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, onResolve, InspectorFunction_onResolve_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getInstruction, InspectorFunction_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getInstructionCount, InspectorFunction_getInstructionCount_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getEntryInstruction, InspectorFunction_getEntryInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, findFirstInstruction, InspectorFunction_find_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, findLastInstruction, InspectorFunction_find_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, flushInstructionCache, InspectorFunction_flush_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, __destruct, InspectorFunction_destruct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, purge, InspectorFunction_purge_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
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
