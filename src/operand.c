/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2018 Joe Watkins                                       |
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

#ifndef HAVE_INSPECTOR_OPERAND
#define HAVE_INSPECTOR_OPERAND

#include "php.h"

#include "php_inspector.h"

#include "reflection.h"
#include "class.h"
#include "function.h"
#include "instruction.h"
#include "operand.h"
#include "frame.h"
#include "break.h"

static zend_object_handlers php_inspector_operand_handlers;
zend_class_entry *php_inspector_operand_ce;

#ifdef EXT_TYPE_UNUSED
#define zend_op_type(type) (type & ~EXT_TYPE_UNUSED)
#else
#define zend_op_type(type) (type)
#endif

/* {{{ */
static void php_inspector_operand_destroy(zend_object *object) {
	php_inspector_operand_t *operand = php_inspector_operand_fetch_from(object);

	zend_object_std_dtor(object);

	if (Z_TYPE(operand->instruction) != IS_UNDEF)
		zval_ptr_dtor(&operand->instruction);
}

void php_inspector_operand_factory(zval *instruction, uint32_t which, zend_uchar type, znode_op *op, zval *return_value) {
	php_inspector_operand_t *operand;

	object_init_ex(return_value, php_inspector_operand_ce);

	operand = php_inspector_operand_fetch(return_value);

	ZVAL_COPY(&operand->instruction, instruction);

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
	ZVAL_UNDEF(&operand->instruction);
	
	return &operand->std;
} /* }}} */

/* {{{ */
static PHP_METHOD(InspectorOperand, isJumpTarget) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_fetch_from(Z_OBJ(operand->instruction));
	php_inspector_break_t *brk = php_inspector_break_find_ptr(instruction);

	switch(brk ? brk->opcode : instruction->opline->opcode) {
		case ZEND_JMP:
		case ZEND_FAST_CALL:
#if PHP_VERSION_ID < 70300
		case ZEND_DECLARE_ANON_CLASS:
		case ZEND_DECLARE_ANON_INHERITED_CLASS:
#endif
			if (operand->which == PHP_INSPECTOR_OPERAND_OP1) {
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
			if (operand->which == PHP_INSPECTOR_OPERAND_OP2) {
				RETURN_TRUE;
			}
		break;
	}
	
	RETURN_FALSE;
}

static PHP_METHOD(InspectorOperand, isUnused) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(zend_op_type(operand->type) == IS_UNUSED);
}

static PHP_METHOD(InspectorOperand, isExtendedTypeUnused) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

#ifdef EXT_TYPE_UNUSED
	RETURN_BOOL(operand->type & EXT_TYPE_UNUSED);
#else
	RETURN_BOOL(operand->type == IS_UNUSED);
#endif
}

static PHP_METHOD(InspectorOperand, isCompiledVariable) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(zend_op_type(operand->type) == IS_CV);
}

static PHP_METHOD(InspectorOperand, isTemporaryVariable) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(zend_op_type(operand->type) == IS_TMP_VAR);
}

static PHP_METHOD(InspectorOperand, isVariable) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(zend_op_type(operand->type) == IS_VAR);
}

static PHP_METHOD(InspectorOperand, isConstant) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_BOOL(zend_op_type(operand->type) == IS_CONST);
}

static PHP_METHOD(InspectorOperand, getWhich) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	RETURN_LONG(operand->which);
}

static zend_always_inline zend_bool php_inspector_operand_is_class(php_inspector_instruction_t *instruction, php_inspector_operand_t *operand) {
	php_inspector_break_t *brk = 
		php_inspector_break_find_ptr(instruction);

	switch (brk ? brk->opcode : instruction->opline->opcode) {
		case ZEND_FETCH_CLASS:
		case ZEND_DECLARE_CLASS:
		case ZEND_DECLARE_ANON_CLASS:
			if (operand->which == PHP_INSPECTOR_OPERAND_RESULT) {
				return 1;
			}
		break;

		case ZEND_ADD_INTERFACE:
		case ZEND_ADD_TRAIT:
		case ZEND_BIND_TRAITS:
		case ZEND_VERIFY_ABSTRACT_CLASS:
			if (operand->which == PHP_INSPECTOR_OPERAND_OP1) {
				return 1;
			}
		break;

		case ZEND_DECLARE_INHERITED_CLASS_DELAYED:
		case ZEND_INSTANCEOF:
			if (operand->which == PHP_INSPECTOR_OPERAND_OP2) {
				return 1;
			}
		break;

		case ZEND_INIT_STATIC_METHOD_CALL:
#ifdef ZEND_FETCH_CLASS_CONSTANT
		case ZEND_FETCH_CLASS_CONSTANT:
#endif
		case ZEND_NEW:
			if (operand->which == PHP_INSPECTOR_OPERAND_OP1 &&
			    operand->type != IS_CONST && operand->type != IS_UNUSED) {
				return 1;
			}
		break;

#ifdef ZEND_UNSET_STATIC_PROP
		case ZEND_UNSET_STATIC_PROP:
			if (operand->which == PHP_INSPECTOR_OPERAND_OP2 &&
			    operand->type != IS_CONST && operand->type != IS_UNUSED) {
				return 1;
			}
		break;
#endif

#ifdef ZEND_ISSET_ISEMPTY_STATIC_PROP
		case ZEND_ISSET_ISEMPTY_STATIC_PROP:
			if (operand->which == PHP_INSPECTOR_OPERAND_OP2 &&
			    operand->type != IS_UNUSED) {
				return 1;
			}
		break;
#endif

		case ZEND_DECLARE_INHERITED_CLASS:
		case ZEND_DECLARE_ANON_INHERITED_CLASS:
			if (operand->which == PHP_INSPECTOR_OPERAND_RESULT ||
			    operand->which == PHP_INSPECTOR_OPERAND_OP2) {
				return 1;
			}
		break;
	}

	return 0;
}

