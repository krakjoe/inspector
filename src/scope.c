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
#ifndef HAVE_INSPECTOR_SCOPE
#define HAVE_INSPECTOR_SCOPE

#include "php.h"
#include "zend_interfaces.h"

#include "ext/spl/spl_exceptions.h"
#include "php_inspector.h"

#include "scope.h"
#include "iterator.h"

zend_object_handlers php_inspector_scope_handlers;
zend_class_entry *php_inspector_scope_ce;

/* {{{ */
zend_object* php_inspector_scope_create(zend_class_entry *ce) {
	php_inspector_scope_t *scope = ecalloc(1, sizeof(php_inspector_scope_t) + zend_object_properties_size(ce));

	zend_object_std_init(&scope->std, ce);	
	object_properties_init(&scope->std, ce);

	scope->std.handlers = &php_inspector_scope_handlers;	

	return &scope->std;
} /* }}} */

/* {{{ */
zend_function* php_inspector_scope_find(zend_class_entry *scope, zend_string *name) {
	zend_string *lower = zend_string_tolower(name);
	zend_function *found = zend_hash_find_ptr(
		scope ? 
			&scope->function_table : 
			CG(function_table), 
		lower);
	zend_string_release(lower);
	return found;
} /* }}} */

/* {{{ */
void php_inspector_scope_construct(zval *object, zend_function *function) {
	php_inspector_scope_t *scope = php_inspector_scope_fetch(object);

	if (function->type != ZEND_USER_FUNCTION) {
		if (function->common.scope) {
			zend_throw_exception_ex(
				spl_ce_RuntimeException, 0, 
				"can not inspect internal function %s::%s", 
				ZSTR_VAL(function->common.scope->name), ZSTR_VAL(function->common.function_name));
		} else {
			zend_throw_exception_ex(
				spl_ce_RuntimeException, 0, 
				"can not inspect internal function %s", 
				ZSTR_VAL(function->common.function_name));
		}
		return;
	}

	scope->ops = emalloc(sizeof(zend_op_array));
	memcpy(
		scope->ops, function, sizeof(zend_op_array));
	function_add_ref((zend_function*)scope->ops);
} /* }}} */

/* {{{ */
void php_inspector_scope_destroy(zend_object *object) {
	php_inspector_scope_t *scope = 
		php_inspector_scope_fetch_from(object);

	zend_object_std_dtor(object);

	if (scope->ops) {
		destroy_op_array(scope->ops);
		efree(scope->ops);
	}
}
/* }}} */

/* {{{ */
PHP_METHOD(Scope, getStatics) {
	php_inspector_scope_t *scope = php_inspector_scope_this();
	
	if (scope->ops->static_variables) {
		RETURN_ARR(zend_array_dup(scope->ops->static_variables));
	} else array_init(return_value);
}

PHP_METHOD(Scope, getConstants) {
	php_inspector_scope_t *scope = php_inspector_scope_this();

	array_init(return_value);

	if (scope->ops->last_literal) {
		uint32_t it = 0;

		for (it = 0; it < scope->ops->last_literal; it++) {
			if (add_next_index_zval(return_value, &scope->ops->literals[it]) == SUCCESS)
				Z_TRY_ADDREF(scope->ops->literals[it]);
		}
	}
} 

PHP_METHOD(Scope, getVariables) {
	php_inspector_scope_t *scope = php_inspector_scope_this();

	array_init(return_value);

	if (scope->ops->last_var) {
		uint32_t it = 0;
		
		for (it = 0; it < scope->ops->last_var; it++) {
			add_next_index_str(return_value, 
				zend_string_copy(scope->ops->vars[it]));
		}
	}
} /* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getArray_arginfo, 0, 0, IS_ARRAY, NULL, 0)
ZEND_END_ARG_INFO()


/* {{{ */
zend_function_entry php_inspector_scope_methods[] = {
	PHP_ME(Scope, getStatics, Scope_getArray_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getConstants, Scope_getArray_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getVariables, Scope_getArray_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(scope) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Scope", php_inspector_scope_methods);
	php_inspector_scope_ce = 
		zend_register_internal_class(&ce);
	php_inspector_scope_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	php_inspector_scope_ce->create_object = php_inspector_scope_create;
	php_inspector_scope_ce->get_iterator  = php_inspector_iterate;
	zend_class_implements(php_inspector_scope_ce, 1, zend_ce_traversable);

	memcpy(&php_inspector_scope_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_scope_handlers.offset = XtOffsetOf(php_inspector_scope_t, std);
	php_inspector_scope_handlers.free_obj = php_inspector_scope_destroy;

	return SUCCESS;
} /* }}} */
#endif
