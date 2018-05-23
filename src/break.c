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

#ifndef HAVE_INSPECTOR_BREAK
#define HAVE_INSPECTOR_BREAK

#include "php.h"

#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "zend_vm.h"

#include "strings.h"
#include "reflection.h"
#include "class.h"
#include "function.h"
#include "instruction.h"
#include "break.h"
#include "frame.h"

typedef enum _php_inspector_break_state_t {
	INSPECTOR_BREAK_RUNTIME,
	INSPECTOR_BREAK_HANDLER,
	INSPECTOR_BREAK_SHUTDOWN,
} php_inspector_break_state_t;

ZEND_BEGIN_MODULE_GLOBALS(inspector_break)
	php_inspector_break_state_t state;
	HashTable files;
	HashTable pending;
	HashTable breaks;
ZEND_END_MODULE_GLOBALS(inspector_break)

ZEND_DECLARE_MODULE_GLOBALS(inspector_break);

static user_opcode_handler_t php_inspector_break_vm[6];

#define PHP_INSPECTOR_BREAK_DECL_CLASS 0			/* ZEND_DECLARE_CLASS */
#define PHP_INSPECTOR_BREAK_DECL_INHERITED_CLASS 1		/* ZEND_DECLARE_INHERITED_CLASS */
#define PHP_INSPECTOR_BREAK_DECL_INHERITED_CLASS_DELAYED 2	/* ZEND_DECLARE_INHERITED_CLASS_DELAYED */
#define PHP_INSPECTOR_BREAK_DECL_ANON_CLASS 3			/* ZEND_DECARRE_ANON_CLASS */
#define PHP_INSPECTOR_BREAK_DECL_ANON_INHERITED_CLASS 4		/* ZEND_DECLARE_ANON_INHERITED_CLASS */
#define PHP_INSPECTOR_BREAK_DECL_FUNCTION 5			/* ZEND_DECLARE_FUNCTION */

static zend_uchar php_inspector_break_vm_map[6] = {
	ZEND_DECLARE_CLASS,
	ZEND_DECLARE_INHERITED_CLASS,
	ZEND_DECLARE_INHERITED_CLASS_DELAYED,
	ZEND_DECLARE_ANON_CLASS,
	ZEND_DECLARE_ANON_INHERITED_CLASS,
	ZEND_DECLARE_FUNCTION
};

#ifdef ZTS
#define BRK(v) TSRMG(inspector_break_globals_id, zend_inspector_break_globals *, v)
#else
#define BRK(v) (inspector_break_globals.v)
#endif

static void php_inspector_break_globals_init(zend_inspector_break_globals *BRK) {
	memset(BRK, 0, sizeof(*BRK));
}

zend_class_entry *php_inspector_break_ce;
zend_object_handlers php_inspector_break_handlers;

static zend_object* php_inspector_break_create(zend_class_entry *ce) {
	php_inspector_break_t *brk =
		(php_inspector_break_t*)
			ecalloc(1, sizeof(php_inspector_break_t) + zend_object_properties_size(ce));

	zend_object_std_init(&brk->std, ce);

	object_properties_init(&brk->std, ce);

	brk->cache.fci = empty_fcall_info;
	brk->cache.fcc = empty_fcall_info_cache;

	brk->std.handlers = &php_inspector_break_handlers;

	return &brk->std;
}

static void php_inspector_break_destroy(zend_object *zo) {
	php_inspector_break_t *brk = 
		php_inspector_break_fetch_from(zo);
	php_inspector_instruction_t *instruction = 
		Z_TYPE(brk->instruction) != IS_UNDEF ?
			php_inspector_instruction_fetch(&brk->instruction) : NULL;

	if (Z_TYPE(brk->instruction) != IS_UNDEF) {
		if (BRK(state) != INSPECTOR_BREAK_SHUTDOWN) {
			zend_hash_index_del(&BRK(breaks), (zend_ulong) instruction->opline);	
		}
		zval_ptr_dtor(&brk->instruction);
	}

	zend_object_std_dtor(&brk->std);
}

