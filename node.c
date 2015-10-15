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

#define NEW_OPERAND(n, t, o) php_inspector_operand_construct(return_value, getThis(), n, t, o)
	switch (operand) {
		case PHP_INSPECTOR_NODE_OP1: 
			NEW_OPERAND(operand, node->opline->op1_type, &node->opline->op1); 
		break;

		case PHP_INSPECTOR_NODE_OP2: 
			NEW_OPERAND(operand, node->opline->op2_type, &node->opline->op2); 
		break;

		case PHP_INSPECTOR_NODE_RESULT: 
			NEW_OPERAND(operand, node->opline->result_type, &node->opline->result); 
		break;
	}
#undef NEW_OPERAND
} 

PHP_METHOD(Node, getExtendedValue) {
	php_inspector_node_t *node = 
		php_inspector_node_this();
	php_inspector_t *inspector = 
		php_inspector_fetch_from(Z_OBJ(node->inspector));

	switch (node->opline->opcode) {
		case ZEND_TYPE_CHECK:
		case ZEND_CAST:
			RETURN_STRING(zend_get_type_by_const(node->opline->extended_value));
		break;

		case ZEND_JMPZNZ:
		case ZEND_FE_FETCH_R:
		case ZEND_FE_FETCH_RW:
			RETURN_LONG(ZEND_OFFSET_TO_OPLINE_NUM(inspector->ops, node->opline, node->opline->extended_value));
		break;
		
		case ZEND_FETCH_CLASS:
		case ZEND_FETCH_CLASS_NAME:
			switch (node->opline->extended_value) {
				case ZEND_FETCH_CLASS_DEFAULT: RETURN_STRING("default"); break;
				case ZEND_FETCH_CLASS_SELF: RETURN_STRING("self"); break;
				case ZEND_FETCH_CLASS_STATIC: RETURN_STRING("static"); break;
				case ZEND_FETCH_CLASS_PARENT: RETURN_STRING("parent"); break;
			}
		break;

		case ZEND_ASSIGN_ADD:
		case ZEND_ASSIGN_SUB:
		case ZEND_ASSIGN_MUL:
		case ZEND_ASSIGN_DIV:
		case ZEND_ASSIGN_MOD:
		case ZEND_ASSIGN_SL:
		case ZEND_ASSIGN_SR:
		case ZEND_ASSIGN_CONCAT:
		case ZEND_ASSIGN_BW_OR:
		case ZEND_ASSIGN_BW_AND:
		case ZEND_ASSIGN_BW_XOR:
		case ZEND_ASSIGN_POW:
		case ZEND_INCLUDE_OR_EVAL:
			if (node->opline->extended_value) {
				RETURN_STRING(zend_get_opcode_name(node->opline->extended_value));
			}
		break;

		case ZEND_ASSIGN_REF:
		case ZEND_RETURN_BY_REF:
			if (node->opline->extended_value == ZEND_RETURNS_FUNCTION) {
				RETURN_STRING("function");
			} else RETURN_STRING("value");
		break;

		case ZEND_FETCH_R:
		case ZEND_FETCH_W:
		case ZEND_FETCH_RW:
		case ZEND_FETCH_FUNC_ARG:
		case ZEND_FETCH_UNSET:
		case ZEND_FETCH_IS:
			switch (node->opline->extended_value & ZEND_FETCH_TYPE_MASK) {
				case ZEND_FETCH_STATIC: RETURN_STRING("static"); break;
				case ZEND_FETCH_GLOBAL_LOCK: RETURN_STRING("global"); break;
				case ZEND_FETCH_LOCAL: RETURN_STRING("local"); break;
				case ZEND_FETCH_STATIC_MEMBER: RETURN_STRING("member"); break;
				case ZEND_FETCH_LEXICAL: RETURN_STRING("lexical"); break;
				case ZEND_FETCH_GLOBAL: RETURN_STRING("global"); break;
			}
		break;

		case ZEND_ROPE_END:
		case ZEND_ROPE_ADD:
		case ZEND_INIT_METHOD_CALL:
		case ZEND_INIT_FCALL_BY_NAME:
		case ZEND_INIT_DYNAMIC_CALL:
		case ZEND_INIT_USER_CALL:
		case ZEND_INIT_NS_FCALL_BY_NAME:
		case ZEND_INIT_FCALL:
		case ZEND_CATCH:
		case ZEND_NEW:
		case ZEND_TICKS:
			RETURN_LONG(node->opline->extended_value);
		break;
	}
} /* }}} */
#endif
