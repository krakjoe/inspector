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
#include "zend_closures.h"
#include "zend_exceptions.h"
#include "zend_extensions.h"
#include "zend_interfaces.h"
#include "zend_vm.h"

#include "php_inspector.h"

#include "strings.h"
#include "reflection.h"
#include "function.h"
#include "method.h"
#include "instruction.h"
#include "break.h"
#include "map.h"

zend_class_entry *php_inspector_function_ce;
zend_object_handlers php_inspector_function_handlers;
extern zend_class_entry *php_inspector_file_ce;

static int php_inspector_function_trace_id;

zend_object* php_inspector_trace_fetch(zend_function *function) {
	return function->op_array.reserved[php_inspector_function_trace_id];
}

zend_object* php_inspector_function_create(zend_class_entry *type) {
	php_inspector_function_t *function = (php_inspector_function_t*)
		ecalloc(1, sizeof(php_inspector_function_t) + zend_object_properties_size(type));

	zend_object_std_init(&function->std, type);

	function->std.handlers = &php_inspector_function_handlers;

	return &function->std;
}

void php_inspector_function_free(zend_object *zo) {
	php_inspector_function_t *function = php_inspector_function_fetch(zo);

	if (function->name) {
		zend_string_release(function->name);
		zend_string_release(function->key);
	}

	if (!Z_ISUNDEF(function->reflector)) {
		zval_ptr_dtor(&function->reflector);
	}

	if (!Z_ISUNDEF(function->cache)) {
		zval_ptr_dtor(&function->cache);
	}

	zend_object_std_dtor(zo);
}

static zend_always_inline zend_bool php_inspector_function_guard(zval *object) {
	php_inspector_function_t *function =
		php_inspector_function_from(object);

	if (!php_inspector_reflection_guard(object)) {
		return 0;
	}

	if (!ZEND_USER_CODE(function->function->type)) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"InspectorInstructionInterface doesn't work on internal functions");
		return 0;
	}

	return 1;
}

void php_inspector_function_factory(zend_function *function, zval *return_value, zend_bool init, zend_bool map) {
	php_inspector_function_t *abstract;

	if (init) {
		if (function->common.scope) {
			object_init_ex(return_value, php_inspector_method_ce);
		} else {
			object_init_ex(return_value, php_inspector_function_ce);
		}
	}

	abstract = php_inspector_function_from(return_value);

	if (!map) {
		if (php_inspector_map_fetch((zend_op_array*)function)) {
			abstract->function = php_inspector_map_fetch((zend_op_array*)function);
		} else  abstract->function = function;
	} else {
		abstract->function =  php_inspector_map_create((zend_op_array*) function);
	}
}

PHP_METHOD(InspectorFunction, __construct)
{
	php_inspector_function_t *function = php_inspector_function_from(getThis());
	zend_string *name = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &name) != SUCCESS) {
		return;
	}

	function->name = zend_string_copy(name);
	function->key  = zend_string_tolower(function->name);

	if (!(function->function = zend_hash_find_ptr(EG(function_table), function->key))) {
		php_inspector_table_insert(
			PHP_INSPECTOR_ROOT_PENDING, 
			PHP_INSPECTOR_TABLE_FUNCTION, 
			function->name, getThis());
		return;
	}

	php_inspector_function_resolve(getThis(), function->function);
}

PHP_METHOD(InspectorFunction, onResolve)
{
	
}

PHP_METHOD(InspectorFunction, onTrace)
{
	
}


PHP_METHOD(InspectorFunction, getInstruction)
{
	php_inspector_function_t *function = php_inspector_function_from(getThis());
	zend_long num = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|l", &num) != SUCCESS) {
		return;
	}

	if (!php_inspector_function_guard(getThis())) {
		return;
	}


	if (num < 0) {
		num += function->function->op_array.last - 1;
	}

	if (num < 0 || num > function->function->op_array.last) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"instruction %d is out of bounds", num);
		return;
	}

	php_inspector_instruction_factory(getThis(), &function->function->op_array.opcodes[num], return_value);
}

PHP_METHOD(InspectorFunction, getInstructionCount)
{
	php_inspector_function_t *function = php_inspector_function_from(getThis());

	if (!php_inspector_function_guard(getThis())) {
		return;
	}

	RETURN_LONG(function->function->op_array.last - 1);
}

PHP_METHOD(InspectorFunction, getEntryInstruction)
{
	php_inspector_function_t *function = php_inspector_function_from(getThis());
	zend_op *op;

	if (!php_inspector_function_guard(getThis())) {
		return;
	}

	op = function->function->op_array.opcodes + function->function->op_array.num_args;

	if (op < function->function->op_array.opcodes + (function->function->op_array.last - 1)) {
		php_inspector_instruction_factory(getThis(), op, return_value);
		return;
	}

	zend_throw_exception_ex(reflection_exception_ptr, 0, "function has no entry point");
}

