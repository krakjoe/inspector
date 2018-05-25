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

#ifndef HAVE_INSPECTOR_FILE
#define HAVE_INSPECTOR_FILE

#include "php.h"

#include "zend_exceptions.h"
#include "zend_interfaces.h"

#include "php_inspector.h"

#include "strings.h"
#include "reflection.h"
#include "instruction.h"
#include "function.h"
#include "break.h"

zend_class_entry *php_inspector_file_ce;

typedef void (*zend_execute_function_t)(zend_execute_data *);

static zend_execute_function_t zend_execute_function;

ZEND_BEGIN_MODULE_GLOBALS(inspector_file)
	HashTable pending;
ZEND_END_MODULE_GLOBALS(inspector_file)

ZEND_DECLARE_MODULE_GLOBALS(inspector_file);

#ifdef ZTS
#define IFG(v) TSRMG(inspector_file_globals_id, zend_inspector_file_globals *, v)
#else
#define IFG(v) (inspector_file_globals.v)
#endif

static void php_inspector_file_globals_init(zend_inspector_file_globals *IFG) {
	memset(IFG, 0, sizeof(*IFG));
}

static zend_always_inline HashTable* php_inspector_file_pending(zend_string *file) {
	HashTable *pending = zend_hash_find_ptr(&IFG(pending), file);

	if (!pending) {
		ALLOC_HASHTABLE(pending);

		zend_hash_init(pending, 8, NULL, ZVAL_PTR_DTOR, 0);

		zend_hash_update_ptr(&IFG(pending), file, pending);
	}

	return pending;
}

int php_inspector_file_resolve(zval *zv, zend_function *ops) {
	php_reflection_object_t *reflector = 
		php_reflection_object_fetch(zv);
	zend_function *oldFunction = 
		php_reflection_object_function(zv);	
	zend_function *onResolve = zend_hash_find_ptr(
		&Z_OBJCE_P(zv)->function_table, PHP_INSPECTOR_STRING_ONRESOLVE);
	
	if (oldFunction && ops->op_array.opcodes == oldFunction->op_array.opcodes) {
		
		reflector->ref_type = PHP_REF_TYPE_OTHER;

		return ZEND_HASH_APPLY_REMOVE;
	}

	reflector->ptr = ops;
	reflector->ref_type = PHP_REF_TYPE_OTHER;

	php_inspector_instruction_cache_flush(zv);

	if (onResolve->type != ZEND_INTERNAL_FUNCTION) {
		zval rv;
		HashTable *registered = ops->common.function_name ?
			php_inspector_registered_function(ops->common.function_name, 1) : NULL;

		ZVAL_NULL(&rv);

		zend_call_method_with_0_params(zv, Z_OBJCE_P(zv), &onResolve, "onresolve", &rv);

		if (Z_REFCOUNTED(rv)) {
			zval_ptr_dtor(&rv);
		}

		if (registered && zend_hash_next_index_insert(registered, zv)) {
			Z_ADDREF_P(zv);
		}
	}

	if (instanceof_function(Z_OBJCE_P(zv), php_inspector_file_ce)) {
		reflector->ref_type = PHP_REF_TYPE_EXPIRED;
	}

	return ZEND_HASH_APPLY_REMOVE;
}


static zend_op_array* php_inspector_file_execute(zend_execute_data *execute_data) {
	zend_op_array *ops = &EX(func)->op_array;
	HashTable *pending = zend_hash_num_elements(&IFG(pending)) ?
		(HashTable*) 
			zend_hash_find_ptr(&IFG(pending), ops->filename) : NULL;

	if (UNEXPECTED(pending)) {
		zend_hash_apply_with_argument(
			pending, 
			(apply_func_arg_t) 
				php_inspector_file_resolve, ops);

		zend_hash_del(&IFG(pending), ops->filename);
	}

	{
		zend_string *name;

		ZEND_HASH_FOREACH_STR_KEY_PTR(&PIG(pending).function, name, pending) {
			zend_function *function = 
				(zend_function*)
					zend_hash_find_ptr(EG(function_table), name);

			if (!function) {
				continue;
			}

			if (ZEND_HASH_GET_APPLY_COUNT(pending) == 0) {
				ZEND_HASH_INC_APPLY_COUNT(pending);

				zend_hash_apply_with_argument(
					pending, 
					(apply_func_arg_t) 
						php_inspector_file_resolve, (zend_op_array*) function);

				ZEND_HASH_DEC_APPLY_COUNT(pending);

				zend_hash_del(&PIG(pending).function, name);
			}			
		} ZEND_HASH_FOREACH_END();
	}

zend_execute_function_forward:
	zend_execute_function(execute_data);
}

zend_bool php_inspector_file_executing(zend_string *file, zend_execute_data *frame) {
	while (frame && (!frame->func || !ZEND_USER_CODE(frame->func->type))) {
		frame = frame->prev_execute_data;
	}

	if (frame) {
		return zend_string_equals(frame->func->op_array.filename, file);
	}
	
	return 0;
}

static PHP_METHOD(InspectorFile, __construct)
{
	php_reflection_object_t *reflection = php_reflection_object_fetch(getThis());
	zend_string *file = NULL;
	HashTable *pending;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &file) != SUCCESS) {
		return;
	}

	if (php_inspector_file_executing(file, execute_data)) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"cannot inspect currently executing file");
		return;
	}

	pending = php_inspector_file_pending(file);

	if (zend_hash_next_index_insert(pending, getThis())) {
		reflection->ref_type = PHP_REF_TYPE_PENDING;

		Z_ADDREF_P(getThis());
	}
}

static PHP_METHOD(InspectorFile, isPending)
{
	php_reflection_object_t *reflector =
		php_reflection_object_fetch(getThis());

	RETURN_BOOL(reflector->ref_type == PHP_REF_TYPE_PENDING);
}

static PHP_METHOD(InspectorFile, isExpired)
{
	php_reflection_object_t *reflector =
		php_reflection_object_fetch(getThis());

	RETURN_BOOL(reflector->ref_type == PHP_REF_TYPE_EXPIRED);
}

ZEND_BEGIN_ARG_INFO_EX(InspectorFile_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, file)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFile_state_arginfo, 0, 0, _IS_BOOL, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFile_state_arginfo, 0, 0, _IS_BOOL, NULL, 0)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(InspectorFile_stateChanged_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_inspector_file_methods[] = {
	PHP_ME(InspectorFile, __construct, InspectorFile_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFile, isPending, InspectorFile_state_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFile, isExpired, InspectorFile_state_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_file) 
{
	zend_class_entry ce;

	ZEND_INIT_MODULE_GLOBALS(inspector_file, php_inspector_file_globals_init, NULL);

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorFile", php_inspector_file_methods);

	php_inspector_file_ce = zend_register_internal_class_ex(&ce, php_inspector_function_ce);

	zend_execute_function = zend_execute_ex;
	zend_execute_ex = php_inspector_file_execute;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(inspector_file)
{
	zend_execute_ex = zend_execute_function;

	return SUCCESS;
}

/* {{{ */
static void php_inspector_file_table_free(zval *zv) {
	zend_hash_destroy(Z_PTR_P(zv));
	efree(Z_PTR_P(zv));
} /* }}} */

PHP_RINIT_FUNCTION(inspector_file)
{
	zend_hash_init(&IFG(pending), 8, NULL, php_inspector_file_table_free, 0);

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(inspector_file)
{
	zend_hash_destroy(&IFG(pending));

	return SUCCESS;
}
#endif
