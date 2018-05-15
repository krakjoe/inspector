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
#include "zend_exceptions.h"

#include "ext/spl/spl_exceptions.h"
#include "ext/spl/spl_iterators.h"
#include "php_inspector.h"

#include "scope.h"
#include "opline.h"
#include "entry.h"

typedef struct _php_inspector_scope_iterator_t {
	zend_object_iterator zit;
	zval object;
	uint32_t opline;
} php_inspector_scope_iterator_t;

static zend_object_iterator* php_inspector_scope_iterate(zend_class_entry *ce, zval *object, int by_ref);

static zend_object_handlers php_inspector_scope_handlers;
zend_class_entry *php_inspector_scope_ce;

/* {{{ */
static zend_object* php_inspector_scope_create(zend_class_entry *ce) {
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
void php_inspector_scope_save(zval *object, HashTable **tables, zval *values) {
	php_inspector_scope_t *scope = php_inspector_scope_fetch(object);

	tables[0] = CG(function_table);
	tables[1] = CG(class_table);
	ZVAL_COPY_VALUE(&values[0], &EG(user_error_handler));

	zend_hash_init(&scope->symbols.functions, 
		zend_hash_num_elements(CG(function_table)),
		NULL, CG(function_table)->pDestructor, 
#if PHP_VERSION_ID >= 70300
		GC_FLAGS(CG(function_table)) & IS_ARRAY_PERSISTENT);
#else
		CG(function_table)->u.flags & HASH_FLAG_PERSISTENT);
#endif

	CG(function_table) = &scope->symbols.functions;

	zend_hash_init(&scope->symbols.classes, 
		zend_hash_num_elements(CG(class_table)),
		NULL, CG(class_table)->pDestructor, 
#if PHP_VERSION_ID >= 70300
		GC_FLAGS(CG(class_table)) & IS_ARRAY_PERSISTENT);
#else
		CG(class_table)->u.flags & HASH_FLAG_PERSISTENT);
#endif

	CG(class_table) = &scope->symbols.classes;

	ZVAL_UNDEF(&EG(user_error_handler));
} /* }}} */

/* {{{ */
void php_inspector_scope_restore(zval *object, HashTable **tables, zval *values) {
	CG(function_table) = tables[0];
	CG(class_table) = tables[1];
	ZVAL_COPY_VALUE(&EG(user_error_handler), &values[0]);
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
static void php_inspector_scope_destroy(zend_object *object) {
	php_inspector_scope_t *scope = 
		php_inspector_scope_fetch_from(object);

	zend_object_std_dtor(object);

	if (scope->symbols.functions.pDestructor) {
		zend_hash_destroy(&scope->symbols.functions);
	}
	
	if (scope->symbols.classes.pDestructor) {
		zend_hash_destroy(&scope->symbols.classes);
	}

	if (scope->ops) {
		destroy_op_array(scope->ops);
		efree(scope->ops);
	}
}
/* }}} */

/* {{{ */
static void php_inspector_scope_iterator_dtor(php_inspector_scope_iterator_t* iterator) {
	zval_ptr_dtor(&iterator->object);
	if (Z_TYPE(iterator->zit.data) != IS_UNDEF)
		zval_ptr_dtor(&iterator->zit.data);
}

static int php_inspector_scope_iterator_validate(php_inspector_scope_iterator_t* iterator) {
	php_inspector_scope_t *scope = 
		php_inspector_scope_fetch_from(Z_OBJ(iterator->object));	
	return (scope->ops->last > iterator->opline) ? SUCCESS : FAILURE;
}

static zval* php_inspector_scope_iterator_current_data(php_inspector_scope_iterator_t* iterator) {
	php_inspector_scope_t *scope = php_inspector_scope_fetch_from(Z_OBJ(iterator->object));

	if (Z_TYPE(iterator->zit.data) != IS_UNDEF) {
		zval_ptr_dtor(&iterator->zit.data);
	}

	php_inspector_opline_construct(&iterator->zit.data, &iterator->object, &scope->ops->opcodes[iterator->opline]);
	
	if (Z_ISUNDEF(iterator->zit.data)) {
		return &EG(uninitialized_zval);
	}

    return &iterator->zit.data;
}

static void php_inspector_scope_iterator_current_key(php_inspector_scope_iterator_t* iterator, zval* result) { ZVAL_LONG(result, iterator->opline); }
static void php_inspector_scope_iterator_move_forward(php_inspector_scope_iterator_t* iterator) { iterator->opline++; }
static void php_inspector_scope_iterator_rewind(php_inspector_scope_iterator_t* iterator) { iterator->opline = 0; }

static zend_object_iterator_funcs php_inspector_scope_iterator_funcs = {
    (void (*) (zend_object_iterator*)) 				php_inspector_scope_iterator_dtor,
    (int (*)(zend_object_iterator *)) 				php_inspector_scope_iterator_validate,
    (zval* (*)(zend_object_iterator *)) 			php_inspector_scope_iterator_current_data,
    (void (*)(zend_object_iterator *, zval *)) 		php_inspector_scope_iterator_current_key,
    (void (*)(zend_object_iterator *))				php_inspector_scope_iterator_move_forward,
    (void (*)(zend_object_iterator *)) 				php_inspector_scope_iterator_rewind
};

static zend_object_iterator* php_inspector_scope_iterate(zend_class_entry *ce, zval *object, int by_ref) {
    php_inspector_scope_iterator_t *iterator = 
		(php_inspector_scope_iterator_t*) ecalloc(1, sizeof(php_inspector_scope_iterator_t));
	
    zend_iterator_init((zend_object_iterator*)iterator);

	ZVAL_COPY(&iterator->object, object);
	ZVAL_UNDEF(&iterator->zit.data);

    iterator->zit.funcs = &php_inspector_scope_iterator_funcs;

    return (zend_object_iterator*) iterator;
}
/* }}} */

/* {{{ */
static PHP_METHOD(Scope, getStatics) {
	php_inspector_scope_t *scope = php_inspector_scope_this();
	
	if (scope->ops->static_variables) {
		RETURN_ARR(zend_array_dup(scope->ops->static_variables));
	} else array_init(return_value);
}

static PHP_METHOD(Scope, getConstants) {
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

static PHP_METHOD(Scope, getVariables) {
	php_inspector_scope_t *scope = php_inspector_scope_this();

	array_init(return_value);

	if (scope->ops->last_var) {
		uint32_t it = 0;
		
		for (it = 0; it < scope->ops->last_var; it++) {
			add_next_index_str(return_value, 
				zend_string_copy(scope->ops->vars[it]));
		}
	}
}

static PHP_METHOD(Scope, getOpline) {
	php_inspector_scope_t *scope = 
		php_inspector_scope_this();
	zend_long opline = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|l", &opline) != SUCCESS) {
		return;
	}

	if (opline < 0 || opline > scope->ops->last) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"opline %ld is out of bounds", opline);
		return;
	}

	php_inspector_opline_construct(return_value, getThis(), &scope->ops->opcodes[opline]);
}

