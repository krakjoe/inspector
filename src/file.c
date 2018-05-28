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

static zend_always_inline zend_bool php_inspector_file_executing(zend_string *file, zend_execute_data *frame) {
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

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &file) != SUCCESS) {
		return;
	}

	if (php_inspector_file_executing(file, execute_data)) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"cannot inspect currently executing file");
		return;
	}

	reflection->ref_type = PHP_REF_TYPE_PENDING;

	php_inspector_table_insert(
		PHP_INSPECTOR_ROOT_PENDING, 
		PHP_INSPECTOR_TABLE_FILE, 
		file, getThis());
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

static PHP_METHOD(InspectorFile, purge)
{
	HashTable *filters = NULL;
	zend_string *included;
	
	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|H", &filters) != SUCCESS) {
		return;
	}

	ZEND_HASH_FOREACH_STR_KEY(&EG(included_files), included) {
		HashTable *registered = php_inspector_table(
			PHP_INSPECTOR_ROOT_REGISTERED, 
			PHP_INSPECTOR_TABLE_FILE, 
			included, 0);
		zval *object;

		if (!registered) {
			zend_hash_del(
				&EG(included_files), included);
			continue;
		}

		ZEND_HASH_FOREACH_VAL(registered, object) {
			php_reflection_object_t *reflection =
				php_reflection_object_fetch(object);
		
			reflection->ref_type = PHP_REF_TYPE_PENDING;

			php_inspector_table_insert(
				PHP_INSPECTOR_ROOT_PENDING, 	
				PHP_INSPECTOR_TABLE_FILE, 
				included, object);
		} ZEND_HASH_FOREACH_END();

		php_inspector_table_drop(
			PHP_INSPECTOR_ROOT_REGISTERED,
			PHP_INSPECTOR_TABLE_FUNCTION, 
			included);

		zend_hash_del(&EG(included_files), included);
	} ZEND_HASH_FOREACH_END();
}

ZEND_BEGIN_ARG_INFO_EX(InspectorFile_purge_arginfo, 0, 0, 0)
	ZEND_ARG_TYPE_INFO(0, filter, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

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
	PHP_ME(InspectorFile, purge, InspectorFile_purge_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_file) 
{
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorFile", php_inspector_file_methods);

	php_inspector_file_ce = zend_register_internal_class_ex(&ce, php_inspector_function_ce);

	return SUCCESS;
}
#endif
