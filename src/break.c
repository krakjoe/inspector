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

#include "zend_closures.h"
#include "zend_exceptions.h"
#include "zend_generators.h"
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
	INSPECTOR_BREAK_EXCEPTION,
	INSPECTOR_BREAK_SHUTDOWN,
} php_inspector_break_state_t;

ZEND_BEGIN_MODULE_GLOBALS(inspector_break)
	php_inspector_break_state_t state;
	struct {
		zval                handler;
		zend_object         *object;
	} exception;
	HashTable                   breaks;
ZEND_END_MODULE_GLOBALS(inspector_break)

ZEND_DECLARE_MODULE_GLOBALS(inspector_break);

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
			zend_hash_index_del(
				&BRK(breaks), (zend_ulong) instruction->opline);
		}
		zval_ptr_dtor(&brk->instruction);
	}

	zend_object_std_dtor(&brk->std);
}

static inline zend_bool php_inspector_break_enable(php_inspector_break_t *brk) {
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_fetch(&brk->instruction);
	php_inspector_function_t *function =
		php_inspector_function_from(&instruction->function);

	if (!function->function) {
		return 0;
	}

	if (instruction->opline->opcode == INSPECTOR_DEBUG_BREAK) {
		return 0;
	}

	instruction->opline->opcode = INSPECTOR_DEBUG_BREAK;

	zend_vm_set_opcode_handler(instruction->opline);

	return 1;
}

static inline zend_bool php_inspector_break_enabled(php_inspector_break_t *brk) {
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_fetch(&brk->instruction);

	return instruction->opline->opcode == INSPECTOR_DEBUG_BREAK;
}

static inline zend_bool php_inspector_break_disable(php_inspector_break_t *brk) {
	php_inspector_instruction_t *instruction =
		php_inspector_instruction_fetch(&brk->instruction);

	if (instruction->opline->opcode != INSPECTOR_DEBUG_BREAK) {
		return 0;
	}

	instruction->opline->opcode = brk->opcode;

	zend_vm_set_opcode_handler(instruction->opline);

	return 1;
}

void php_inspector_breaks_purge(zend_function *ops) {
	zend_op *opline, *end;

	if (!ZEND_USER_CODE(ops->type)) {
		return;
	}
	
	opline = ops->op_array.opcodes;
	end    = opline + ops->op_array.last;

	while (opline < end) {
		zend_hash_index_del(
			&BRK(breaks), (zend_ulong) opline);
		opline++;
	}
}

php_inspector_break_t* php_inspector_break_find_opline(const zend_op *op) {
	return zend_hash_index_find_ptr(&BRK(breaks), (zend_ulong) op);
}

php_inspector_break_t* php_inspector_break_find_ptr(php_inspector_instruction_t *instruction) {
	return php_inspector_break_find_opline(instruction->opline);
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
	php_inspector_instruction_t *instruction;	
	zval *in = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "O", &in, php_inspector_instruction_ce) != SUCCESS) {
		return;
	}

	ZVAL_COPY(&brk->instruction, in);

	instruction = 
		php_inspector_instruction_fetch(&brk->instruction);

	brk->opcode = instruction->opline->opcode;

	if (!zend_hash_index_add_ptr(&BRK(breaks), (zend_ulong) instruction->opline, brk)) {
		zend_throw_exception_ex(reflection_exception_ptr, 0, 
			"already a InspectorBreakPoint on that instruction");
		return;
	}

	php_inspector_break_enable(brk);
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

