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

#include "zend_interfaces.h"

#include "php_inspector.h"

#include "reflection.h"
#include "instruction.h"
#include "function.h"
#include "break.h"

zend_class_entry *php_inspector_file_ce;

typedef zend_op_array* (*zend_compile_file_function_t) (zend_file_handle *fh, int type);

static zend_compile_file_function_t zend_compile_file_function;

ZEND_BEGIN_MODULE_GLOBALS(inspector_file)
	HashTable files;
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

zend_function* php_inspector_file_source(zend_string *file) {
	return zend_hash_find_ptr(&IFG(files), file);
}

static zend_always_inline HashTable* php_inspector_file_pending_list(zend_string *file) {
	HashTable *pending = zend_hash_find_ptr(&IFG(pending), file);

	if (!pending) {
		ALLOC_HASHTABLE(pending);

		zend_hash_init(pending, 8, NULL, ZVAL_PTR_DTOR, 0);

		zend_hash_update_ptr(&IFG(pending), file, pending);
	}

	return pending;
}

void php_inspector_file_pending(zend_string *file, zval *function) {
	php_reflection_object_t *reflector = 
		php_reflection_object_fetch(function);
	HashTable *pending = php_inspector_file_pending_list(file);

	reflector->ref_type = PHP_REF_TYPE_PENDING;

	if (zend_hash_next_index_insert(pending, function)) {
		Z_ADDREF_P(function);
	}
}

int php_inspector_file_resolve(zval *zv, zend_function *ops) {
	php_reflection_object_t *reflector = 
		php_reflection_object_fetch(zv);
	zend_function *onResolve = zend_hash_str_find_ptr(
		&Z_OBJCE_P(zv)->function_table, ZEND_STRL("onresolve"));

	reflector->ptr = ops;
	reflector->ref_type = PHP_REF_TYPE_OTHER;

	if (onResolve->type != ZEND_INTERNAL_FUNCTION) {
		zval rv;

		ZVAL_NULL(&rv);

		zend_call_method_with_0_params(zv, Z_OBJCE_P(zv), &onResolve, "onresolve", &rv);

		if (Z_REFCOUNTED(rv)) {
			zval_ptr_dtor(&rv);
		}	
	}

	return ZEND_HASH_APPLY_REMOVE;
}

static zend_op_array* php_inspector_file_compile(zend_file_handle *handle, int type) {
	zend_op_array *ops = zend_compile_file_function(handle, type);
	HashTable     *pending;
	zend_op_array *cache;

	if (!ops) {
		return NULL;
	}

	cache = (zend_op_array*) ecalloc(1, sizeof(zend_op_array));

	memcpy(cache, ops, sizeof(zend_op_array));

	cache->refcount = emalloc(sizeof(uint32_t));
	*(cache->refcount) = 1;

	if (!zend_hash_update_ptr(&IFG(files), ops->filename, cache)) {
		efree(cache->refcount);
		efree(cache);

		return ops;
	}

	pending = zend_hash_find_ptr(&IFG(pending), ops->filename);

	if (pending) {
		zend_hash_apply_with_argument(
			pending, 
			(apply_func_arg_t) 
				php_inspector_file_resolve, cache);
	}

	return ops;
}


static PHP_METHOD(InspectorFile, __construct)
{
	php_reflection_object_t *reflection = php_reflection_object_fetch(getThis());
	zend_string *file = NULL;
	zend_function *source;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &file) != SUCCESS) {
		return;
	}

	source = php_inspector_file_source(file);

	if (!source) {
		php_inspector_file_pending(file, getThis());
		return;
	}

	php_inspector_file_resolve(getThis(), source);
}

static PHP_METHOD(InspectorFile, onResolve) {}

static PHP_METHOD(InspectorFile, isPending)
{
	php_reflection_object_t *reflector =
		php_reflection_object_fetch(getThis());

	RETURN_BOOL(reflector->ref_type == PHP_REF_TYPE_PENDING);
}

ZEND_BEGIN_ARG_INFO_EX(InspectorFile_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, file)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFile_isPending_arginfo, 0, 0, _IS_BOOL, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFile_isPending_arginfo, 0, 0, _IS_BOOL, NULL, 0)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(InspectorFile_onResolve_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_inspector_file_methods[] = {
	PHP_ME(InspectorFile, __construct, InspectorFile_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFile, isPending, InspectorFile_isPending_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFile, onResolve, InspectorFile_onResolve_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_file) 
{
	zend_class_entry ce;

	ZEND_INIT_MODULE_GLOBALS(inspector_file, php_inspector_file_globals_init, NULL);

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorFile", php_inspector_file_methods);

	php_inspector_file_ce = zend_register_internal_class_ex(&ce, php_inspector_function_ce);

	zend_compile_file_function = zend_compile_file;
	zend_compile_file = php_inspector_file_compile;

	if (!zend_compile_file_function) {
		zend_compile_file_function = compile_file;
	}

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(inspector_file)
{
	if (zend_compile_file_function != compile_file) {
		zend_compile_file = zend_compile_file_function;
	} else {
		zend_compile_file = NULL;
	}
}

/* {{{ */
static void php_inspector_file_free(zval *zv) {
	zend_op_array *ops = Z_PTR_P(zv);

	efree(ops->refcount);
	efree(ops);
} /* }}} */

/* {{{ */
static void php_inspector_file_pending_free(zval *zv) {
	zend_hash_destroy(Z_PTR_P(zv));
	efree(Z_PTR_P(zv));
} /* }}} */

PHP_RINIT_FUNCTION(inspector_file)
{
	zend_hash_init(&IFG(files), 8, NULL, php_inspector_file_free, 0);
	zend_hash_init(&IFG(pending), 8, NULL, php_inspector_file_pending_free, 0);

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(inspector_file)
{
	zend_hash_destroy(&IFG(files));
	zend_hash_destroy(&IFG(pending));

	return SUCCESS;
}
#endif
