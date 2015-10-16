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
#ifndef HAVE_INSPECTOR_OPLINE
#define HAVE_INSPECTOR_OPLINE

#include "php.h"

#include "php_inspector.h"

#include "scope.h"
#include "opline.h"
#include "operand.h"

zend_object_handlers php_inspector_opline_handlers;

/* {{{ */
void php_inspector_opline_destroy(zend_object *object) {
	php_inspector_opline_t *opline = 
		php_inspector_opline_fetch_from(object);

	zend_object_std_dtor(object);

	if (Z_TYPE(opline->scope) != IS_UNDEF)
		zval_ptr_dtor(&opline->scope);	
}

void php_inspector_opline_construct(zval *object, zval *scope, zend_op *zopline) {
	php_inspector_opline_t *opline = NULL;

	object_init_ex(object, php_inspector_opline_ce);

	opline = php_inspector_opline_fetch(object);
	ZVAL_COPY(&opline->scope, scope);
	opline->opline = zopline;
}

zend_object* php_inspector_opline_create(zend_class_entry *ce) {
	php_inspector_opline_t *opline = ecalloc(1, sizeof(php_inspector_opline_t) + zend_object_properties_size(ce));

	zend_object_std_init(&opline->std, ce);	
	object_properties_init(&opline->std, ce);

	opline->std.handlers = &php_inspector_opline_handlers;
	
	ZVAL_UNDEF(&opline->scope);	
	
	return &opline->std;
} /* }}} */

/* {{{ */
PHP_METHOD(Opline, getType) {
	php_inspector_opline_t *opline = php_inspector_opline_this();
	const char *name;
	
	if (opline->opline->opcode) {
		name = zend_get_opcode_name
			(opline->opline->opcode);
		RETURN_STRING((char*)name);
	}
}

PHP_METHOD(Opline, getOperand) {
	php_inspector_opline_t *opline = php_inspector_opline_this();
	zend_long operand = PHP_INSPECTOR_OPLINE_INVALID;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &operand) != SUCCESS) {
		return;
	}

#define NEW_OPERAND(n, t, o) php_inspector_operand_construct(return_value, getThis(), n, t, o)
	switch (operand) {
		case PHP_INSPECTOR_OPLINE_OP1: 
			NEW_OPERAND(operand, opline->opline->op1_type, &opline->opline->op1); 
		break;

		case PHP_INSPECTOR_OPLINE_OP2: 
			NEW_OPERAND(operand, opline->opline->op2_type, &opline->opline->op2); 
		break;

		case PHP_INSPECTOR_OPLINE_RESULT: 
			NEW_OPERAND(operand, opline->opline->result_type, &opline->opline->result); 
		break;
	}
#undef NEW_OPERAND
}

PHP_METHOD(Opline, getExtendedValue) {
	php_inspector_opline_t *opline = 
		php_inspector_opline_this();
	php_inspector_scope_t *scope = 
		php_inspector_scope_fetch_from(Z_OBJ(opline->scope));

	switch (opline->opline->opcode) {
		case ZEND_TYPE_CHECK:
		case ZEND_CAST:
			RETURN_STRING(zend_get_type_by_const(opline->opline->extended_value));
		break;

		case ZEND_JMPZNZ:
		case ZEND_FE_FETCH_R:
		case ZEND_FE_FETCH_RW:
			RETURN_LONG(ZEND_OFFSET_TO_OPLINE_NUM(scope->ops, opline->opline, opline->opline->extended_value));
		break;
		
		case ZEND_FETCH_CLASS:
		case ZEND_FETCH_CLASS_NAME:
			switch (opline->opline->extended_value) {
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
			if (opline->opline->extended_value) {
				RETURN_STRING(zend_get_opcode_name(opline->opline->extended_value));
			}
		break;

		case ZEND_ASSIGN_REF:
		case ZEND_RETURN_BY_REF:
			if (opline->opline->extended_value == ZEND_RETURNS_FUNCTION) {
				RETURN_STRING("function");
			} else RETURN_STRING("value");
		break;

		case ZEND_FETCH_R:
		case ZEND_FETCH_W:
		case ZEND_FETCH_RW:
		case ZEND_FETCH_FUNC_ARG:
		case ZEND_FETCH_UNSET:
		case ZEND_FETCH_IS:
			switch (opline->opline->extended_value & ZEND_FETCH_TYPE_MASK) {
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
			RETURN_LONG(opline->opline->extended_value);
		break;
	}
} /* }}} */

/* {{{ */
zend_function_entry php_inspector_opline_methods[] = {
	PHP_ME(Opline, getType, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Opline, getOperand, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Opline, getExtendedValue, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(opline) {
	zend_class_entry ce;
	
	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Opline", php_inspector_opline_methods);
	php_inspector_opline_ce = 
		zend_register_internal_class(&ce);
	php_inspector_opline_ce->create_object = php_inspector_opline_create;
	zend_declare_class_constant_long(php_inspector_opline_ce, ZEND_STRL("OP1"), PHP_INSPECTOR_OPLINE_OP1);
	zend_declare_class_constant_long(php_inspector_opline_ce, ZEND_STRL("OP2"), PHP_INSPECTOR_OPLINE_OP2);
	zend_declare_class_constant_long(php_inspector_opline_ce, ZEND_STRL("RESULT"), PHP_INSPECTOR_OPLINE_RESULT);

	memcpy(&php_inspector_opline_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_opline_handlers.offset = XtOffsetOf(php_inspector_opline_t, std);
	php_inspector_opline_handlers.free_obj = php_inspector_opline_destroy;
	
	return SUCCESS;
} /* }}} */
#endif
