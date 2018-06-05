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

#ifndef HAVE_INSPECTOR_INSTRUCTION
#define HAVE_INSPECTOR_INSTRUCTION

#include "php.h"
#include "zend_exceptions.h"

#include "php_inspector.h"

#include "strings.h"
#include "reflection.h"
#include "class.h"
#include "method.h"
#include "function.h"
#include "instruction.h"
#include "operand.h"
#include "break.h"

#ifndef ZEND_VM_LAST_OPCODE
#define ZEND_VM_LAST_OPCODE ZEND_DECLARE_ANON_INHERITED_CLASS
#endif

static zend_object_handlers php_inspector_instruction_handlers;
zend_class_entry *php_inspector_instruction_ce;
zend_class_entry *php_inspector_instruction_interface_ce;

/* {{{ */
static zend_always_inline zend_string* php_inspector_opcode_name(php_inspector_instruction_t *instruction) {
	zend_uchar opcode = 0;

	if (instruction->opline->opcode == INSPECTOR_DEBUG_BREAK) {
		php_inspector_break_t *brk = 
			php_inspector_break_find_ptr(instruction);

		opcode = brk->opcode;
	} else if (instruction->opline->opcode) {
		opcode = instruction->opline->opcode;
	}

	return php_inspector_strings_fetch(opcode);
}

static zend_always_inline zend_uchar php_inspector_opcode_num(php_inspector_instruction_t *instruction) {
	if (instruction->opline->opcode == INSPECTOR_DEBUG_BREAK) {
		php_inspector_break_t *brk = 
			php_inspector_break_find_ptr(instruction);

		return brk->opcode;
	} else {
		return instruction->opline->opcode;
	}
}
/* }}} */

/* {{{ */
static void php_inspector_instruction_destroy(zend_object *object) {
	php_inspector_instruction_t *instruction = 
		php_inspector_instruction_fetch_from(object);

	if (Z_TYPE(instruction->function) != IS_UNDEF)
		zval_ptr_dtor(&instruction->function);
	if (Z_TYPE(instruction->previous) != IS_UNDEF)
		zval_ptr_dtor(&instruction->previous);
	if (Z_TYPE(instruction->next) != IS_UNDEF)
		zval_ptr_dtor(&instruction->next);

	zend_object_std_dtor(object);
}

zval *php_inspector_instruction_cache_fetch(zval *function) {
	php_reflection_object_t *reflection =
		php_reflection_object_fetch(function);
	zval k;
	zend_class_entry *scope;
	zval *cache;

	if (EXPECTED(Z_TYPE(reflection->dummy) == IS_ARRAY)) {
		return &reflection->dummy;
	}

	ZVAL_STR(&k, 
		PHP_INSPECTOR_STRING_INSTRUCTION_CACHE);

	scope = EG(fake_scope);

	EG(fake_scope) = Z_OBJCE_P(function);

	cache = Z_OBJ_HT_P(function)
		->read_property(
			function, &k, BP_VAR_RW, NULL, &reflection->dummy);

	EG(fake_scope) = scope;

	return cache;
}

void php_inspector_instruction_cache_flush(zval *function) {
	php_reflection_object_t *reflection =
		php_reflection_object_fetch(function);
	zval *cache = 
		php_inspector_instruction_cache_fetch(function);

	if (EXPECTED(cache && Z_TYPE_P(cache) == IS_ARRAY)) {
		zend_hash_clean(Z_ARRVAL_P(cache));
	}
}

void php_inspector_instruction_factory(zval *function, zend_op *op, zval *return_value) {
	zval *cache = php_inspector_instruction_cache_fetch(function);
	zval *cached = NULL;
	zend_ulong offset = op - php_reflection_object_function(function)->op_array.opcodes;

	if (Z_TYPE_P(cache) != IS_ARRAY) {
		array_init(cache);
	}

	if (!(cached = zend_hash_index_find(Z_ARRVAL_P(cache), offset))) {
		php_inspector_instruction_t *instruction = NULL;

		object_init_ex(return_value, php_inspector_instruction_ce);

		instruction = php_inspector_instruction_fetch(return_value);
		instruction->opline = op;

		ZVAL_COPY(&instruction->function, function);

		if (zend_hash_index_add(Z_ARRVAL_P(cache), offset, return_value)) {
			Z_ADDREF_P(return_value);
		}
	} else {
		ZVAL_COPY(return_value, cached);
	}
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
	php_inspector_instruction_t *instruction = 
		php_inspector_instruction_this();
	zend_string *name = NULL;

	if (instruction->opline->opcode == INSPECTOR_DEBUG_BREAK) {
		php_inspector_break_t *brk = 
			php_inspector_break_find_ptr(instruction);

		name = php_inspector_strings_fetch(brk->opcode);
	} else {
		name = php_inspector_strings_fetch(instruction->opline->opcode);
	}

	if (!name) {
		return;
	}

	RETURN_STR_COPY(name);
}

