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
#ifndef HAVE_INSPECTOR_FRAME
#define HAVE_INSPECTOR_FRAME

#include "php.h"

#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
#include "zend_vm.h"

#include "reflection.h"
#include "class.h"
#include "function.h"
#include "opline.h"
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

	if (Z_TYPE(frame->opline) != IS_UNDEF)
		zval_ptr_dtor(&frame->opline);

	zend_object_std_dtor(&frame->std);
}

void php_inspector_frame_factory(zend_execute_data *execute_data, zval *return_value) {
	php_inspector_frame_t *frame;

	object_init_ex(return_value, php_inspector_frame_ce);

	frame = php_inspector_frame_fetch(return_value);
	frame->frame = execute_data;

	php_inspector_function_factory(frame->frame->func, &frame->function);

	if (frame->frame->opline) {
		php_inspector_opline_factory(
			&frame->function, (zend_op*) frame->frame->opline, &frame->opline);
	} else {
		ZVAL_NULL(&frame->opline);
	}
}

PHP_METHOD(InspectorFrame, getOpline)
{
	php_inspector_frame_t *frame = 
		php_inspector_frame_this();
	
	ZVAL_COPY(return_value, &frame->opline);
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
		zval *val = param;
		
		if (Z_TYPE_P(val) == IS_UNDEF) {
			param++;
			continue;
		}

		if (Z_TYPE_P(val) == IS_INDIRECT) {
			val = Z_INDIRECT_P(val);
		}

		add_next_index_zval(return_value, val);
		Z_TRY_ADDREF_P(val);
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

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "l", &num) != SUCCESS) {
		return;
	}

	if (function->type != ZEND_USER_FUNCTION) {
		return;
	}

	if (num < 0 || function->op_array.last_var < num) {
		zend_throw_exception_ex(reflection_exception_ptr, 0, 
			"%d is out of bounds", num);
		return;
	}

	variable = ZEND_CALL_VAR_NUM(frame->frame, num);

	if (Z_TYPE_P(variable) == IS_UNDEF) {
		if (Z_PTR_P(variable)) {
			/* not nice */
			php_inspector_class_factory(Z_PTR_P(variable), return_value);
		}
		return;
	}

	if (Z_TYPE_P(variable) == IS_INDIRECT) {
		ZVAL_COPY(return_value, Z_INDIRECT_P(variable));
	} else {
		ZVAL_COPY(return_value, variable);	
	}
}

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorFrame_getOpline_arginfo, 0, 0, Inspector\\InspectorOpline, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getOpline_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorOpline", 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorFrame_getFunction_arginfo, 0, 0, Inspector\\InspectorFunction, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorFrame_getFunction_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorFunction", 0)
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
	ZEND_ARG_TYPE_INFO(0, num, IS_LONG, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_inspector_frame_methods[] = {
	PHP_ME(InspectorFrame, getOpline, InspectorFrame_getOpline_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getFunction, InspectorFrame_getFunction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getSymbols, InspectorFrame_getSymbols_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getPrevious, InspectorFrame_getPrevious_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getStack, InspectorFrame_getStack_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getCall, InspectorFrame_getCall_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getParameters, InspectorFrame_getParameters_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorFrame, getVariable, InspectorFrame_getVariable_arginfo, ZEND_ACC_PUBLIC)
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
