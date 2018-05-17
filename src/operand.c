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

#include "reflection.h"
#include "class.h"
#include "function.h"
#include "opline.h"
#include "operand.h"
#include "frame.h"

static zend_object_handlers php_inspector_operand_handlers;
zend_class_entry *php_inspector_operand_ce;

/* {{{ */
static void php_inspector_operand_destroy(zend_object *object) {
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

static zend_object* php_inspector_operand_create(zend_class_entry *ce) {
	php_inspector_operand_t *operand = 
		ecalloc(1, sizeof(php_inspector_operand_t) + zend_object_properties_size(ce));

	zend_object_std_init(&operand->std, ce);
	object_properties_init(&operand->std, ce);

	operand->std.handlers = &php_inspector_operand_handlers;	
	ZVAL_UNDEF(&operand->opline);
	
	return &operand->std;
} /* }}} */

/* {{{ */
static PHP_METHOD(InspectorOperand, isJumpTarget) {
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

static PHP_METHOD(InspectorOperand, isUnused) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type == IS_UNUSED);
}

static PHP_METHOD(InspectorOperand, isExtendedTypeUnused) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_UNUSED);
}

static PHP_METHOD(InspectorOperand, isCompiledVariable) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_CV);
}

static PHP_METHOD(InspectorOperand, isTemporaryVariable) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_TMP_VAR);
}

static PHP_METHOD(InspectorOperand, isVariable) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_VAR);
}

static PHP_METHOD(InspectorOperand, isConstant) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(operand->type & IS_CONST);
}

static PHP_METHOD(InspectorOperand, getWhich) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_LONG(operand->which);
}

static PHP_METHOD(InspectorOperand, getValue) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();
	php_inspector_opline_t *opline =
		php_inspector_opline_fetch(&operand->opline);
	zend_op_array *function =
		(zend_op_array*) 
			php_reflection_object_function(&opline->function);
	zval *rt = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|O", &rt, php_inspector_frame_ce) != SUCCESS) {
		return;
	}

	if (operand->type & IS_CONST) {
#if PHP_VERSION_ID >= 70300
		ZEND_PASS_TWO_UNDO_CONSTANT(function, opline->opline, *operand->op);
#else
		ZEND_PASS_TWO_UNDO_CONSTANT(function, *operand->op);
#endif
		ZVAL_COPY(return_value, &function->literals[operand->op->num]);
#if PHP_VERSION_ID >= 70300
		ZEND_PASS_TWO_UPDATE_CONSTANT(function, opline->opline, *operand->op);
#else
		ZEND_PASS_TWO_UPDATE_CONSTANT(function, *operand->op);
#endif
	} else if(rt) {
		php_inspector_frame_t *frame = 
			php_inspector_frame_fetch(rt);
		zval *value = 
			ZEND_CALL_VAR(frame->frame, operand->op->var);

		if (Z_ISUNDEF_P(value)) {
			if (Z_PTR_P(value)) {
				php_inspector_class_factory(Z_PTR_P(value), return_value);
			}
			return;
		}

		if (Z_TYPE_P(value) == IS_INDIRECT) {
			value = Z_INDIRECT_P(value);
		}

		ZVAL_COPY(return_value, value);
	}
}

static PHP_METHOD(InspectorOperand, getName) {
	php_inspector_operand_t *operand = php_inspector_operand_this();

	if (operand->type & IS_CV) {
		php_inspector_opline_t *opline = 
			php_inspector_opline_fetch(&operand->opline);
		zend_op_array *function = 
			(zend_op_array*)
				php_reflection_object_function(&opline->function);

		RETURN_STR(zend_string_copy(function->vars[EX_VAR_TO_NUM(operand->op->var)]));
	}
}

static PHP_METHOD(InspectorOperand, getNumber) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();
	php_inspector_opline_t *opline = 
		php_inspector_opline_fetch_from(Z_OBJ(operand->opline));
	zend_op_array *function = 
		(zend_op_array*)
			php_reflection_object_function(&opline->function);

	switch(opline->opline->opcode) {
		case ZEND_JMP:
		case ZEND_FAST_CALL:
		case ZEND_DECLARE_ANON_CLASS:
		case ZEND_DECLARE_ANON_INHERITED_CLASS:
			if (operand->which == PHP_INSPECTOR_OPLINE_OP1) {
				ZEND_PASS_TWO_UNDO_JMP_TARGET(function, opline->opline, *operand->op);
				ZVAL_LONG(return_value, operand->op->num);
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(function, opline->opline, *operand->op);
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
				ZEND_PASS_TWO_UNDO_JMP_TARGET(function, opline->opline, *operand->op);
				ZVAL_LONG(return_value, operand->op->num);
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(function, opline->opline, *operand->op);
				break;
			}

		default: {
			if (operand->type & IS_CONST) {
#if PHP_VERSION_ID >= 70300
				ZEND_PASS_TWO_UNDO_CONSTANT(function, opline->opline, *operand->op);
#else
				ZEND_PASS_TWO_UNDO_CONSTANT(function, *operand->op);
#endif
				ZVAL_LONG(return_value, operand->op->num);
#if PHP_VERSION_ID >= 70300
				ZEND_PASS_TWO_UPDATE_CONSTANT(function, opline->opline, *operand->op);
#else
				ZEND_PASS_TWO_UPDATE_CONSTANT(function, *operand->op);
#endif
			} else if (operand->type & IS_TMP_VAR|IS_VAR) {
				ZVAL_LONG(return_value, EX_VAR_TO_NUM(operand->op->num - function->last_var));
			} else if (operand->type & IS_CV) {
				ZVAL_LONG(return_value, EX_VAR_TO_NUM(operand->op->num));
			}
		}	
	}
} 

static PHP_METHOD(InspectorOperand, getOpline) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	ZVAL_COPY(return_value, &operand->opline);
} /* }}} */

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorOperand_returns_bool_arginfo, 0, 0, _IS_BOOL, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorOperand_returns_bool_arginfo, 0, 0, _IS_BOOL, NULL, 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorOperand_returns_int_arginfo, 0, 0, IS_LONG, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorOperand_returns_int_arginfo, 0, 0, IS_LONG, NULL, 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorOperand_returns_string_or_null_arginfo, 0, 0, IS_STRING, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorOperand_returns_string_or_null_arginfo, 0, 0, IS_STRING, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorOperand_returns_opline_arginfo, 0, 0, Inspector\\InspectorOpline, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorOperand_returns_opline_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorOpline", 0)
#endif
ZEND_END_ARG_INFO()

/* {{{ */
static zend_function_entry php_inspector_operand_methods[] = {
	PHP_ME(InspectorOperand, isUnused, InspectorOperand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, isExtendedTypeUnused, InspectorOperand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, isCompiledVariable, InspectorOperand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, isTemporaryVariable, InspectorOperand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, isVariable, InspectorOperand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, isConstant, InspectorOperand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, isJumpTarget, InspectorOperand_returns_bool_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, getWhich, InspectorOperand_returns_int_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, getValue, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, getName, InspectorOperand_returns_string_or_null_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, getNumber, InspectorOperand_returns_int_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorOperand, getOpline, InspectorOperand_returns_opline_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(inspector_operand) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorOperand", php_inspector_operand_methods);
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
