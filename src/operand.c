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

#include "php_inspector.h"

#include "scope.h"
#include "opline.h"
#include "operand.h"

zend_object_handlers php_inspector_operand_handlers;
zend_class_entry *php_inspector_operand_ce;

/* {{{ */
void php_inspector_operand_destroy(zend_object *object) {
	php_inspector_operand_t *operand = php_inspector_operand_fetch_from(object);

	zend_object_std_dtor(object);

	if (Z_TYPE(operand->opline) != IS_UNDEF)
		zval_ptr_dtor(&operand->opline);
}

void php_inspector_operand_construct(zval *object, zval *opline, uint32_t which, zend_uchar type, znode_op *op) {
	php_inspector_operand_t *operand;
	
	object_init_ex(object, php_inspector_operand_ce);
	operand = php_inspector_operand_fetch(object);
	ZVAL_COPY(&operand->opline, opline);
	operand->which = which;
	operand->type = type;
	operand->op = op;
}

zend_object* php_inspector_operand_create(zend_class_entry *ce) {
	php_inspector_operand_t *operand = 
		ecalloc(1, sizeof(php_inspector_operand_t) + zend_object_properties_size(ce));

	zend_object_std_init(&operand->std, ce);
	object_properties_init(&operand->std, ce);

	operand->std.handlers = &php_inspector_operand_handlers;	
	ZVAL_UNDEF(&operand->opline);
	
	return &operand->std;
} /* }}} */

/* {{{ */
PHP_METHOD(Operand, isJumpTarget) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();
	php_inspector_opline_t *opline =
		php_inspector_opline_fetch_from(Z_OBJ(operand->opline));

	switch(opline->opline->opcode) {
		case ZEND_JMP:
		case ZEND_FAST_CALL:
		case ZEND_DECLARE_ANON_CLASS:
		case ZEND_DECLARE_ANON_INHERITED_CLASS:
			if (operand->which == PHP_INSPECTOR_OPLINE_OP1) {
				RETURN_TRUE;
			}
		break;

		case ZEND_JMPZNZ:
		case ZEND_JMPZ:
		case ZEND_JMPNZ:
		case ZEND_JMPZ_EX:
		case ZEND_JMP_SET:
		case ZEND_COALESCE:
		case ZEND_NEW:
		case ZEND_FE_RESET_R:
		case ZEND_FE_RESET_RW:
		case ZEND_ASSERT_CHECK:
			if (operand->which == PHP_INSPECTOR_OPLINE_OP2) {
				RETURN_TRUE;
			}
		break;
	}
	
	RETURN_FALSE;
}

PHP_METHOD(Operand, isUnused) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_UNUSED);
}

PHP_METHOD(Operand, isExtendedTypeUnused) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & EXT_TYPE_UNUSED);
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

PHP_METHOD(Operand, getWhich) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_LONG(operand->which);
}

PHP_METHOD(Operand, getValue) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	if (operand->type & IS_CONST) {
		php_inspector_opline_t *opline = 
			php_inspector_opline_fetch_from(Z_OBJ(operand->opline));
		php_inspector_scope_t *scope = 
			php_inspector_scope_fetch_from(Z_OBJ(opline->scope));

		ZEND_PASS_TWO_UNDO_CONSTANT(scope->ops, *operand->op);
		ZVAL_COPY(return_value, &scope->ops->literals[operand->op->num]);
		ZEND_PASS_TWO_UPDATE_CONSTANT(scope->ops, *operand->op);
	}
}

PHP_METHOD(Operand, getName) {
	php_inspector_operand_t *operand = php_inspector_operand_this();

	if (operand->type & IS_CV) {
		php_inspector_opline_t *opline = 
			php_inspector_opline_fetch_from(Z_OBJ(operand->opline));
		php_inspector_scope_t *scope = 
			php_inspector_scope_fetch_from(Z_OBJ(opline->scope));
		
		RETURN_STR(zend_string_copy(scope->ops->vars[EX_VAR_TO_NUM(operand->op->var)]));
	}
}

PHP_METHOD(Operand, getNumber) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();
	php_inspector_opline_t *opline = 
		php_inspector_opline_fetch_from(Z_OBJ(operand->opline));
	php_inspector_scope_t *scope = 
		php_inspector_scope_fetch_from(Z_OBJ(opline->scope));

	switch(opline->opline->opcode) {
		case ZEND_JMP:
		case ZEND_FAST_CALL:
		case ZEND_DECLARE_ANON_CLASS:
		case ZEND_DECLARE_ANON_INHERITED_CLASS:
			if (operand->which == PHP_INSPECTOR_OPLINE_OP1) {
				ZEND_PASS_TWO_UNDO_JMP_TARGET(scope->ops, opline->opline, *operand->op);
				ZVAL_LONG(return_value, operand->op->num);
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(scope->ops, opline->opline, *operand->op);
				break;
			}
		
		case ZEND_JMPZNZ:
		case ZEND_JMPZ:
		case ZEND_JMPNZ:
		case ZEND_JMPZ_EX:
		case ZEND_JMP_SET:
		case ZEND_COALESCE:
		case ZEND_NEW:
		case ZEND_FE_RESET_R:
		case ZEND_FE_RESET_RW:
		case ZEND_ASSERT_CHECK:
			if (operand->which == PHP_INSPECTOR_OPLINE_OP2) {
				ZEND_PASS_TWO_UNDO_JMP_TARGET(scope->ops, opline->opline, *operand->op);
				ZVAL_LONG(return_value, operand->op->num);
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(scope->ops, opline->opline, *operand->op);
				break;
			}

		default: {
			if (operand->type & IS_CONST) {
				ZEND_PASS_TWO_UNDO_CONSTANT(scope->ops, *operand->op);
				ZVAL_LONG(return_value, operand->op->num);
				ZEND_PASS_TWO_UPDATE_CONSTANT(scope->ops, *operand->op);
			} else if (operand->type & IS_TMP_VAR|IS_VAR) {
				ZVAL_LONG(return_value, EX_VAR_TO_NUM(operand->op->num - scope->ops->last_var));
			} else if (operand->type & IS_CV) {
				ZVAL_LONG(return_value, EX_VAR_TO_NUM(operand->op->num));
			}
		}	
	}
} /* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Operand_returns_bool_arginfo, 0, 0, _IS_BOOL, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Operand_returns_int_arginfo, 0, 0, IS_LONG, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(Operand_returns_string_or_null_arginfo, 0, 0, IS_STRING, NULL, 1)
ZEND_END_ARG_INFO()

/* {{{ */
zend_function_entry php_inspector_operand_methods[] = {
	PHP_ME(Operand, isUnused, Operand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isExtendedTypeUnused, Operand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isCompiledVariable, Operand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isTemporaryVariable, Operand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isVariable, Operand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isConstant, Operand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isJumpTarget, Operand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, getWhich, Operand_returns_int_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, getValue, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, getName, Operand_returns_string_or_null_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, getNumber, Operand_returns_int_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(operand) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Operand", php_inspector_operand_methods);
	php_inspector_operand_ce = 
		zend_register_internal_class(&ce);
	php_inspector_operand_ce->create_object = php_inspector_operand_create;
	php_inspector_operand_ce->ce_flags |= ZEND_ACC_FINAL;

	memcpy(&php_inspector_operand_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_operand_handlers.offset = XtOffsetOf(php_inspector_operand_t, std);
	php_inspector_operand_handlers.free_obj = php_inspector_operand_destroy;

	return SUCCESS;
} /* }}} */
#endif