static PHP_METHOD(InspectorInstruction, getOpcode) {
	php_inspector_instruction_t *instruction = 
		php_inspector_instruction_this();

	RETURN_LONG(php_inspector_opcode_num(instruction));
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
	php_inspector_break_t *brk = 
		php_inspector_break_find_ptr(instruction);
	uint32_t flags = 
		zend_get_opcode_flags(
			brk ? brk->opcode : instruction->opline->opcode)
		& ZEND_VM_EXT_MASK;

	if (!flags) {
		return;
	}

	switch (flags) {
		case ZEND_VM_EXT_JMP_ADDR: {
			zend_op_array *function =
				(zend_op_array*)
					php_reflection_object_function(&instruction->function);

			RETURN_LONG(ZEND_OFFSET_TO_OPLINE(
				instruction->opline, 
				instruction->opline->extended_value) - instruction->opline);
		} break;

		default:
			RETURN_LONG(instruction->opline->extended_value);					
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

static PHP_METHOD(InspectorInstruction, getRelative) {
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_this();
	zend_op_array *function =
		(zend_op_array*)
			php_reflection_object_function(&instruction->function);
	zend_long relative = 0;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "l", &relative) != SUCCESS) {
		return;
	}

	if ((instruction->opline + relative) < function->opcodes + function->last &&
	    (instruction->opline + relative) >= function->opcodes) {
		php_inspector_instruction_factory(
			&instruction->function,
			instruction->opline + relative, return_value);
	}
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

static PHP_METHOD(InspectorInstruction, getAddress)
{
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_this();

	RETURN_LONG((zend_ulong) instruction->opline);
}

static PHP_METHOD(InspectorInstruction, getFlags)
{
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_this();
	php_inspector_break_t *brk = 
		php_inspector_break_find_ptr(instruction);
	zend_long which = -1;
	uint32_t flags = zend_get_opcode_flags(
		brk ? brk->opcode : instruction->opline->opcode);

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "|l", &which) != SUCCESS) {
		return;
	}

	switch (which) {
		case PHP_INSPECTOR_OPERAND_OP1:
			RETURN_LONG(ZEND_VM_OP1_FLAGS(flags));
		case PHP_INSPECTOR_OPERAND_OP2:
			RETURN_LONG(ZEND_VM_OP2_FLAGS(flags));

		default:
			RETURN_LONG(flags);
	}
}
/* }}} */

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getAddress_arginfo, 0, 0, IS_LONG, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getAddress_arginfo, 0, 0, IS_LONG, NULL, 0)
#endif
ZEND_END_ARG_INFO()

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
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorInstruction_getFunction_arginfo, 0, 0, Inspector\\InspectorInstructionInterface, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getFunction_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorInstructionInterface", 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorInstruction_getInstruction_arginfo, 0, 0, Inspector\\InspectorInstruction, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getInstruction_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorInstruction", 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorInstruction_getRelative_arginfo, 0, 0, Inspector\\InspectorInstruction, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getRelative_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorInstruction", 1)
#endif
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
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

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getFlags_arginfo, 0, 0, IS_LONG, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorInstruction_getFlags_arginfo, 0, 0, IS_LONG, NULL, 0)
#endif
	ZEND_ARG_TYPE_INFO(0, which, IS_LONG, 1)
ZEND_END_ARG_INFO()