static PHP_METHOD(InspectorOperand, getValue) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_fetch(&operand->instruction);
	zend_op_array *function =
		(zend_op_array*) 
			php_reflection_object_function(&instruction->function);
	zval *rt = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|O", &rt, php_inspector_frame_ce) != SUCCESS) {
		return;
	}

	if (zend_op_type(operand->type) == IS_CONST) {
		znode_op op = *operand->op;

#if PHP_VERSION_ID >= 70300
		ZEND_PASS_TWO_UNDO_CONSTANT(function, instruction->opline, op);
#else
		ZEND_PASS_TWO_UNDO_CONSTANT(function, op);
#endif
		ZVAL_COPY(return_value, &function->literals[op.num]);
	} else if(rt) {
		php_inspector_frame_t *frame = 
			php_inspector_frame_fetch(rt);
		zval *value = 
			ZEND_CALL_VAR(frame->frame, operand->op->var);

		if (Z_PTR_P(value) && php_inspector_operand_is_class(instruction, operand)) {
			php_inspector_class_factory(Z_PTR_P(value), return_value);
			return;
		}

		if (Z_TYPE_P(value) == IS_INDIRECT) {
			value = Z_INDIRECT_P(value);
		}

		if (Z_TYPE_P(value) != IS_UNDEF) {
			ZVAL_COPY(return_value, value);
		}
	}
}

static PHP_METHOD(InspectorOperand, getName) {
	php_inspector_operand_t *operand = php_inspector_operand_this();

	if (zend_op_type(operand->type) == IS_CV) {
		php_inspector_instruction_t *instruction = 
			php_inspector_instruction_fetch(&operand->instruction);
		zend_op_array *function = 
			(zend_op_array*)
				php_reflection_object_function(&instruction->function);

		RETURN_STR(zend_string_copy(function->vars[EX_VAR_TO_NUM(operand->op->var)]));
	}
}

static PHP_METHOD(InspectorOperand, getNumber) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();
	php_inspector_instruction_t *instruction = 
		php_inspector_instruction_fetch_from(Z_OBJ(operand->instruction));
	php_inspector_break_t *brk = php_inspector_break_find_ptr(instruction);

	zend_op_array *function = 
		(zend_op_array*)
			php_reflection_object_function(&instruction->function);

	switch(brk ? brk->opcode : instruction->opline->opcode) {
		case ZEND_JMP:
		case ZEND_FAST_CALL:
#if PHP_VERSION_ID < 70300
		case ZEND_DECLARE_ANON_CLASS:
		case ZEND_DECLARE_ANON_INHERITED_CLASS:
#endif
			if (operand->which == PHP_INSPECTOR_OPERAND_OP1) {
				znode_op op = *operand->op;

				ZEND_PASS_TWO_UNDO_JMP_TARGET(function, instruction->opline, op);
				ZVAL_LONG(return_value, op.num);
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
			if (operand->which == PHP_INSPECTOR_OPERAND_OP2) {
				znode_op op = *operand->op;;

				ZEND_PASS_TWO_UNDO_JMP_TARGET(function, instruction->opline, op);

				ZVAL_LONG(return_value, op.num);
				break;
			}

		default: {
			if (zend_op_type(operand->type) == IS_CONST) {
				znode_op op = *operand->op;
#if PHP_VERSION_ID >= 70300
				ZEND_PASS_TWO_UNDO_CONSTANT(function, instruction->opline, op);
#else
				ZEND_PASS_TWO_UNDO_CONSTANT(function, op);
#endif
				ZVAL_LONG(return_value, op.num);
			} else if (zend_op_type(operand->type) == IS_TMP_VAR || IS_VAR == zend_op_type(operand->type)) {
				ZVAL_LONG(return_value, EX_VAR_TO_NUM(operand->op->num - function->last_var));
			} else if (zend_op_type(operand->type) == IS_CV) {
				ZVAL_LONG(return_value, EX_VAR_TO_NUM(operand->op->num));
			}
		}
	}
}

static PHP_METHOD(InspectorOperand, getInstruction) {
	php_inspector_operand_t *operand = 
		php_inspector_operand_this();

	ZVAL_COPY(return_value, &operand->instruction);
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
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorOperand_returns_instruction_arginfo, 0, 0, Inspector\\InspectorInstruction, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorOperand_returns_instruction_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorInstruction", 0)
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
	PHP_ME(InspectorOperand, getInstruction, InspectorOperand_returns_instruction_arginfo, ZEND_ACC_PUBLIC)
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

	zend_declare_class_constant_long(php_inspector_operand_ce, ZEND_STRL("OP1"), PHP_INSPECTOR_OPERAND_OP1);
	zend_declare_class_constant_long(php_inspector_operand_ce, ZEND_STRL("OP2"), PHP_INSPECTOR_OPERAND_OP2);
	zend_declare_class_constant_long(php_inspector_operand_ce, ZEND_STRL("RESULT"), PHP_INSPECTOR_OPERAND_RESULT);

	memcpy(&php_inspector_operand_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_operand_handlers.offset = XtOffsetOf(php_inspector_operand_t, std);
	php_inspector_operand_handlers.free_obj = php_inspector_operand_destroy;

	return SUCCESS;
} /* }}} */
#endif
