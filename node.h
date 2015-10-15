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
#ifndef HAVE_INSPECTOR_NODE_H
#define HAVE_INSPECTOR_NODE_H
typedef struct _php_inspector_node_t {
	zval inspector;
	zend_op *opline;
	zend_object std;
} php_inspector_node_t;

#define PHP_INSPECTOR_NODE_INVALID	0
#define PHP_INSPECTOR_NODE_OP1		1
#define PHP_INSPECTOR_NODE_OP2		2
#define PHP_INSPECTOR_NODE_RESULT	3

zend_object_handlers php_inspector_node_handlers;
zend_class_entry *php_inspector_node_ce;

#define php_inspector_node_fetch_from(o) ((php_inspector_node_t*) (((char*)o) - XtOffsetOf(php_inspector_node_t, std)))
#define php_inspector_node_fetch(z) php_inspector_node_fetch_from(Z_OBJ_P(z))
#define php_inspector_node_this() php_inspector_node_fetch(getThis())

void php_inspector_node_destroy(zend_object *object);
void php_inspector_node_construct(zval *object, zval *inspector, zend_op *opline);
zend_object* php_inspector_node_create(zend_class_entry *ce);

PHP_METHOD(Node, getType);
PHP_METHOD(Node, getOperand);
#endif
