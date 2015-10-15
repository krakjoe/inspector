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
#ifndef HAVE_INSPECTOR_NODE
#define HAVE_INSPECTOR_NODE

#include "php.h"
#include "php_ini.h"
#include "zend_closures.h"

#include "ext/standard/info.h"
#include "ext/spl/spl_exceptions.h"
#include "php_inspector.h"

#include "inspector.h"
#include "node.h"
#include "operand.h"

/* {{{ */
void php_inspector_node_destroy(zend_object *object) {
	php_inspector_node_t *node = 
		php_inspector_node_fetch_from(object);

	zend_object_std_dtor(object);

	if (Z_TYPE(node->inspector) != IS_UNDEF)
		zval_ptr_dtor(&node->inspector);	
}

void php_inspector_node_construct(zval *object, zval *inspector, zend_op *opline) {
	php_inspector_node_t *node = NULL;
		
	object_init_ex(object, php_inspector_node_ce);

	node = php_inspector_node_fetch(object);
	ZVAL_COPY(&node->inspector, inspector);
	node->opline = opline;	
}

zend_object* php_inspector_node_create(zend_class_entry *ce) {
	php_inspector_node_t *node = ecalloc(1, sizeof(php_inspector_node_t) + zend_object_properties_size(ce));

	zend_object_std_init(&node->std, ce);	
	object_properties_init(&node->std, ce);

	node->std.handlers = &php_inspector_node_handlers;
	
	ZVAL_UNDEF(&node->inspector);	
	
	return &node->std;
} /* }}} */

/* {{{ */
PHP_METHOD(Node, getType) {
	php_inspector_node_t *node = php_inspector_node_this();
	const char *name;
	
	if (node->opline->opcode) {
		name = zend_get_opcode_name
			(node->opline->opcode);
		RETURN_STRING((char*)name);
	}
}

PHP_METHOD(Node, getOperand) {
	php_inspector_node_t *node = php_inspector_node_this();
	zend_long operand = PHP_INSPECTOR_NODE_INVALID;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &operand) != SUCCESS) {
		return;
	}

#define NEW_OPERAND(t, o) php_inspector_operand_construct(return_value, getThis(), t, o)
	switch (operand) {
		case PHP_INSPECTOR_NODE_OP1: 
			NEW_OPERAND(node->opline->op1_type, &node->opline->op1); 
		break;

		case PHP_INSPECTOR_NODE_OP2: 
			NEW_OPERAND(node->opline->op2_type, &node->opline->op2); 
		break;

		case PHP_INSPECTOR_NODE_RESULT: 
			NEW_OPERAND(node->opline->result_type, &node->opline->result); 
		break;
	}
#undef NEW_OPERAND
} /* }}} */
#endif
