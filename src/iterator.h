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
#ifndef HAVE_INSPECTOR_ITERATOR_H
#define HAVE_INSPECTOR_ITERATOR_H
typedef struct _php_inspector_iterator_t {
	zend_object_iterator zit;
	zval object;
	uint32_t opline;
} php_inspector_iterator_t;

extern zend_object_iterator_funcs php_inspector_iterator_funcs;

zend_object_iterator* php_inspector_iterate(zend_class_entry *ce, zval *object, int by_ref);
void php_inspector_iterator_dtor(php_inspector_iterator_t* iterator);
int php_inspector_iterator_validate(php_inspector_iterator_t* iterator);
zval* php_inspector_iterator_current_data(php_inspector_iterator_t* iterator);
void php_inspector_iterator_current_key(php_inspector_iterator_t* iterator, zval* result);
void php_inspector_iterator_move_forward(php_inspector_iterator_t* iterator);
void php_inspector_iterator_rewind(php_inspector_iterator_t* iterator);
#endif