static PHP_METHOD(Scope, count) {
	php_inspector_scope_t *scope = 
		php_inspector_scope_this();

	RETURN_LONG(scope->ops->last);
}

static PHP_METHOD(Scope, getLineStart) {
	php_inspector_scope_t *scope = 
		php_inspector_scope_this();

	RETURN_LONG(scope->ops->line_start);
}

static PHP_METHOD(Scope, getLineEnd) {
	php_inspector_scope_t *scope = 
		php_inspector_scope_this();

	RETURN_LONG(scope->ops->line_end);
}

static PHP_METHOD(Scope, getName) 
{
	php_inspector_scope_t *scope = 
		php_inspector_scope_this();

	if (scope->ops->function_name) {
		RETURN_STR_COPY(scope->ops->function_name);
	}
}

static PHP_METHOD(Scope, getEntry)
{
	php_inspector_scope_t *scope =
		php_inspector_scope_this();

	if (scope->ops->fn_flags & ZEND_ACC_CLOSURE) {
		/* ?? */
	} else if(scope->ops->scope) {
		object_init_ex(
			return_value, php_inspector_entry_ce);

		php_inspector_entry_construct(return_value, scope->ops->scope);
	}
}

static PHP_METHOD(Scope, getFileName)
{
	php_inspector_scope_t *scope =
		php_inspector_scope_this();

	if (scope->ops->filename) {
		RETURN_STR_COPY(scope->ops->filename);
	}
}
/* }}} */

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getArray_arginfo, 0, 0, IS_ARRAY, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getArray_arginfo, 0, 0, IS_ARRAY, NULL, 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(Scope_getOpline_arginfo, 0, 0, Inspector\\Opline, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getOpline_arginfo, 0, 0, IS_OBJECT, "Inspector\\Opline", 1)
#endif
	ZEND_ARG_TYPE_INFO(0, num, IS_LONG, 0)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getLineStart_arginfo, 0, 0, IS_LONG, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getLineStart_arginfo, 0, 0, IS_LONG, NULL, 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getLineEnd_arginfo, 0, 0, IS_LONG, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getLineEnd_arginfo, 0, 0, IS_LONG, NULL, 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getName_arginfo, 0, 0, IS_STRING, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getName_arginfo, 0, 0, IS_STRING, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(Scope_getEntry_arginfo, 0, 0, Inspector\\Entry, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getEntry_arginfo, 0, 0, IS_OBJECT, "Inspector\\Entry", 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getFileName_arginfo, 0, 0, IS_STRING, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Scope_getFileName_arginfo, 0, 0, IS_STRING, NULL, 1)
#endif
ZEND_END_ARG_INFO()

/* {{{ */
static zend_function_entry php_inspector_scope_methods[] = {
	PHP_ME(Scope, getName, Scope_getName_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getStatics, Scope_getArray_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getConstants, Scope_getArray_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getVariables, Scope_getArray_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getOpline, Scope_getOpline_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getLineStart, Scope_getLineStart_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getLineEnd, Scope_getLineEnd_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getFileName, Scope_getFileName_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, count, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getEntry, Scope_getEntry_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(inspector_scope) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Scope", php_inspector_scope_methods);
	php_inspector_scope_ce = 
		zend_register_internal_class(&ce);
	php_inspector_scope_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	php_inspector_scope_ce->create_object = php_inspector_scope_create;
	php_inspector_scope_ce->get_iterator  = php_inspector_scope_iterate;
	zend_class_implements(php_inspector_scope_ce, 2, zend_ce_traversable, spl_ce_Countable);

	memcpy(&php_inspector_scope_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_scope_handlers.offset = XtOffsetOf(php_inspector_scope_t, std);
	php_inspector_scope_handlers.free_obj = php_inspector_scope_destroy;

	return SUCCESS;
} /* }}} */
#endif
