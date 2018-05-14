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
#ifndef HAVE_INSPECTOR_ENTRY
#define HAVE_INSPECTOR_ENTRY

#include "php.h"
#include "zend_interfaces.h"

#include "ext/spl/spl_exceptions.h"
#include "ext/spl/spl_iterators.h"
#include "zend_exceptions.h"
#include "php_inspector.h"

#include "scope.h"
#include "method.h"
#include "entry.h"

typedef struct _php_inspector_entry_iterator_t {
	zend_object_iterator zit;
	zval object;
	HashPosition position;
} php_inspector_entry_iterator_t;

static zend_object_iterator* php_inspector_entry_iterate(zend_class_entry *ce, zval *object, int by_ref);

static zend_object_handlers php_inspector_entry_handlers;
zend_class_entry *php_inspector_entry_ce;

/* {{{ */
static zend_object* php_inspector_entry_create(zend_class_entry *ce) {
	php_inspector_entry_t *entry = ecalloc(1, sizeof(php_inspector_entry_t) + zend_object_properties_size(ce));

	zend_object_std_init(&entry->std, ce);	
	object_properties_init(&entry->std, ce);

	entry->std.handlers = &php_inspector_entry_handlers;	

	return &entry->std;
} /* }}} */

/* {{{ */
void php_inspector_entry_construct(zval *object, zend_class_entry *ce) {
	php_inspector_entry_t *entry = php_inspector_entry_fetch(object);

	if (ce->type != ZEND_USER_CLASS) {
		zend_throw_exception_ex(
			spl_ce_RuntimeException, 0, 
			"can not inspect internal class %s", 
			ZSTR_VAL(ce->name));
		return;
	}

	entry->entry = ce;
} /* }}} */

/* {{{ */
static void php_inspector_entry_destroy(zend_object *object) {
	php_inspector_entry_t *entry = 
		php_inspector_entry_fetch_from(object);

	zend_object_std_dtor(object);
}
/* }}} */

/* {{{ */
static void php_inspector_entry_iterator_dtor(php_inspector_entry_iterator_t* iterator) {
	zval_ptr_dtor(&iterator->object);
	if (Z_TYPE(iterator->zit.data) != IS_UNDEF)
		zval_ptr_dtor(&iterator->zit.data);
}

static int php_inspector_entry_iterator_validate(php_inspector_entry_iterator_t* iterator) {	
	return (iterator->position != HT_INVALID_IDX) ? SUCCESS : FAILURE;
}

static zval* php_inspector_entry_iterator_current_data(php_inspector_entry_iterator_t* iterator) {
	php_inspector_entry_t *entry = 
		php_inspector_entry_fetch_from(Z_OBJ(iterator->object));
	zend_function *current;

	if (Z_TYPE(iterator->zit.data) != IS_UNDEF) {
		zval_ptr_dtor(&iterator->zit.data);
		ZVAL_UNDEF(&iterator->zit.data);
	}

	current = zend_hash_get_current_data_ptr_ex(
		&entry->entry->function_table, &iterator->position);

	if (current) {
		object_init_ex(&iterator->zit.data, php_inspector_method_ce);
		php_inspector_scope_construct(
			&iterator->zit.data, current);
	}
	
	if (Z_ISUNDEF(iterator->zit.data)) {
		return &EG(uninitialized_zval);
	}

    return &iterator->zit.data;
}

static void php_inspector_entry_iterator_current_key(php_inspector_entry_iterator_t* iterator, zval* result) { 
	php_inspector_entry_t *entry = 
		php_inspector_entry_fetch_from(Z_OBJ(iterator->object));	
	zend_hash_get_current_key_zval_ex(&entry->entry->function_table, result, &iterator->position); 
}

static void php_inspector_entry_iterator_move_forward(php_inspector_entry_iterator_t* iterator) { 
	php_inspector_entry_t *entry = 
		php_inspector_entry_fetch_from(Z_OBJ(iterator->object));
	zend_hash_move_forward_ex(&entry->entry->function_table, &iterator->position);
}
static void php_inspector_entry_iterator_rewind(php_inspector_entry_iterator_t* iterator) { 	
	php_inspector_entry_t *entry = 
		php_inspector_entry_fetch_from(Z_OBJ(iterator->object));
	zend_hash_internal_pointer_reset_ex(&entry->entry->function_table, &iterator->position);
}

