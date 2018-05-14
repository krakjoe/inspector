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
#ifndef HAVE_INSPECTOR_FILE
#define HAVE_INSPECTOR_FILE

#include "php.h"
#include "php_main.h"

#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"

#include "scope.h"
#include "func.h"
#include "entry.h"

zend_class_entry *php_inspector_file_ce;

/* {{{ */
static PHP_METHOD(File, __construct)
{
	zend_string *filename = NULL;
	zend_file_handle fh;
	zend_op_array *ops = NULL;
	zend_bool clobber = 0;
	HashTable *tables[2];
	zval values[1];

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S|b", &filename, &clobber) != SUCCESS) {
		return;
	}

	if (zend_hash_exists(&EG(included_files), filename) && clobber) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot load already included file %s",
			ZSTR_VAL(filename));
		return;
	}

	if (php_stream_open_for_zend_ex(ZSTR_VAL(filename), &fh, USE_PATH|STREAM_OPEN_FOR_INCLUDE) != SUCCESS) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot open %s for parsing",
			ZSTR_VAL(filename));
		return;
	}

	if (!fh.opened_path) {
		fh.opened_path = zend_string_copy(filename);	
	}

	if (!zend_hash_add_empty_element(&EG(included_files), fh.opened_path) && clobber) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot add %s to included files table",
			ZSTR_VAL(filename));
		zend_file_handle_dtor(&fh);
		return;
	}

	if (!clobber) {
		php_inspector_scope_save(getThis(), tables, values);
	}

	ops = zend_compile_file(&fh, ZEND_REQUIRE);

	if (!clobber) {
		php_inspector_scope_restore(getThis(), tables, values);
	}

	php_inspector_scope_construct(getThis(), (zend_function*) ops);
	zend_destroy_file_handle(&fh);
	destroy_op_array(ops);
	efree(ops);
}

/* {{{ proto array File::getGlobals(void) */
PHP_METHOD(File, getGlobals)
{
	zend_function *function;
	php_inspector_scope_t *scope = php_inspector_scope_fetch(getThis());

	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

	if (scope->symbols.functions.pDestructor) {
	
		array_init(return_value);

		ZEND_HASH_FOREACH_PTR(&scope->symbols.functions, function) {
			zval o;

			if (function->type != ZEND_USER_FUNCTION) {
				continue;
			}

			object_init_ex(&o, php_inspector_func_ce);

			php_inspector_scope_construct(&o, function);

			zend_hash_add(Z_ARRVAL_P(return_value), function->common.function_name, &o);
		} ZEND_HASH_FOREACH_END();
	}
} /* }}} */

/* {{{ proto array File::getEntries(void) */
PHP_METHOD(File, getEntries)
{
	zend_class_entry *entry;
	php_inspector_scope_t *scope = php_inspector_scope_fetch(getThis());

	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}

	if (scope->symbols.classes.pDestructor) {
	
		array_init(return_value);

		ZEND_HASH_FOREACH_PTR(&scope->symbols.classes, entry) {
			zval o;

			if (entry->type != ZEND_USER_CLASS) {
				continue;
			}

			object_init_ex(&o, php_inspector_entry_ce);

			php_inspector_entry_construct(&o, entry);

			zend_hash_add(
				Z_ARRVAL_P(return_value), entry->name, &o);
		} ZEND_HASH_FOREACH_END();
	}
} /* }}} */

/* {{{ proto Global File::getGlobal(string name); */
PHP_METHOD(File, getGlobal) {
	php_inspector_scope_t *scope = php_inspector_scope_fetch(getThis());
	zend_string *name = NULL;
	zend_function *function = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) != SUCCESS) {
		return;
	}

	if (!scope->symbols.functions.pDestructor) {
		return;
	}

	name = zend_string_tolower(name);

	function = zend_hash_find_ptr(&scope->symbols.functions, name);

	if (function) {
		object_init_ex(return_value, php_inspector_func_ce);

		php_inspector_scope_construct(return_value, function);
	}

	zend_string_release(name);
} /* }}} */

/* {{{ proto Entry File::getEntry(string name); */
PHP_METHOD(File, getEntry) {
	php_inspector_scope_t *scope = php_inspector_scope_fetch(getThis());
	zend_string *name = NULL;
	zend_class_entry *entry = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) != SUCCESS) {
		return;
	}

	if (!scope->symbols.classes.pDestructor) {
		return;
	}

	name = zend_string_tolower(name);

	entry = zend_hash_find_ptr(&scope->symbols.classes, name);

	if (entry) {
		object_init_ex(return_value, php_inspector_entry_ce);

		php_inspector_entry_construct(return_value, entry);
	}

	zend_string_release(name);
} /* }}} */

ZEND_BEGIN_ARG_INFO_EX(File_construct_arginfo, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, clobber, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(File_getArray_arginfo, 0, 0, IS_ARRAY, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(File_getArray_arginfo, 0, 0, IS_ARRAY, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(File_getGlobal_arginfo, 0, 0, Inspector\\Global, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(File_getGlobal_arginfo, 0, 0, IS_OBJECT, "Inspector\\Global", 0)
#endif
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(File_getEntry_arginfo, 0, 0, Inspector\\Entry, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(File_getEntry_arginfo, 0, 0, IS_OBJECT, "Inspector\\Entry", 0)
#endif
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_inspector_file_methods[] = {
	PHP_ME(File, __construct, File_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(File, getEntries, File_getArray_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(File, getGlobals, File_getArray_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(File, getGlobal, File_getGlobal_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(File, getEntry, File_getEntry_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_file) {
	zend_class_entry ce;
	
	INIT_NS_CLASS_ENTRY(ce, "Inspector", "File", php_inspector_file_methods);
	php_inspector_file_ce = 
		zend_register_internal_class_ex(&ce, php_inspector_scope_ce);
	php_inspector_file_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
} /* }}} */
#endif
