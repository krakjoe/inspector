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

#ifndef HAVE_INSPECTOR_FRAME
#define HAVE_INSPECTOR_FRAME

#include "php.h"

#include "zend_vm.h"

#include "reflection.h"
#include "class.h"
#include "function.h"
#include "instruction.h"
#include "frame.h"

zend_class_entry *php_inspector_frame_ce;
zend_object_handlers php_inspector_frame_handlers;

static zend_object* php_inspector_frame_create(zend_class_entry *ce) {
	php_inspector_frame_t *frame =
		(php_inspector_frame_t*)
			ecalloc(1, sizeof(php_inspector_frame_t) + zend_object_properties_size(ce));

	zend_object_std_init(&frame->std, ce);

	object_properties_init(&frame->std, ce);

	frame->std.handlers = &php_inspector_frame_handlers;

	return &frame->std;
}

static void php_inspector_frame_free(zend_object *zo) {
	php_inspector_frame_t *frame =
		php_inspector_frame_fetch_from(zo);

	if (Z_TYPE(frame->function) != IS_UNDEF)
		zval_ptr_dtor(&frame->function);

	if (Z_TYPE(frame->instruction) != IS_UNDEF)
		zval_ptr_dtor(&frame->instruction);

	zend_object_std_dtor(&frame->std);
}

void php_inspector_frame_factory(zend_execute_data *execute_data, zval *return_value) {
	php_inspector_frame_t *frame;

	object_init_ex(return_value, php_inspector_frame_ce);

	frame = php_inspector_frame_fetch(return_value);
	frame->frame = execute_data;

	php_inspector_function_factory(frame->frame->func, &frame->function, 0);

	if (frame->frame->func->type == ZEND_USER_FUNCTION && frame->frame->opline) {
		php_inspector_instruction_factory(
			&frame->function, (zend_op*) frame->frame->opline, &frame->instruction);
	} else {
		ZVAL_NULL(&frame->instruction);
	}
}

PHP_METHOD(InspectorFrame, getInstruction)
{
	php_inspector_frame_t *frame = 
		php_inspector_frame_this();
	
	ZVAL_COPY(return_value, &frame->instruction);
}

PHP_METHOD(InspectorFrame, setInstruction)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();
	zend_function *function =
		php_reflection_object_function(&frame->function);
	php_inspector_instruction_t *instruction;
	zval *in = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "O", &in, php_inspector_instruction_ce) != SUCCESS) {
		return;
	}

	instruction = php_inspector_instruction_fetch(in);

	if (!ZEND_USER_CODE(function->type)) {
		RETURN_FALSE;
	}

	if (instruction->opline < function->op_array.opcodes ||
	    instruction->opline > function->op_array.opcodes + function->op_array.last) {
		RETURN_FALSE;
	}

	if (Z_TYPE(frame->instruction) == IS_OBJECT) {
		zval_ptr_dtor(&frame->instruction);
	}

	ZVAL_COPY(&frame->instruction, in);
	
	instruction = 
		php_inspector_instruction_fetch(&frame->instruction);

	frame->frame->opline = instruction->opline;

	RETURN_TRUE;
}

PHP_METHOD(InspectorFrame, getStack)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();
	zend_function *function =
		php_reflection_object_function(&frame->function);

	zval *var = ZEND_CALL_VAR_NUM(frame->frame, 0),
	     *end = var + function->op_array.last_var;

	array_init_size(return_value, function->op_array.last_var);

	while (var < end) {
		zval *val = var;

		if (Z_TYPE_P(val) == IS_UNDEF) {
			var++;
			continue;
		}

		if (Z_TYPE_P(val) == IS_INDIRECT) {
			val = Z_INDIRECT_P(val);
		}

		zend_hash_add(Z_ARRVAL_P(return_value), 
			function->op_array.vars[function->op_array.last_var - (end - var)],
			val);
		Z_TRY_ADDREF_P(val);

		var++;
	}
}

PHP_METHOD(InspectorFrame, getFunction)
{
	php_inspector_frame_t *frame = 
		php_inspector_frame_this();

	ZVAL_COPY(return_value, &frame->function);
}

PHP_METHOD(InspectorFrame, getSymbols)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();

	if (frame->frame->symbol_table) {
#if PHP_VERSION_ID >= 70300
		GC_ADDREF(frame->frame->symbol_table);
#else
		GC_REFCOUNT(frame->frame->symbol_table)++;
#endif

		RETURN_ARR(frame->frame->symbol_table);
	}
}

PHP_METHOD(InspectorFrame, getPrevious)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();

	if (frame->frame->prev_execute_data) {
		php_inspector_frame_factory(
			frame->frame->prev_execute_data,
			return_value);
	}
}