PHP_METHOD(InspectorFunction, findFirstInstruction)
{
	php_inspector_function_t *function = php_inspector_function_from(getThis());
	zend_long offset = 0;
	zend_long opcode = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "l|l", &opcode, &offset) != SUCCESS) {
		return;
	}

	if (!php_inspector_function_guard(getThis())) {
		return;
	}

	if (offset < 0) {
		offset += function->function->op_array.last - 1;
	}

	if (offset < 0 || offset > function->function->op_array.last) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"offset %d is out of bounds", offset);
		return;
	}

	{
		zend_op *op = &function->function->op_array.opcodes[offset],
			*end = function->function->op_array.opcodes + function->function->op_array.last;
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
	php_inspector_function_t *function = php_inspector_function_from(getThis());
	zend_long offset = 0;
	zend_long opcode = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "l|l", &opcode, &offset) != SUCCESS) {
		return;
	}

	if (!php_inspector_function_guard(getThis())) {
		return;
	}

	if (offset <= 0) {
		offset += function->function->op_array.last - 1;
	}

	if (offset < 0 || offset > function->function->op_array.last) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"offset %d is out of bounds", offset);
		return;
	}

	{
		zend_op *op = &function->function->op_array.opcodes[offset],
			*end = function->function->op_array.opcodes;
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
	php_inspector_instruction_cache_flush(getThis(), return_value);
}

int php_inspector_function_resolve(zval *zv, zend_function *ops) {
	php_inspector_function_t *function = php_inspector_function_from(zv);
	zend_function *onResolve = zend_hash_find_ptr(
		&Z_OBJCE_P(zv)->function_table, PHP_INSPECTOR_STRING_ONRESOLVE);
	zend_function *onTrace = zend_hash_find_ptr(
		&Z_OBJCE_P(zv)->function_table, PHP_INSPECTOR_STRING_ONTRACE);

	if (function->function) {
		php_inspector_breaks_purge(function->function);
	}

	php_inspector_instruction_cache_flush(zv, NULL);

	function->function = (zend_function*) php_inspector_map_create((zend_op_array*) ops);

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

	if (ZEND_USER_CODE(onTrace->type)) {
		/* cannot addref, maintain your own ref ... dark wizard ... */
		function->function->op_array.reserved[php_inspector_function_trace_id] = Z_OBJ_P(zv);
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
		php_inspector_function_t *r = 
			php_inspector_function_from(object);

		r->function = NULL;

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

	if (function->common.fn_flags & ZEND_ACC_CLOSURE) {
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
			Z_STRLEN_P(filter)) == SUCCESS) {
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

	zend_hash_apply_with_argument(CG(function_table), (apply_func_arg_t) php_inspector_function_purge, filters);
}

PHP_METHOD(InspectorFunction, __call)
{
	php_inspector_function_t *function = php_inspector_function_from(getThis());
	zend_string *method = NULL;
	zend_string *key    = NULL;
	zval        *args   = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "Sa", &method, &args) != SUCCESS) {
		return;
	}

	if (Z_ISUNDEF(function->reflector)) {
		zval arg;
		zval rv;

		ZVAL_STR(&arg, function->name);

		object_init_ex(&function->reflector, reflection_function_ptr);

		zend_call_method_with_1_params(
			&function->reflector, 
			reflection_function_ptr, 
			&reflection_function_ptr->constructor, "__construct", 
			&rv, &arg);
	}

	php_inspector_reflector_call(&function->reflector, method, args, return_value);
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
	PHP_ME(InspectorFunction, onTrace, InspectorFunction_onTrace_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getInstruction, InspectorFunction_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getInstructionCount, InspectorFunction_getInstructionCount_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, getEntryInstruction, InspectorFunction_getEntryInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, findFirstInstruction, InspectorFunction_find_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, findLastInstruction, InspectorFunction_find_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, flushInstructionCache, InspectorFunction_flush_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFunction, purge, InspectorFunction_purge_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(InspectorFunction, __call, InspectorReflector_call_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_function) {
	zend_class_entry ce;
	zend_extension dummy;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorFunction", php_inspector_function_methods);

	php_inspector_function_ce = zend_register_internal_class(&ce);
	php_inspector_function_ce->create_object = php_inspector_function_create;

	zend_class_implements(php_inspector_function_ce, 1, php_inspector_instruction_interface_ce);

	zend_declare_property_null(
		php_inspector_function_ce, 
		ZEND_STRL("instructionCache"), ZEND_ACC_PROTECTED);

	memcpy(&php_inspector_function_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	php_inspector_function_handlers.offset = XtOffsetOf(php_inspector_function_t, std);
	php_inspector_function_handlers.free_obj = php_inspector_function_free;

	php_inspector_function_trace_id = zend_get_resource_handle(&dummy);	

	if (php_inspector_function_trace_id < 0) {
		return FAILURE;
	}

	return SUCCESS;
}  /* }}} */
#endif
