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
#ifndef HAVE_INSPECTOR_INSTRUCTION
#define HAVE_INSPECTOR_INSTRUCTION

#include "php.h"
#include "zend_exceptions.h"

#include "php_inspector.h"

#include "reflection.h"
#include "class.h"
#include "function.h"
#include "instruction.h"
#include "operand.h"
#include "break.h"

static zend_object_handlers php_inspector_instruction_handlers;
zend_class_entry *php_inspector_instruction_ce;

/* {{{ */
static void php_inspector_instruction_destroy(zend_object *object) {
	php_inspector_instruction_t *instruction = 
		php_inspector_instruction_fetch_from(object);

	zend_object_std_dtor(object);

	if (Z_TYPE(instruction->function) != IS_UNDEF)
		zval_ptr_dtor(&instruction->function);
	if (Z_TYPE(instruction->previous) != IS_UNDEF)
		zval_ptr_dtor(&instruction->previous);
	if (Z_TYPE(instruction->next) != IS_UNDEF)
		zval_ptr_dtor(&instruction->next);
}

void php_inspector_instruction_factory(zval *function, zend_op *op, zval *return_value) {
	php_inspector_instruction_t *instruction = NULL;

	object_init_ex(return_value, php_inspector_instruction_ce);

	instruction = php_inspector_instruction_fetch(return_value);
	instruction->opline = op;

	ZVAL_COPY(&instruction->function, function);
}

static zend_object* php_inspector_instruction_create(zend_class_entry *ce) {
	php_inspector_instruction_t *instruction = 
		(php_inspector_instruction_t*) 
			ecalloc(1, 
				sizeof(php_inspector_instruction_t) + zend_object_properties_size(ce));

	zend_object_std_init(&instruction->std, ce);	
	object_properties_init(&instruction->std, ce);

	instruction->std.handlers = &php_inspector_instruction_handlers;

	ZVAL_UNDEF(&instruction->function);

	return &instruction->std;
} /* }}} */

/* {{{ */
static PHP_METHOD(InspectorInstruction, getOpcodeName) {
	php_inspector_instruction_t *instruction = php_inspector_instruction_this();
	const char *name = NULL;

	if (instruction->opline->opcode == INSPECTOR_DEBUG_BREAK) {
		php_inspector_break_t *brk = 
			php_inspector_break_find_ptr(instruction);

		name = zend_get_opcode_name(brk->opcode);
	} else if (instruction->opline->opcode) {
		name = zend_get_opcode_name
			(instruction->opline->opcode);
	}

	if (!name) {
		return;
	}

	RETURN_STRING((char*)name);
}

static PHP_METHOD(InspectorInstruction, getOpcode) {
	php_inspector_instruction_t *instruction = php_inspector_instruction_this();

	if (instruction->opline->opcode == INSPECTOR_DEBUG_BREAK) {
		php_inspector_break_t *brk = 
			php_inspector_break_find_ptr(instruction);

		RETURN_LONG(brk->opcode);
	} else {
		RETURN_LONG(instruction->opline->opcode);
	}
}

static PHP_METHOD(InspectorInstruction, getOperand) {
	php_inspector_instruction_t *instruction = php_inspector_instruction_this();
	zend_long operand = PHP_INSPECTOR_OPERAND_INVALID;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &operand) != SUCCESS) {
		return;
	}

#define NEW_OPERAND(n, t, o) do { \
	php_inspector_operand_factory(getThis(), n, t, o, return_value); \
	return; \
} while(0)

	switch (operand) {
		case PHP_INSPECTOR_OPERAND_OP1: 
			NEW_OPERAND(operand, instruction->opline->op1_type, &instruction->opline->op1); 
		break;

		case PHP_INSPECTOR_OPERAND_OP2: 
			NEW_OPERAND(operand, instruction->opline->op2_type, &instruction->opline->op2); 
		break;

		case PHP_INSPECTOR_OPERAND_RESULT: 
			NEW_OPERAND(operand, instruction->opline->result_type, &instruction->opline->result); 
		break;

		default:
			zend_throw_exception_ex(reflection_exception_ptr, 0,
				"the requested operand (%ld) is invalid", operand);
	}
#undef NEW_OPERAND
}

