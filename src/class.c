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

#ifndef HAVE_INSPECTOR_CLASS
#define HAVE_INSPECTOR_CLASS

#include "php.h"
#include "php_inspector.h"

#include "zend_exceptions.h"

#include "strings.h"
#include "reflection.h"
#include "class.h"
#include "function.h"

zend_class_entry *php_inspector_class_ce;

/* {{{ */
void php_inspector_class_factory(zend_class_entry *ce, zval *return_value) {
	php_reflection_object_t *reflection;

	object_init_ex(return_value, reflection_class_ptr);

	reflection = php_reflection_object_fetch(return_value);
	reflection->ptr = ce;
	reflection->ref_type = PHP_REF_TYPE_OTHER;

	{
		zval k, v;

		ZVAL_STR(&k, PHP_INSPECTOR_STRING_NAME);
		ZVAL_STR(&v, ce->name);

		zend_std_write_property(return_value, &k, &v, NULL);
	}
} /* }}} */

/* {{{ */
zend_function* php_inspector_method_find(zend_class_entry *class, zend_string *name) {
	zend_string *lower = zend_string_tolower(name);
	zend_function *found = 
		(zend_function*)
			zend_hash_find_ptr(&class->function_table, lower);
	zend_string_release(lower);
	return found;
} /* }}} */

/* {{{ */
static PHP_METHOD(InspectorClass, getMethod) {
	zend_class_entry *ce =
		php_reflection_object_class(getThis());
	zend_string *method;
	zend_function *function;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &method) != SUCCESS) {
		return;
	}

	if (!(function = php_inspector_method_find(ce, method))) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"Method %s does not exist", ZSTR_VAL(method));
		return;
	}

	php_inspector_function_factory(function, return_value);
}

static PHP_METHOD(InspectorClass, getMethods)
{
	zend_class_entry *ce = 
		php_reflection_object_class(getThis());
	zend_long filter = ZEND_ACC_PPP_MASK | ZEND_ACC_ABSTRACT | ZEND_ACC_FINAL | ZEND_ACC_STATIC;
	zend_string *name = NULL;
	zend_function *function = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|l", &filter) != SUCCESS) {
		return;
	}

	array_init(return_value);

	ZEND_HASH_FOREACH_STR_KEY_PTR(&ce->function_table, name, function) {
		if (function->common.fn_flags & filter) {
			zval inspector;

			php_inspector_function_factory(function, &inspector);

			zend_hash_add(
				Z_ARRVAL_P(return_value), name, &inspector);
		}
	} ZEND_HASH_FOREACH_END();
}

static int php_inspector_class_purge(zval *zv, HashTable *filters) {
	zval *filter;
	zend_class_entry *ce = Z_PTR_P(zv);

	if (ce->type != ZEND_USER_CLASS) {
		return ZEND_HASH_APPLY_KEEP;
	}

	if (!filters) {
		zend_hash_del(
			&EG(included_files), ce->info.user.filename);

		return ZEND_HASH_APPLY_REMOVE;
	}

	ZEND_HASH_FOREACH_VAL(filters, filter) {
		if (Z_TYPE_P(filter) != IS_STRING || !Z_STRLEN_P(filter)) {
			continue;
		}

		if (Z_STRLEN_P(filter) <= ZSTR_LEN(ce->name) &&
		   strncasecmp(
			ZSTR_VAL(ce->name),
			Z_STRVAL_P(filter),
			Z_STRLEN_P(filter)) == SUCCESS) {
			return ZEND_HASH_APPLY_KEEP;
		}
	} ZEND_HASH_FOREACH_END();

	zend_hash_del(&EG(included_files), ce->info.user.filename);

	return ZEND_HASH_APPLY_REMOVE;
}

static PHP_METHOD(InspectorClass, purge)
{
	HashTable *filters = NULL;

	zend_parse_parameters_throw(
		ZEND_NUM_ARGS(), "|H", &filters);

	zend_hash_apply_with_argument(CG(class_table), (apply_func_arg_t) php_inspector_class_purge, filters);
}
/* }}} */

/* {{{ */
ZEND_BEGIN_ARG_INFO(InspectorClass_getMethod_arginfo, 1)
	ZEND_ARG_INFO(0, method)
ZEND_END_ARG_INFO() /* }}} */

/* {{{ */
ZEND_BEGIN_ARG_INFO_EX(InspectorClass_getMethods_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, filter)
ZEND_END_ARG_INFO() /* }}} */

/* {{{ */
ZEND_BEGIN_ARG_INFO_EX(InspectorClass_purge_arginfo, 0, 0, 0)
	ZEND_ARG_TYPE_INFO(0, filter, IS_ARRAY, 0)
ZEND_END_ARG_INFO() /* }}} */

/* {{{ */
static zend_function_entry php_inspector_class_methods[] = {
	PHP_ME(InspectorClass, getMethod, InspectorClass_getMethod_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorClass, getMethods, InspectorClass_getMethods_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorClass, purge, InspectorClass_purge_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(inspector_class) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorClass", php_inspector_class_methods);
	php_inspector_class_ce = 
		zend_register_internal_class_ex(&ce, reflection_class_ptr);

	return SUCCESS;
} /* }}} */
#endif