PHP_METHOD(InspectorFrame, getCall)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();

	if (frame->frame->call) {
		php_inspector_frame_factory(
			frame->frame->call,
			return_value);
	}
}

PHP_METHOD(InspectorFrame, getParameters)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();
	zval *param = ZEND_CALL_ARG(frame->frame, 1),
	     *end   = param + ZEND_CALL_NUM_ARGS(frame->frame);

	array_init_size(return_value, ZEND_CALL_NUM_ARGS(frame->frame));

	while (param < end) {
		add_next_index_zval(return_value, param);
		Z_TRY_ADDREF_P(param);
		param++;
	}
}

PHP_METHOD(InspectorFrame, getVariable)
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();
	zend_function *function =
		php_reflection_object_function(&frame->function);
	zend_long num = 0;
	zval *variable = NULL;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "l", &num) != SUCCESS) {
		return;
	}

	if (function->type != ZEND_USER_FUNCTION) {
		return;
	}

	if (num < 0 || function->op_array.last_var < num) {
		return;
	}

	variable = ZEND_CALL_VAR_NUM(frame->frame, num);

	if (Z_TYPE_P(variable) == IS_UNDEF) {
		return;
	}

	if (Z_TYPE_P(variable) == IS_INDIRECT) {
		ZVAL_COPY(return_value, Z_INDIRECT_P(variable));
	} else {
		ZVAL_COPY(return_value, variable);	
	}
}

static PHP_METHOD(InspectorFrame, getThis) 
{
	php_inspector_frame_t *frame =
		php_inspector_frame_this();

	if (Z_TYPE(frame->frame->This) == IS_OBJECT) {
		ZVAL_COPY(return_value, &frame->frame->This);
	}
}

static PHP_METHOD(InspectorFrame, getCurrent)
{
	php_inspector_frame_factory(EX(prev_execute_data), return_value);
}

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorFrame_getInstruction_arginfo, 0, 0, Inspector\\InspectorInstruction, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getInstruction_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorInstruction", 1)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(InspectorFrame_setInstruction_arginfo, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, name, Inspector\\InspectorInstruction, 0)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorFrame_getFunction_arginfo, 0, 0, Inspector\\InspectorInstructionInterface, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getFunction_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorInstructionInterface", 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getSymbols_arginfo, 0, 0, IS_ARRAY, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getSymbols_arginfo, 0, 0, IS_ARRAY, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorFrame_getPrevious_arginfo, 0, 0, Inspector\\InspectorFrame, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getPrevious_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorFrame", 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getStack_arginfo, 0, 0, IS_ARRAY, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getStack_arginfo, 0, 0, IS_ARRAY, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorFrame_getCall_arginfo, 0, 0, Inspector\\InspectorFrame, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getCall_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorFrame", 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getParameters_arginfo, 0, 0, IS_ARRAY, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getParameters_arginfo, 0, 0, IS_ARRAY, NULL, 1)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(InspectorFrame_getVariable_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, num)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getThis_arginfo, 0, 0, IS_OBJECT, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getThis_arginfo, 0, 0, IS_OBJECT, "object", 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorFrame_getCurrent_arginfo, 0, 0, Inspector\\InspectorFrame, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getCurrent_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorFrame", 1)
#endif
ZEND_END_ARG_INFO()


static zend_function_entry php_inspector_frame_methods[] = {
	PHP_ME(InspectorFrame, getInstruction, InspectorFrame_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, setInstruction, InspectorFrame_setInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getFunction, InspectorFrame_getFunction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getSymbols, InspectorFrame_getSymbols_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getPrevious, InspectorFrame_getPrevious_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getStack, InspectorFrame_getStack_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getThis, InspectorFrame_getThis_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getCall, InspectorFrame_getCall_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getParameters, InspectorFrame_getParameters_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getVariable, InspectorFrame_getVariable_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getCurrent, InspectorFrame_getCurrent_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END
};

PHP_MINIT_FUNCTION(inspector_frame) {
	zend_class_entry ce;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorFrame", php_inspector_frame_methods);
	php_inspector_frame_ce = 
		zend_register_internal_class(&ce);
	php_inspector_frame_ce->ce_flags |= ZEND_ACC_FINAL;
	php_inspector_frame_ce->create_object = php_inspector_frame_create;

	memcpy(&php_inspector_frame_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	php_inspector_frame_handlers.offset = XtOffsetOf(php_inspector_frame_t, std);
	php_inspector_frame_handlers.free_obj = php_inspector_frame_free;

	return SUCCESS;
} /* }}} */
#endif
