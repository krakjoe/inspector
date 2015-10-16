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
#ifndef HAVE_INSPECTOR_ITERATOR
#define HAVE_INSPECTOR_ITERATOR

#include "php.h"
#include "php_ini.h"
#include "zend_closures.h"

#include "ext/standard/info.h"
#include "ext/spl/spl_exceptions.h"
#include "php_inspector.h"

#include "scope.h"
#include "iterator.h"
#include "opline.h"

void php_inspector_iterator_dtor(php_inspector_iterator_t* iterator) {
	zval_ptr_dtor(&iterator->object);
	if (Z_TYPE(iterator->zit.data) != IS_UNDEF)
		zval_ptr_dtor(&iterator->zit.data);
}

int php_inspector_iterator_validate(php_inspector_iterator_t* iterator) {
	php_inspector_scope_t *scope = 
		php_inspector_scope_fetch_from(Z_OBJ(iterator->object));	
	return (scope->ops->last > iterator->opline) ? SUCCESS : FAILURE;
}

zval* php_inspector_iterator_current_data(php_inspector_iterator_t* iterator) {
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

void php_inspector_iterator_current_key(php_inspector_iterator_t* iterator, zval* result) { ZVAL_LONG(result, iterator->opline); }
void php_inspector_iterator_move_forward(php_inspector_iterator_t* iterator) { iterator->opline++; }
void php_inspector_iterator_rewind(php_inspector_iterator_t* iterator) { iterator->opline = 0; }

zend_object_iterator_funcs php_inspector_iterator_funcs = {
    (void (*) (zend_object_iterator*)) 				php_inspector_iterator_dtor,
    (int (*)(zend_object_iterator *)) 				php_inspector_iterator_validate,
    (zval* (*)(zend_object_iterator *)) 			php_inspector_iterator_current_data,
    (void (*)(zend_object_iterator *, zval *)) 		php_inspector_iterator_current_key,
    (void (*)(zend_object_iterator *))				php_inspector_iterator_move_forward,
    (void (*)(zend_object_iterator *)) 				php_inspector_iterator_rewind
};

zend_object_iterator* php_inspector_iterate(zend_class_entry *ce, zval *object, int by_ref) {
    php_inspector_iterator_t *iterator = 
		(php_inspector_iterator_t*) ecalloc(1, sizeof(php_inspector_iterator_t));
	
    zend_iterator_init((zend_object_iterator*)iterator);

	ZVAL_COPY(&iterator->object, object);
	ZVAL_UNDEF(&iterator->zit.data);

    iterator->zit.funcs = &php_inspector_iterator_funcs;

    return (zend_object_iterator*) iterator;
}
#endif