static inline zend_bool php_inspector_break_enable(php_inspector_break_t *brk) {
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_fetch(&brk->instruction);

	if (!zend_hash_index_add_ptr(&BRK(breaks), (zend_ulong) instruction->opline, brk)) {
		return 0;
	}

	brk->opcode = instruction->opline->opcode;

	instruction->opline->opcode = INSPECTOR_DEBUG_BREAK;

	zend_vm_set_opcode_handler(instruction->opline);

	return 1;
}

static inline zend_bool php_inspector_break_enabled(php_inspector_break_t *brk) {
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_fetch(&brk->instruction);

	return zend_hash_index_exists(&BRK(breaks), (zend_ulong) instruction->opline);
}

static inline zend_bool php_inspector_break_disable(php_inspector_break_t *brk) {
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_fetch(&brk->instruction);

	return zend_hash_index_del(&BRK(breaks), (zend_ulong) instruction->opline) == SUCCESS;
}

void php_inspector_breaks_disable(zend_op *begin, zend_op *end) {
	const zend_op *address = begin;

	while (address < end) {
		zend_hash_index_del(&BRK(breaks), (zend_ulong) address);
		address++;
	}
}

php_inspector_break_t* php_inspector_break_find_ptr(php_inspector_instruction_t *instruction) {
	return zend_hash_index_find_ptr(&BRK(breaks), (zend_ulong) instruction->opline);;
}

void php_inspector_break_find(zval *return_value, php_inspector_instruction_t *instruction) {
	php_inspector_break_t *brk = php_inspector_break_find_ptr(instruction);

	if (!brk) {
		return;
	}

	ZVAL_OBJ(return_value, &brk->std);
	Z_ADDREF_P(return_value);
}

/* {{{ */
static PHP_METHOD(InspectorBreakPoint, __construct)
{
	php_inspector_break_t *brk = php_inspector_break_this();
	zval *in = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "O", &in, php_inspector_instruction_ce) != SUCCESS) {
		return;
	}

	ZVAL_COPY(&brk->instruction, in);

	if (!php_inspector_break_enable(brk)) {
		zend_throw_exception_ex(reflection_exception_ptr, 0, 
			"already a InspectorBreakPoint on that instruction");
	}
}

static PHP_METHOD(InspectorBreakPoint, getInstruction)
{
	php_inspector_break_t *brk =
		php_inspector_break_this();

	ZVAL_COPY(return_value, &brk->instruction);
}

static PHP_METHOD(InspectorBreakPoint, disable)
{
	php_inspector_break_t *brk =
		php_inspector_break_this();

	RETURN_BOOL(php_inspector_break_disable(brk));
}

static PHP_METHOD(InspectorBreakPoint, enable)
{
	php_inspector_break_t *brk =
		php_inspector_break_this();

	RETURN_BOOL(php_inspector_break_enable(brk));
}

