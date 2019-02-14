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
#include "zend_interfaces.h"

#include "strings.h"
#include "reflection.h"
#include "class.h"
#include "function.h"
#include "instruction.h"
#include "break.h"

zend_class_entry *php_inspector_class_ce;
zend_object_handlers php_inspector_class_handlers;

zend_object* php_inspector_class_create(zend_class_entry *type) {
	php_inspector_class_t *class = (php_inspector_class_t*)
		ecalloc(1, sizeof(php_inspector_class_t) + zend_object_properties_size(type));

	zend_object_std_init(&class->std, type);

	class->std.handlers = &php_inspector_class_handlers;

	return &class->std;
}

int php_inspector_class_clean(zval *zv) {
	zend_function *function = (zend_function*) Z_PTR_P(zv);

	if (!ZEND_USER_CODE(function->type)) {
		return ZEND_HASH_APPLY_KEEP;
	}

	php_inspector_breaks_purge(function);

	return ZEND_HASH_APPLY_KEEP;	
}

static zend_always_inline void php_inspector_class_destroy(zend_class_entry *ce) {
	zval tmp;
	
	ZVAL_PTR(&tmp, ce);

	zend_hash_apply(&ce->function_table, php_inspector_class_clean);

	destroy_zend_class(&tmp);
}

void php_inspector_class_free(zend_object *zo) {
	php_inspector_class_t *class = php_inspector_class_fetch(zo);

	if (!Z_ISUNDEF(class->reflector)) {
		zval_ptr_dtor(&class->reflector);
	}

	//php_inspector_class_destroy(class->ce);

	zend_string_release(class->name);
	zend_string_release(class->key);

	zend_object_std_dtor(zo);
}

static zend_always_inline zend_bool php_inspector_class_guard(zval *object) {
	if (!php_inspector_reflection_guard(object)) {
		return 0;
	}

	return 1;
}

/* {{{ */
void php_inspector_class_factory(zend_class_entry *ce, zval *return_value) {
	php_inspector_class_t *class;

	object_init_ex(return_value, php_inspector_class_ce);

	class         = php_inspector_class_from(return_value);
	class->name   = zend_string_copy(ce->name);
	class->key    = zend_string_tolower(class->name);
	class->ce     = ce;
} /* }}} */

/* {{{ */
zend_function* php_inspector_method_find(php_inspector_class_t *class, zend_string *name) {
	zend_string *lower = zend_string_tolower(name);

	zend_function *found = 
		(zend_function*)
			zend_hash_find_ptr(
				&class->ce->function_table, lower);

	zend_string_release(lower);
	return found;
} /* }}} */

/* {{{ */
static PHP_METHOD(InspectorClass, __construct)
{
	php_inspector_class_t *class = php_inspector_class_from(getThis());
	zend_string *name = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &name) != SUCCESS) {
		return;
	}

	class->name = zend_string_copy(name);
	class->key  = zend_string_tolower(class->name);

	if (!(class->ce = zend_hash_find_ptr(CG(class_table), class->key))) {
		php_inspector_table_insert(
			PHP_INSPECTOR_ROOT_PENDING, 
			PHP_INSPECTOR_TABLE_CLASS,
			class->name, getThis());
		return;
	}
}

static PHP_METHOD(InspectorClass, getMethod) {
	php_inspector_class_t *class = php_inspector_class_from(getThis());
	zend_string *name = NULL;
	zend_function *function = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &name) != SUCCESS) {
		return;
	}

	/*
	if (!php_inspector_class_guard(getThis())) {
		return;
	}
	*/

	if (!(function = php_inspector_method_find(class, name))) {
		zend_throw_exception_ex(reflection_exception_ptr, 0,
			"Method %s does not exist", ZSTR_VAL(name));
		return;
	}

	php_inspector_function_factory(function, return_value, 1, 1);
}

static PHP_METHOD(InspectorClass, getMethods)
{
	php_inspector_class_t *class = php_inspector_class_from(getThis());
	zend_long filter = ZEND_ACC_PPP_MASK | ZEND_ACC_ABSTRACT | ZEND_ACC_FINAL | ZEND_ACC_STATIC;
	zend_string *name = NULL;
	zend_function *function = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|l", &filter) != SUCCESS) {
		return;
	}

	/*
	if (!php_inspector_class_guard(getThis())) {
		return;
	}
	*/

	array_init(return_value);

	ZEND_HASH_FOREACH_STR_KEY_PTR(&class->ce->function_table, name, function) {
		if (function->common.fn_flags & filter) {
			zval inspector;

			php_inspector_function_factory(function, &inspector, 1, 1);

			zend_hash_add(
				Z_ARRVAL_P(return_value), name, &inspector);
		}
	} ZEND_HASH_FOREACH_END();
}