static zend_object_iterator_funcs php_inspector_entry_iterator_funcs = {
    (void (*) (zend_object_iterator*)) 				php_inspector_entry_iterator_dtor,
    (int (*)(zend_object_iterator *)) 				php_inspector_entry_iterator_validate,
    (zval* (*)(zend_object_iterator *)) 			php_inspector_entry_iterator_current_data,
    (void (*)(zend_object_iterator *, zval *)) 		php_inspector_entry_iterator_current_key,
    (void (*)(zend_object_iterator *))				php_inspector_entry_iterator_move_forward,
    (void (*)(zend_object_iterator *)) 				php_inspector_entry_iterator_rewind
};

static zend_object_iterator* php_inspector_entry_iterate(zend_class_entry *ce, zval *object, int by_ref) {
    php_inspector_entry_iterator_t *iterator = 
		(php_inspector_entry_iterator_t*) ecalloc(1, sizeof(php_inspector_entry_iterator_t));
	php_inspector_entry_t *entry;

    zend_iterator_init((zend_object_iterator*)iterator);

	ZVAL_COPY(&iterator->object, object);
	ZVAL_UNDEF(&iterator->zit.data);

	entry = 
		php_inspector_entry_fetch_from(Z_OBJ(iterator->object));
	zend_hash_internal_pointer_reset_ex(&entry->entry->function_table, &iterator->position);

    iterator->zit.funcs = &php_inspector_entry_iterator_funcs;

    return (zend_object_iterator*) iterator;
}
/* }}} */

/* {{{ */
static PHP_METHOD(Entry, __construct) {
	php_inspector_entry_t *entry = 
		php_inspector_entry_this();
	zend_class_entry *ce = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "C", &ce) != SUCCESS) {
		return;
	}

	php_inspector_entry_construct(getThis(), ce);
}

static PHP_METHOD(Entry, count) {
	php_inspector_entry_t *entry = 
		php_inspector_entry_this();

	RETURN_LONG(zend_hash_num_elements(&entry->entry->function_table));
}

static PHP_METHOD(Entry, getMethod) {
	php_inspector_entry_t *entry =
		php_inspector_entry_this();
	zend_string *method;
	zend_function *function;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &method) != SUCCESS) {
		return;		
	}

	if (!(function = php_inspector_scope_find(entry->entry, method))) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"could not find method %s::%s", 
			ZSTR_VAL(entry->entry->name), ZSTR_VAL(method));
		return;
	}

	object_init_ex(return_value, php_inspector_method_ce);
	php_inspector_scope_construct(return_value, function);
}

static PHP_METHOD(Entry, getMethods) {
	php_inspector_entry_t *entry = 
		php_inspector_entry_this();
	zend_string *name;
	zend_function *function;

	array_init(return_value);

	ZEND_HASH_FOREACH_STR_KEY_PTR(&entry->entry->function_table, name, function) {
		add_next_index_str(return_value, zend_string_copy(name));
	} ZEND_HASH_FOREACH_END();
}

static PHP_METHOD(Entry, getName) 
{
	php_inspector_entry_t *entry = 
		php_inspector_entry_this();

	RETURN_STR_COPY(entry->entry->name);
}
/* }}} */

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(Entry_getMethod_arginfo, 0, 1, Inspector\\Method, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Entry_getMethod_arginfo, 0, 1, IS_OBJECT, "Inspector\\Method", 1)
#endif
	ZEND_ARG_TYPE_INFO(0, method, IS_STRING, 0)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Entry_getMethods_arginfo, 0, 0, IS_ARRAY, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Entry_getMethods_arginfo, 0, 0, IS_ARRAY, NULL, 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Entry_getName_arginfo, 0, 0, IS_STRING, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Entry_getName_arginfo, 0, 0, IS_STRING, NULL, 0)
#endif
ZEND_END_ARG_INFO()

/* {{{ */
static zend_function_entry php_inspector_entry_methods[] = {
	PHP_ME(Entry, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Entry, count, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Entry, getMethod, Entry_getMethod_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Entry, getMethods, Entry_getMethods_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Entry, getName, Entry_getName_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(inspector_entry) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Entry", php_inspector_entry_methods);
	php_inspector_entry_ce = 
		zend_register_internal_class(&ce);
	php_inspector_entry_ce->create_object = php_inspector_entry_create;
	php_inspector_entry_ce->get_iterator  = php_inspector_entry_iterate;
	php_inspector_entry_ce->ce_flags |= ZEND_ACC_FINAL;
	zend_class_implements(php_inspector_entry_ce, 2, zend_ce_traversable, spl_ce_Countable);

	memcpy(&php_inspector_entry_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_entry_handlers.offset = XtOffsetOf(php_inspector_entry_t, std);
	php_inspector_entry_handlers.free_obj = php_inspector_entry_destroy;

	return SUCCESS;
} /* }}} */
#endif