static PHP_METHOD(InspectorBreakPoint, onException)
{
	zval *handler = NULL;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "O", &handler, zend_ce_closure) != SUCCESS) {
		return;
	}

	if (Z_REFCOUNTED(BRK(exception).handler)) {
		zval_ptr_dtor(&BRK(exception).handler);
	}

	ZVAL_COPY(&BRK(exception).handler, handler);
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
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(InspectorBreakPoint_hit_arginfo, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, frame, Inspector\\InspectorFrame, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(InspectorBreakPoint_onException_arginfo, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, handler, Closure, 0)
ZEND_END_ARG_INFO()

static zend_function_entry php_inspector_break_methods[] = {
	PHP_ME(InspectorBreakPoint, __construct, InspectorBreakPoint_construct_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorBreakPoint, getInstruction, InspectorBreakPoint_getInstruction_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorBreakPoint, disable, InspectorBreakPoint_switch_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorBreakPoint, enable, InspectorBreakPoint_switch_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorBreakPoint, isEnabled, InspectorBreakPoint_switch_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(InspectorBreakPoint, onException, InspectorBreakPoint_onException_arginfo, ZEND_ACC_STATIC|ZEND_ACC_PUBLIC)
	PHP_ABSTRACT_ME(InspectorBreakPoint, hit, InspectorBreakPoint_hit_arginfo)
	PHP_FE_END
};

static zend_always_inline zend_bool php_inspector_break_exception_caught(zend_execute_data *execute_data, zend_object *exception) {
	zend_op_array *ops = (zend_op_array*) EX(func);
	const zend_op *op  = EX(opline) >= EG(exception_op) && EX(opline) < (EG(exception_op) + 3) ?
		EG(opline_before_exception) : EX(opline);
	uint32_t       num = op - ops->opcodes;
	zend_try_catch_element *it = ops->try_catch_array,
			       *end = ops->try_catch_array + ops->last_try_catch;

	while (it < end) {
		zend_op *check;
		uint32_t catch = it->catch_op,
			 finally = it->finally_op;

		if (it->try_op <= num) {
			break;
		}

		if (num <= catch || num <= finally) {
			if (finally) {
				return 1;
			}
			
			check = ops->opcodes + it->catch_op;

			do {
#if PHP_VERSION_ID >= 70300
				zend_class_entry *ce = CACHED_PTR(check->extended_value & ~ZEND_LAST_CATCH);

				if (!ce) {
					ce = zend_fetch_class_by_name(
						Z_STR_P(RT_CONSTANT(ops, check->op1)), 
# if PHP_VERSION_ID >= 70400
						Z_STR_P(RT_CONSTANT(check, check->op1) + 1), ZEND_FETCH_CLASS_NO_AUTOLOAD);
# else
						RT_CONSTANT(check, check->op1) + 1, ZEND_FETCH_CLASS_NO_AUTOLOAD);
# endif

					CACHE_PTR(check->extended_value &~ ZEND_LAST_CATCH, ce);
				}
#else
				zend_class_entry *ce = CACHED_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(check->op1)));

				if (!ce) {
					ce = zend_fetch_class_by_name(
						Z_STR_P(RT_CONSTANT(ops, check->op1)), 
						RT_CONSTANT(ops, check->op1) + 1, ZEND_FETCH_CLASS_NO_AUTOLOAD);

					CACHE_PTR(Z_CACHE_SLOT_P(EX_CONSTANT(check->op1)), ce);
				}
#endif

				if (ce == exception->ce ||
				    (ce && instanceof_function(exception->ce, ce))) {
					return 1;
				}


#if PHP_VERSION_ID >= 70300
				if (check->extended_value & ZEND_LAST_CATCH) {
					return 0;
				}

				check = OP_JMP_ADDR(check, check->op2);
#else
				catch += check->extended_value / sizeof(zend_op);
#endif

#if PHP_VERSION_ID >= 70300
			} while (1);
#else
			} while (!check->result.num);
#endif

			return 0;
		}

		it++;
	}

	if (op->opcode == INSPECTOR_DEBUG_BREAK) {
		php_inspector_break_t *brk = 
			php_inspector_break_find_opline(op);

		return brk->opcode == ZEND_CATCH;
	}

	return op->opcode == ZEND_CATCH;
}

static zend_always_inline zend_bool php_inspector_break_exception_handled(zend_execute_data *execute_data, zend_object *exception) {
	do {
		execute_data = zend_generator_check_placeholder_frame(execute_data);

		if (!EX(func) || !ZEND_USER_CODE(EX(func)->type)) {
			continue;
		}

		if (php_inspector_break_exception_caught(execute_data, exception)) {
			return 1;
		}
	} while ((execute_data = EX(prev_execute_data)) != NULL);

	return 0;
}

static zend_always_inline void php_inspector_break_hit(php_inspector_break_t *brk, zend_fcall_info *fci, zend_fcall_info_cache *fcc, zend_execute_data *execute_data) {
	zval rv;
	zval fp;

	if (fci->size == 0) {
		fci->size = sizeof(zend_fcall_info);
		fci->object = &brk->std;
#if PHP_VERSION_ID < 70300
		fcc->initialized = 1;
#endif
		fcc->object = brk->cache.fci.object;
		fcc->function_handler = (zend_function*)
			zend_hash_find_ptr(
				&brk->std.ce->function_table, PHP_INSPECTOR_STRING_HIT);
	}

	ZVAL_NULL(&rv);

	php_inspector_frame_factory(execute_data, &fp);

	fci->param_count = 1;
	fci->params = &fp;
	fci->retval = &rv;

	if (zend_call_function(fci, fcc) != SUCCESS) {
		/* do something */
	}

	if (Z_REFCOUNTED(rv)) {
		zval_ptr_dtor(&rv);
	}

	zval_ptr_dtor(&fp);
}