static PHP_METHOD(InspectorInstruction, getExtendedValue) {
	php_inspector_instruction_t *instruction = 
		php_inspector_instruction_this();
	zend_op_array *function = 
		(zend_op_array*)
			php_reflection_object_function(&instruction->function);

	switch (instruction->opline->opcode) {
		case ZEND_TYPE_CHECK:
		case ZEND_CAST:
			RETURN_STRING(zend_get_type_by_const(instruction->opline->extended_value));
		break;

		case ZEND_JMPZNZ:
		case ZEND_FE_FETCH_R:
		case ZEND_FE_FETCH_RW:
			RETURN_LONG(ZEND_OFFSET_TO_OPLINE_NUM(function, instruction->opline, instruction->opline->extended_value));
		break;
		
		case ZEND_FETCH_CLASS:
		case ZEND_FETCH_CLASS_NAME:
			switch (instruction->opline->extended_value) {
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
			if (instruction->opline->extended_value) {
				RETURN_STRING(zend_get_opcode_name(instruction->opline->extended_value));
			}
		break;

		case ZEND_ASSIGN_REF:
		case ZEND_RETURN_BY_REF:
			if (instruction->opline->extended_value == ZEND_RETURNS_FUNCTION) {
				RETURN_STRING("function");
			} else RETURN_STRING("value");
		break;

		case ZEND_FETCH_R:
		case ZEND_FETCH_W:
		case ZEND_FETCH_RW:
		case ZEND_FETCH_FUNC_ARG:
		case ZEND_FETCH_UNSET:
		case ZEND_FETCH_IS:
			switch (instruction->opline->extended_value & ZEND_FETCH_TYPE_MASK) {
#ifdef ZEND_FETCH_STATIC
				case ZEND_FETCH_STATIC: RETURN_STRING("static"); break;
#endif
				case ZEND_FETCH_GLOBAL_LOCK: RETURN_STRING("global"); break;
				case ZEND_FETCH_LOCAL: RETURN_STRING("local"); break;
#ifdef ZEND_FETCH_STATIC_MEMBER
				case ZEND_FETCH_STATIC_MEMBER: RETURN_STRING("member"); break;
#endif
#ifdef ZEND_FETCH_LEXICAL
				case ZEND_FETCH_LEXICAL: RETURN_STRING("lexical"); break;
#endif
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
			RETURN_LONG(instruction->opline->extended_value);
		break;
	}
}

static PHP_METHOD(InspectorInstruction, getLine) {
	php_inspector_instruction_t *instruction = 
		php_inspector_instruction_this();
	
	RETURN_LONG(instruction->opline->lineno);
}

static PHP_METHOD(InspectorInstruction, getFunction) {
	php_inspector_instruction_t *instruction = 
		php_inspector_instruction_this();
	
	ZVAL_COPY(return_value, &instruction->function);
} 

static PHP_METHOD(InspectorInstruction, getNext) {
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_this();
	zend_op_array *function =
		(zend_op_array*)
			php_reflection_object_function(&instruction->function);

	if ((instruction->opline + 1) < function->opcodes + function->last) {
		if (Z_TYPE(instruction->next) == IS_UNDEF) {
			php_inspector_instruction_factory(
				&instruction->function, instruction->opline + 1, &instruction->next);
		}
		ZVAL_COPY(return_value, &instruction->next);
	}
}

static PHP_METHOD(InspectorInstruction, getPrevious) {
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_this();
	zend_op_array *function =
		(zend_op_array*)
			php_reflection_object_function(&instruction->function);

	if ((instruction->opline - 1) >= function->opcodes) {
		if (Z_TYPE(instruction->previous) == IS_UNDEF) {
			php_inspector_instruction_factory(
				&instruction->function, instruction->opline - 1, &instruction->previous);
		}
		ZVAL_COPY(return_value, &instruction->previous);
	}
}

static PHP_METHOD(InspectorInstruction, getOffset)
{
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_this();
	zend_op_array *function =
		(zend_op_array*)
			php_reflection_object_function(&instruction->function);
	
	RETURN_LONG(instruction->opline - function->opcodes);
}

static PHP_METHOD(InspectorInstruction, getBreakPoint)
{
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_this();

	php_inspector_break_find(return_value, instruction);
}
/* }}} */

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getOpcode_arginfo, 0, 0, IS_LONG, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getOpcode_arginfo, 0, 0, IS_LONG, NULL, 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getOpcodeName_arginfo, 0, 0, IS_STRING, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getOpcodeName_arginfo, 0, 0, IS_STRING, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorInstruction_getOperand_arginfo, 0, 1, Inspector\\InspectorOperand, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getOperand_arginfo, 0, 1, IS_OBJECT, "Inspector\\InspectorOperand", 1)
#endif
	ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 0)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getLine_arginfo, 0, 0, IS_LONG, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getLine_arginfo, 0, 0, IS_LONG, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorInstruction_getFunction_arginfo, 0, 0, ReflectionFunctionAbstract, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getFunction_arginfo, 0, 0, IS_OBJECT, ReflectionFunctionAbstract, 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorInstruction_getInstruction_arginfo, 0, 0, Inspector\\InspectorInstruction, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getInstruction_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorInstruction", 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getOffset_arginfo, 0, 0, IS_LONG, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getOffset_arginfo, 0, 0, IS_LONG, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorInstruction_getBreakPoint_arginfo, 0, 0, Inspector\\InspectorBreakPoint, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getBreakPoint_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorBreakPoint", 1)
#endif
ZEND_END_ARG_INFO()

/* {{{ */
static zend_function_entry php_inspector_instruction_methods[] = {
	PHP_ME(InspectorInstruction, getOpcode, InspectorInstruction_getOpcode_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getOpcodeName, InspectorInstruction_getOpcodeName_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getOperand, InspectorInstruction_getOperand_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getExtendedValue, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getLine, InspectorInstruction_getLine_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getFunction, InspectorInstruction_getFunction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getNext, InspectorInstruction_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getPrevious, InspectorInstruction_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getOffset, InspectorInstruction_getOffset_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getBreakPoint, InspectorInstruction_getBreakPoint_arginfo, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(inspector_instruction) {
	zend_class_entry ce;
	
	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorInstruction", php_inspector_instruction_methods);
	php_inspector_instruction_ce = 
		zend_register_internal_class(&ce);
	php_inspector_instruction_ce->create_object = php_inspector_instruction_create;
	php_inspector_instruction_ce->ce_flags |= ZEND_ACC_FINAL;

	memcpy(&php_inspector_instruction_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_instruction_handlers.offset = XtOffsetOf(php_inspector_instruction_t, std);
	php_inspector_instruction_handlers.free_obj = php_inspector_instruction_destroy;
	
	return SUCCESS;
} /* }}} */
#endif