static PHP_METHOD(InspectorBreakPoint, isEnabled)
{
	php_inspector_break_t *brk =
		php_inspector_break_this();

	RETURN_BOOL(php_inspector_break_enabled(brk));
}

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorBreakPoint_getOpcode_arginfo, 0, 0, IS_LONG, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorBreakPoint_getOpcode_arginfo, 0, 0, IS_LONG, NULL, 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorBreakPoint_getOpcodeName_arginfo, 0, 0, IS_STRING, 1)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorBreakPoint_getOpcodeName_arginfo, 0, 0, IS_STRING, NULL, 1)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(InspectorBreakPoint_getInstruction_arginfo, 0, 0, Inspector\\InspectorInstruction, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorBreakPoint_getInstruction_arginfo, 0, 0, IS_OBJECT, "Inspector\\InspectorInstruction", 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 70200
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorBreakPoint_switch_arginfo, 0, 0, _IS_BOOL, 0)
#else
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(InspectorBreakPoint_switch_arginfo, 0, 0, _IS_BOOL, NULL, 0)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(InspectorBreakPoint_construct_arginfo, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, instruction, Inspector\\InspectorInstruction, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(InspectorBreakPoint_hit_arginfo, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, frame, Inspector\\InspectorFrame, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_inspector_break_methods[] = {
	PHP_ME(InspectorBreakPoint, __construct, InspectorBreakPoint_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorBreakPoint, getInstruction, InspectorBreakPoint_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorBreakPoint, disable, InspectorBreakPoint_switch_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorBreakPoint, enable, InspectorBreakPoint_switch_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorBreakPoint, isEnabled, InspectorBreakPoint_switch_arginfo, ZEND_ACC_PUBLIC)
	PHP_ABSTRACT_ME(InspectorBreakPoint, hit, InspectorBreakPoint_hit_arginfo)
	PHP_FE_END
};

static int php_inspector_break_handler(zend_execute_data *execute_data) {
	zend_op *instruction = (zend_op *) EX(opline);
	php_inspector_break_t *brk = 
		zend_hash_index_find_ptr(&BRK(breaks), (zend_ulong) instruction);

	BRK(state) = INSPECTOR_BREAK_HANDLER;
	{
		zval rv;
		zval ip;

		if (brk->cache.fci.size == 0) {
			brk->cache.fci.size = sizeof(zend_fcall_info);
			brk->cache.fci.object = &brk->std;
#if PHP_VERSION_ID < 70300
			brk->cache.fcc.initialized = 1;
#endif
			brk->cache.fcc.object = brk->cache.fci.object;
			brk->cache.fcc.function_handler = (zend_function*)
				zend_hash_find_ptr(
					&brk->std.ce->function_table, PHP_INSPECTOR_STRING_HIT);
		}

		ZVAL_NULL(&rv);

		php_inspector_frame_factory(execute_data, &ip);

		brk->cache.fci.param_count = 1;
		brk->cache.fci.params = &ip;
		brk->cache.fci.retval = &rv;

		if (zend_call_function(&brk->cache.fci, &brk->cache.fcc) != SUCCESS) {
			/* do something */
			
		}

		if (Z_REFCOUNTED(rv)) {
			zval_ptr_dtor(&rv);
		}

		zval_ptr_dtor(&ip);
	}
	BRK(state) = INSPECTOR_BREAK_RUNTIME;

	if (EG(exception)) {
#if PHP_VERSION_ID >= 70200 && PHP_VERSION_ID < 70300
		const zend_op *throw = EG(opline_before_exception);

		if (throw->result_type & IS_VAR|IS_TMP_VAR) {
			ZVAL_NULL(EX_VAR(throw->result.var));
		}
#endif
		return ZEND_USER_OPCODE_DISPATCH_TO | ZEND_HANDLE_EXCEPTION;
	}

	if (EX(opline) != instruction) {
		return ZEND_USER_OPCODE_CONTINUE;
	}

	return ZEND_USER_OPCODE_DISPATCH_TO | brk->opcode;
}

static zend_always_inline int php_inspector_break_vm_dispatch(zend_uchar opcode, zend_execute_data *execute_data) {
	user_opcode_handler_t php_inspector_break_vm_handler = NULL;

	switch (opcode) {
		case ZEND_DECLARE_CLASS: 
			php_inspector_break_vm_handler = 
				php_inspector_break_vm[PHP_INSPECTOR_BREAK_DECL_CLASS]; 
		break;

		case ZEND_DECLARE_INHERITED_CLASS:
			php_inspector_break_vm_handler = 
				php_inspector_break_vm[PHP_INSPECTOR_BREAK_DECL_INHERITED_CLASS]; 
		break;

		case ZEND_DECLARE_INHERITED_CLASS_DELAYED:
			php_inspector_break_vm_handler = 
				php_inspector_break_vm[PHP_INSPECTOR_BREAK_DECL_INHERITED_CLASS_DELAYED]; 
		break;

		case ZEND_DECLARE_ANON_CLASS:
			php_inspector_break_vm_handler = 
				php_inspector_break_vm[PHP_INSPECTOR_BREAK_DECL_ANON_CLASS]; 
		break;

		case ZEND_DECLARE_ANON_INHERITED_CLASS:
			php_inspector_break_vm_handler = 
				php_inspector_break_vm[PHP_INSPECTOR_BREAK_DECL_ANON_INHERITED_CLASS]; 
		break;

		case ZEND_DECLARE_FUNCTION:
			php_inspector_break_vm_handler = 
				php_inspector_break_vm[PHP_INSPECTOR_BREAK_DECL_FUNCTION];
		break;
	}

	if (UNEXPECTED(php_inspector_break_vm_handler)) {
		return php_inspector_break_vm_handler(execute_data);
	}

	return ZEND_USER_OPCODE_DISPATCH;
}

static int php_inspector_break_vm_persist(zend_execute_data *execute_data) {
	zval *op1 = RT_CONSTANT(&EX(func)->op_array, EX(opline)->op1);
	zval *op2 = RT_CONSTANT(&EX(func)->op_array, EX(opline)->op2);
	HashTable *table = 
		(EX(opline)->opcode == ZEND_DECLARE_FUNCTION) ? 
			CG(function_table) : CG(class_table);

	switch (EX(opline)->opcode) {
		case ZEND_DECLARE_CLASS:
		case ZEND_DECLARE_FUNCTION:
			if (zend_hash_exists(table, Z_STR_P(op2))) {
				EX(opline)++;

				return ZEND_USER_OPCODE_CONTINUE;
			}
		break;

		case ZEND_DECLARE_INHERITED_CLASS: 
		case ZEND_DECLARE_INHERITED_CLASS_DELAYED: {
			zend_class_entry *ce = 
				(zend_class_entry*) 
					zend_hash_find_ptr(CG(class_table), Z_STR_P(op1));
			zend_class_entry *parent = Z_CE_P(EX_VAR(EX(opline)->extended_value));

			if (instanceof_function(ce, parent)) {
				EX(opline)++;
				return ZEND_USER_OPCODE_CONTINUE;
			}
		} break;
	}

	return php_inspector_break_vm_dispatch(EX(opline)->opcode, execute_data);
}

PHP_MINIT_FUNCTION(inspector_break) {
	zend_class_entry ce;

	ZEND_INIT_MODULE_GLOBALS(inspector_break, php_inspector_break_globals_init, NULL);	

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "InspectorBreakPoint", php_inspector_break_methods);
	php_inspector_break_ce = 
		zend_register_internal_class(&ce);
	php_inspector_break_ce->create_object = php_inspector_break_create;

	memcpy(&php_inspector_break_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	php_inspector_break_handlers.offset = XtOffsetOf(php_inspector_break_t, std);
	php_inspector_break_handlers.free_obj = php_inspector_break_destroy;

	zend_set_user_opcode_handler(INSPECTOR_DEBUG_BREAK, php_inspector_break_handler);

	{
		user_opcode_handler_t *handler = php_inspector_break_vm;
		user_opcode_handler_t *last    = handler + 
			(sizeof(php_inspector_break_vm) / sizeof(user_opcode_handler_t));

		while (handler < last) {
			php_inspector_break_vm[(last - handler) - 1] = 
				zend_get_user_opcode_handler(
					php_inspector_break_vm_map[(last - handler) - 1]);
			zend_set_user_opcode_handler(
				php_inspector_break_vm_map[(last - handler) - 1], 
				php_inspector_break_vm_persist);
			
			handler++;
		}
	}

	return SUCCESS;
} 

PHP_MSHUTDOWN_FUNCTION(inspector_break)
{
	user_opcode_handler_t *handler = php_inspector_break_vm;
	user_opcode_handler_t *last    = handler + 
		(sizeof(php_inspector_break_vm) / sizeof(user_opcode_handler_t));

	while (handler < last) {
		zend_set_user_opcode_handler(
			php_inspector_break_vm_map[(last - handler) - 1], 
			php_inspector_break_vm[(last - handler) - 1]);
		handler++;
	}
	
	return SUCCESS;
}
/* }}} */

/* {{{ */
static void php_inspector_break_unset(zval *zv) {
	php_inspector_break_t *brk = Z_PTR_P(zv);
	php_inspector_instruction_t *instruction = 
		php_inspector_instruction_fetch(&brk->instruction);

	instruction->opline->opcode = brk->opcode;

	zend_vm_set_opcode_handler(instruction->opline);
} /* }}} */

/* {{{ */
PHP_RINIT_FUNCTION(inspector_break) 
{
	BRK(state) = INSPECTOR_BREAK_RUNTIME;

	zend_hash_init(&BRK(breaks), 8, NULL, php_inspector_break_unset, 0);

	return SUCCESS;
} /* }}} */

/* {{{ */
PHP_RSHUTDOWN_FUNCTION(inspector_break)
{
	BRK(state) = INSPECTOR_BREAK_SHUTDOWN;

	zend_hash_destroy(&BRK(breaks));

	return SUCCESS;
} /* }}} */
#endif