zend_bool php_inspector_break_handle_exception(zend_execute_data *execute_data) {
	if (BRK(state) != INSPECTOR_BREAK_EXCEPTION &&
	   !php_inspector_break_exception_handled(execute_data, EG(exception)) && !Z_ISUNDEF(BRK(exception).handler)) {
		BRK(state) = INSPECTOR_BREAK_EXCEPTION;
		{
			zval fp, ex, rv;
			zend_fcall_info fci = empty_fcall_info;
			zend_fcall_info_cache fcc = empty_fcall_info_cache;
			const zend_op *opline = EX(opline);

			if (zend_fcall_info_init(&BRK(exception).handler, 0, &fci, &fcc, NULL, NULL) != SUCCESS) {
				BRK(state) = INSPECTOR_BREAK_RUNTIME;
				return 0;
			}

			EX(opline) = EG(opline_before_exception);

			BRK(exception).object = EG(exception);
			EG(exception) = NULL;

			ZVAL_OBJ(&ex, BRK(exception).object);

			php_inspector_frame_factory(execute_data, &fp);

			zend_fcall_info_argn(&fci, 2, &fp, &ex);

			if (zend_fcall_info_call(&fci, &fcc, &rv, NULL) != SUCCESS) {
				/* do something */
			}

			zend_fcall_info_args_clear(&fci, 1);

			if (EG(exception)) {
				zend_exception_set_previous(
					EG(exception), BRK(exception).object);		
				EG(exception) = BRK(exception).object;
			} else {
				OBJ_RELEASE(BRK(exception).object);
			}

			zval_ptr_dtor(&fp);

			if (Z_REFCOUNTED(rv)) {
				zval_ptr_dtor(&rv);
			}

			EX(opline) = opline;
		}
		BRK(state) = INSPECTOR_BREAK_RUNTIME;

		return 1;
	}

	return 0;
}

static int php_inspector_break_handler(zend_execute_data *execute_data) {
	zend_op *instruction = (zend_op *) EX(opline);
	php_inspector_break_t *brk = 
		zend_hash_index_find_ptr(&BRK(breaks), (zend_ulong) instruction);

	BRK(state) = INSPECTOR_BREAK_HANDLER;
	{
		php_inspector_break_hit(
			brk, 
			&brk->cache.fci, 
			&brk->cache.fcc, 
			execute_data);
	}
	BRK(state) = INSPECTOR_BREAK_RUNTIME;

	if (EG(exception)) {
#if PHP_VERSION_ID >= 70200
		const zend_op *throw = EG(opline_before_exception);

		if (throw->result_type & (IS_VAR|IS_TMP_VAR)) {
			ZVAL_NULL(EX_VAR(throw->result.var));
		}
#endif

		if (!php_inspector_break_handle_exception(execute_data)) {
			return ZEND_USER_OPCODE_DISPATCH_TO | ZEND_HANDLE_EXCEPTION;	
		}
	}

	if (EX(opline) != instruction) {
		return ZEND_USER_OPCODE_CONTINUE;
	}

	return ZEND_USER_OPCODE_DISPATCH_TO | brk->opcode;
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

	return SUCCESS;
} 

PHP_MSHUTDOWN_FUNCTION(inspector_break)
{	
	return SUCCESS;
}
/* }}} */

/* {{{ */
static void php_inspector_break_unset(zval *zv) {
	php_inspector_break_t *brk = Z_PTR_P(zv);
	php_inspector_instruction_t *instruction = 
		php_inspector_instruction_fetch(&brk->instruction);
	php_inspector_function_t *function =
		php_inspector_function_from(&instruction->function);

	if (BRK(state) != INSPECTOR_BREAK_RUNTIME) {
		return;
	}

	if (!function->expired) {
		ZEND_ASSERT(brk->opcode != 255);

		instruction->opline->opcode = brk->opcode;

		zend_vm_set_opcode_handler(instruction->opline);
	}
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

	if (Z_REFCOUNTED(BRK(exception).handler)) {
		zval_ptr_dtor(&BRK(exception).handler);
	}

	return SUCCESS;
} /* }}} */
#endif
