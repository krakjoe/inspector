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
#ifndef HAVE_INSPECTOR_OPERAND
#define HAVE_INSPECTOR_OPERAND

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
void php_inspector_operand_destroy(zend_object *object) {
	php_inspector_operand_t *operand = php_inspector_operand_fetch_from(object);

	zend_object_std_dtor(object);	

	if (Z_TYPE(operand->node) != IS_UNDEF)
		zval_ptr_dtor(&operand->node);
}

void php_inspector_operand_construct(zval *object, zval *node, zend_uchar type, znode_op *op) {
	php_inspector_operand_t *operand;
	
	object_init_ex(object, php_inspector_operand_ce);
	operand = php_inspector_operand_fetch(object);
	ZVAL_COPY(&operand->node, node);
	operand->type = type;
	operand->op = op;
}

zend_object* php_inspector_operand_create(zend_class_entry *ce) {
	php_inspector_operand_t *operand = 
		ecalloc(1, sizeof(php_inspector_operand_t) + zend_object_properties_size(ce));

	zend_object_std_init(&operand->std, ce);
	object_properties_init(&operand->std, ce);

	operand->std.handlers = &php_inspector_operand_handlers;	
	ZVAL_UNDEF(&operand->node);
	
	return &operand->std;
} /* }}} */

PHP_METHOD(Operand, isUnused) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_UNUSED);
}

PHP_METHOD(Operand, isCompiledVariable) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_CV);
}

PHP_METHOD(Operand, isTemporaryVariable) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_TMP_VAR);
}

PHP_METHOD(Operand, isVariable) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_VAR);
}

PHP_METHOD(Operand, isConstant) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_CONST);
}

PHP_METHOD(Operand, getConstantValue) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	if (operand->type & IS_CONST) {
		php_inspector_node_t *node = 
			php_inspector_node_fetch_from(Z_OBJ(operand->node));
		php_inspector_t *inspector = 
			php_inspector_fetch_from(Z_OBJ(node->inspector));

		ZEND_PASS_TWO_UNDO_CONSTANT(inspector->ops, *operand->op);
		ZVAL_COPY(return_value, &inspector->ops->literals[operand->op->num]);
		ZEND_PASS_TWO_UPDATE_CONSTANT(inspector->ops, *operand->op);
	}
}

PHP_METHOD(Operand, getVariableName) {
	php_inspector_operand_t *operand = php_inspector_operand_this();

	if (operand->type & IS_CV) {
		php_inspector_node_t *node = 
			php_inspector_node_fetch_from(Z_OBJ(operand->node));
		php_inspector_t *inspector = 
			php_inspector_fetch_from(Z_OBJ(node->inspector));
		
		RETURN_STR(zend_string_copy(inspector->ops->vars[EX_VAR_TO_NUM(operand->op->var)]));
	}
}

PHP_METHOD(Operand, getVariableNumber) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();
	php_inspector_node_t *node = 
		php_inspector_node_fetch_from(Z_OBJ(operand->node));
	php_inspector_t *inspector = 
		php_inspector_fetch_from(Z_OBJ(node->inspector));

	if (operand->type & IS_CONST) {
		ZEND_PASS_TWO_UNDO_CONSTANT(inspector->ops, *operand->op);
		ZVAL_LONG(return_value, operand->op->num);
		ZEND_PASS_TWO_UPDATE_CONSTANT(inspector->ops, *operand->op);
	} else if (operand->type & IS_TMP_VAR|IS_VAR) {
		ZVAL_LONG(return_value, EX_VAR_TO_NUM(operand->op->num - inspector->ops->last_var));
	} else ZVAL_LONG(return_value, EX_VAR_TO_NUM(operand->op->num));
}
#endif