int php_inspector_class_resolve(zval *zv, zend_class_entry *ce) {
	php_inspector_class_t *class = php_inspector_class_from(zv);
	zend_function *onResolve = (zend_function*) zend_hash_find_ptr(
		&Z_OBJCE_P(zv)->function_table, PHP_INSPECTOR_STRING_ONRESOLVE);

	if (class->ce) {
		php_inspector_class_destroy(class->ce);
	}

	class->ce = ce;

	if (ZEND_USER_CODE(onResolve->type)) {
		zval rv;

		ZVAL_NULL(&rv);

		zend_call_method_with_0_params(zv, Z_OBJCE_P(zv), &onResolve, "onresolve", &rv);

		php_inspector_table_insert(
			PHP_INSPECTOR_ROOT_REGISTERED,
			PHP_INSPECTOR_TABLE_CLASS,
			class->name, zv);

		if (Z_REFCOUNTED(rv)) {
			zval_ptr_dtor(&rv);
		}
	}

	return ZEND_HASH_APPLY_REMOVE;
}

static int php_inspector_class_remove(zend_class_entry *class) {
	HashTable *registered = php_inspector_table(
		PHP_INSPECTOR_ROOT_REGISTERED, 
		PHP_INSPECTOR_TABLE_CLASS, 
		class->name, 0);
	zval *object;

	if (!registered) {
		return ZEND_HASH_APPLY_REMOVE;
	}

	ZEND_HASH_FOREACH_VAL(registered, object) {
		php_inspector_class_t *r =
			php_inspector_class_from(object);

		r->ce = NULL;

		php_inspector_table_insert(
			PHP_INSPECTOR_ROOT_PENDING,
			PHP_INSPECTOR_TABLE_CLASS, 
			class->name, object);
	} ZEND_HASH_FOREACH_END();

	php_inspector_table_drop(
		PHP_INSPECTOR_ROOT_REGISTERED,
		PHP_INSPECTOR_TABLE_CLASS, 
		class->name);

	return ZEND_HASH_APPLY_REMOVE;
}

static int php_inspector_class_purge(zval *zv, HashTable *filters) {
	zval *filter;
	zend_class_entry *ce = Z_PTR_P(zv);

	if (ce->type != ZEND_USER_CLASS || instanceof_function(ce, reflector_ptr)) {
		return ZEND_HASH_APPLY_KEEP;
	}

	if (!filters) {
		zend_hash_del(
			&EG(included_files), ce->info.user.filename);

		return php_inspector_class_remove(ce);
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

	return php_inspector_class_remove(ce);
}

static PHP_METHOD(InspectorClass, purge)
{
	HashTable *filters = NULL;

	zend_parse_parameters_throw(
		ZEND_NUM_ARGS(), "|H", &filters);

	zend_hash_apply_with_argument(CG(class_table), (apply_func_arg_t) php_inspector_class_purge, filters);
}

static PHP_METHOD(InspectorClass, onResolve) {}

static PHP_METHOD(InspectorClass, __call)
{
	php_inspector_class_t *class = php_inspector_class_from(getThis());
	zend_string *method = NULL;
	zend_string *key    = NULL;
	zval        *args   = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "Sa", &method, &args) != SUCCESS) {
		return;
	}

	if (Z_ISUNDEF(class->reflector)) {
		zval arg;
		zval rv;

		ZVAL_STR(&arg, class->name);

		object_init_ex(
			&class->reflector, reflection_class_ptr);

		zend_call_method_with_1_params(
			&class->reflector, 
			reflection_class_ptr, 
			&reflection_class_ptr->constructor, "__construct", 
			&rv, &arg);
	}

	php_inspector_reflector_call(&class->reflector, method, args, return_value);
}

/* {{{ */
ZEND_BEGIN_ARG_INFO_EX(InspectorClass_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, class)
ZEND_END_ARG_INFO() /* }}} */

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
ZEND_BEGIN_ARG_INFO_EX(InspectorClass_onResolve_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO() /* }}} */

/* {{{ */
static zend_function_entry php_inspector_class_methods[] = {
	PHP_ME(InspectorClass, __construct, InspectorClass_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorClass, onResolve, InspectorClass_onResolve_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorClass, getMethod, InspectorClass_getMethod_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorClass, getMethods, InspectorClass_getMethods_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorClass, purge, InspectorClass_purge_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(InspectorClass, __call, InspectorReflector_call_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(inspector_class) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorClass", php_inspector_class_methods);
	php_inspector_class_ce = zend_register_internal_class(&ce);
	php_inspector_class_ce->create_object = php_inspector_class_create;

	memcpy(&php_inspector_class_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	php_inspector_class_handlers.offset = XtOffsetOf(php_inspector_class_t, std);
	php_inspector_class_handlers.free_obj = php_inspector_class_free;

	return SUCCESS;
} /* }}} */
#endif