/* {{{ */
static zend_function_entry php_inspector_instruction_methods[] = {
	PHP_ME(InspectorInstruction, getOpcode, InspectorInstruction_getOpcode_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getOpcodeName, InspectorInstruction_getOpcodeName_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getOperand, InspectorInstruction_getOperand_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getExtendedValue, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getFlags, InspectorInstruction_getFlags_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getLine, InspectorInstruction_getLine_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getFunction, InspectorInstruction_getFunction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getNext, InspectorInstruction_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getPrevious, InspectorInstruction_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getRelative, InspectorInstruction_getRelative_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getOffset, InspectorInstruction_getOffset_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorInstruction, getAddress, InspectorInstruction_getAddress_arginfo, ZEND_ACC_PUBLIC)
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

	{
		uint32_t opcode = 1,
			 end    = ZEND_VM_LAST_OPCODE + 1;

		while (opcode < end) {
			const char *name = zend_get_opcode_name(opcode);

			if (!name) {
				opcode++;
				continue;
			}

			zend_declare_class_constant_long(
				php_inspector_instruction_ce,
				name,
				strlen(name),
				opcode);

			php_inspector_strings_register_opcode(opcode, &name[5]);

			opcode++;
		}

#define php_inspector_instruction_zend_constant(l) do { \
	zend_declare_class_constant_long( \
		php_inspector_instruction_ce, \
			#l, \
			sizeof(#l) - 1, \
			l);\
} while (0)

		php_inspector_instruction_zend_constant(ZEND_VM_OP_SPEC);
		php_inspector_instruction_zend_constant(ZEND_VM_OP_CONST);
		php_inspector_instruction_zend_constant(ZEND_VM_OP_TMPVAR);
		php_inspector_instruction_zend_constant(ZEND_VM_OP_TMPVARCV);
		php_inspector_instruction_zend_constant(ZEND_VM_OP_MASK);
		php_inspector_instruction_zend_constant(ZEND_VM_OP_NUM);
		php_inspector_instruction_zend_constant(ZEND_VM_OP_JMP_ADDR);
		php_inspector_instruction_zend_constant(ZEND_VM_OP_TRY_CATCH);
#ifdef ZEND_VM_OP_LIVE_RANGE
		php_inspector_instruction_zend_constant(ZEND_VM_OP_LIVE_RANGE);
#endif
		php_inspector_instruction_zend_constant(ZEND_VM_OP_THIS);
		php_inspector_instruction_zend_constant(ZEND_VM_OP_NEXT);
		php_inspector_instruction_zend_constant(ZEND_VM_OP_CLASS_FETCH);
		php_inspector_instruction_zend_constant(ZEND_VM_OP_CONSTRUCTOR);
#ifdef ZEND_VM_OP_CONST_FETCH
		php_inspector_instruction_zend_constant(ZEND_VM_OP_CONST_FETCH);
#endif
#ifdef ZEND_VM_OP_CACHE_SLOT
		php_inspector_instruction_zend_constant(ZEND_VM_OP_CACHE_SLOT);
#endif
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_VAR_FETCH);
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_ISSET);
#ifdef ZEND_VM_EXT_CACHE_SLOT
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_CACHE_SLOT);
#endif
#ifdef ZEND_VM_EXT_ARG_NUM
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_ARG_NUM);
#endif
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_ARRAY_INIT);
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_REF);
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_MASK);
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_NUM);
#ifdef ZEND_VM_EXT_LAST_CATCH
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_LAST_CATCH);
#endif
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_JMP_ADDR);
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_DIM_OBJ);
#ifdef ZEND_VM_EXT_CLASS_FETCH
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_CLASS_FETCH);
#endif
#ifdef ZEND_VM_EXT_CONST_FETCH
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_CONST_FETCH);
#endif
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_TYPE);
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_EVAL);
		php_inspector_instruction_zend_constant(ZEND_VM_EXT_SRC);
		php_inspector_instruction_zend_constant(ZEND_VM_NO_CONST_CONST);
		php_inspector_instruction_zend_constant(ZEND_VM_COMMUTATIVE);
	}

	return SUCCESS;
} /* }}} */

/* {{{ */
static zend_function_entry php_inspector_instruction_interface_methods[] = {
	PHP_ABSTRACT_ME(InspectorInstructionInterface, onResolve, InspectorFunction_onResolve_arginfo)
	PHP_ABSTRACT_ME(InspectorInstructionInterface, getInstruction, InspectorFunction_getInstruction_arginfo)
	PHP_ABSTRACT_ME(InspectorInstructionInterface, getInstructionCount, InspectorFunction_getInstructionCount_arginfo)
	PHP_ABSTRACT_ME(InspectorInstructionInterface, getEntryInstruction, InspectorFunction_getEntryInstruction_arginfo)
	PHP_ABSTRACT_ME(InspectorInstructionInterface, findFirstInstruction, InspectorFunction_find_arginfo)
	PHP_ABSTRACT_ME(InspectorInstructionInterface, findLastInstruction, InspectorFunction_find_arginfo)
	PHP_ABSTRACT_ME(InspectorInstructionInterface, flushInstructionCache, InspectorFunction_flush_arginfo)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_MINIT_FUNCTION(inspector_instruction_interface) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorInstructionInterface", php_inspector_instruction_interface_methods);

	php_inspector_instruction_interface_ce = zend_register_internal_interface(&ce);
} /* }}} */
#endif
