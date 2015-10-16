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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_closures.h"
#include "zend_interfaces.h"

#include "ext/standard/info.h"
#include "ext/spl/spl_exceptions.h"
#include "php_inspector.h"

#include "scope.h"
#include "iterator.h"
#include "opline.h"
#include "operand.h"

/* {{{ */
PHP_METHOD(Scope, getStatics) {
	php_inspector_scope_t *scope = php_inspector_scope_this();
	
	if (scope->ops->static_variables) {
		RETURN_ARR(zend_array_dup(scope->ops->static_variables));
	}
}

PHP_METHOD(Scope, getConstants) {
	php_inspector_scope_t *scope = php_inspector_scope_this();

	if (scope->ops->last_literal) {
		uint32_t it = 0;

		array_init(return_value);
		for (it = 0; it < scope->ops->last_literal; it++) {
			if (add_next_index_zval(return_value, &scope->ops->literals[it]) == SUCCESS)
				Z_TRY_ADDREF(scope->ops->literals[it]);
		}
	}
} 

PHP_METHOD(Scope, getVariables) {
	php_inspector_scope_t *scope = php_inspector_scope_this();

	if (scope->ops->last_var) {
		uint32_t it = 0;
		
		array_init(return_value);
		for (it = 0; it < scope->ops->last_var; it++) {
			add_next_index_str(return_value, 
				zend_string_copy(scope->ops->vars[it]));
		}
	}
} /* }}} */

/* {{{ */
PHP_METHOD(File, __construct)
{
	zend_string *filename = NULL;
	zend_file_handle fh;
	zend_op_array *ops = NULL;

	if (0) {
InvalidArgumentException:
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
			"%s expects (string filename)",
			ZSTR_VAL(EX(func)->common.function_name));
		return;
	}

	switch (ZEND_NUM_ARGS()) {
		case 1: if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &filename) != SUCCESS) {
			return;
		} break;
		
		default:
			goto InvalidArgumentException;
	}

	if (zend_hash_exists(&EG(included_files), filename)) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot load already included file %s",
			ZSTR_VAL(filename));
		return;
	}

	if (php_stream_open_for_zend_ex(ZSTR_VAL(filename), &fh, USE_PATH|STREAM_OPEN_FOR_INCLUDE) != SUCCESS) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot open %s for parsing",
			ZSTR_VAL(filename));
		return;
	}

	if (!fh.opened_path) {
		fh.opened_path = zend_string_copy(filename);	
	}

	if (!zend_hash_add_empty_element(&EG(included_files), fh.opened_path)) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot add %s to included files table",
			ZSTR_VAL(filename));
		zend_file_handle_dtor(&fh);
		return;
	}

	ops = zend_compile_file(&fh, ZEND_REQUIRE);
	php_inspector_scope_construct(getThis(), (zend_function*) ops);
	zend_destroy_file_handle(&fh);
	destroy_op_array(ops);
	efree(ops);
}

static zend_function_entry php_inspector_file_methods[] = {
	PHP_ME(File, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_METHOD(Global, __construct)
{
	zend_string *name = NULL;
	zend_function *function = NULL;

	if (0) {
InvalidArgumentException:
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
			"%s expects (string filename)",
			ZSTR_VAL(EX(func)->common.function_name));
		return;
	}

	switch (ZEND_NUM_ARGS()) {
		case 1: if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) != SUCCESS) {
			return;
		} break;
		
		default:
			goto InvalidArgumentException;
	}

	if (!(function = php_inspector_scope_find(NULL, name))) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot find function %s",
			ZSTR_VAL(name));
		return;
	}

	php_inspector_scope_construct(getThis(), function);
}

static zend_function_entry php_inspector_global_methods[] = {
	PHP_ME(Global, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_METHOD(Closure, __construct)
{
	zval *closure;

	if (0) {
InvalidArgumentException:
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
			"%s expects (Closure closure)",
			ZSTR_VAL(EX(func)->common.function_name));
		return;
	}

	switch (ZEND_NUM_ARGS()) {
		case 1: if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &closure, zend_ce_closure) != SUCCESS) {
			return;
		} break;
		
		default:
			goto InvalidArgumentException;
	}

	php_inspector_scope_construct(getThis(), (zend_function*) zend_get_closure_method_def(closure));
}

static zend_function_entry php_inspector_closure_methods[] = {
	PHP_ME(Closure, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
PHP_METHOD(Method, __construct)
{
	zend_class_entry *clazz = NULL;
	zend_string *method = NULL;
	zend_function *function = NULL;

	if (0) {
InvalidArgumentException:
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
			"%s expects (string class, string method)",
			ZSTR_VAL(EX(func)->common.function_name));
		return;
	}

	switch (ZEND_NUM_ARGS()) {
		case 1: if (zend_parse_parameters(ZEND_NUM_ARGS(), "CS", &clazz, &method) != SUCCESS) {
			return;
		} break;
		
		default:
			goto InvalidArgumentException;
	}

	if (!(function = php_inspector_scope_find(clazz, method))) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot find function %s::%s",
			ZSTR_VAL(clazz->name), ZSTR_VAL(method));
		return;
	}

	php_inspector_scope_construct(getThis(), function);
}

static zend_function_entry php_inspector_method_methods[] = {
	PHP_ME(Method, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
static zend_function_entry php_inspector_scope_methods[] = {
	PHP_ABSTRACT_ME(Scope, __construct, NULL)
	PHP_ME(Scope, getStatics, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getConstants, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Scope, getVariables, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
static zend_function_entry php_inspector_opline_methods[] = {
	PHP_ME(Opline, getType, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Opline, getOperand, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Opline, getExtendedValue, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
static zend_function_entry php_inspector_operand_methods[] = {
	PHP_ME(Operand, isUnused, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isExtendedTypeUnused, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isCompiledVariable, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isTemporaryVariable, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isVariable, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isConstant, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, isJumpTarget, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, getWhich, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, getValue, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, getName, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Operand, getNumber, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(inspector)
{
	zend_class_entry ce;
	
	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Scope", php_inspector_scope_methods);
	php_inspector_scope_ce = 
		zend_register_internal_class(&ce);
	php_inspector_scope_ce->create_object = php_inspector_scope_create;
	php_inspector_scope_ce->get_iterator  = php_inspector_iterate;

	zend_class_implements(php_inspector_scope_ce, 1, zend_ce_traversable);

	memcpy(&php_inspector_scope_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_scope_handlers.offset = XtOffsetOf(php_inspector_scope_t, std);
	php_inspector_scope_handlers.free_obj = php_inspector_scope_destroy;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "File", php_inspector_file_methods);
	php_inspector_file_ce = 
		zend_register_internal_class_ex(&ce, php_inspector_scope_ce);

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Global", php_inspector_global_methods);
	php_inspector_global_ce = 
		zend_register_internal_class_ex(&ce, php_inspector_scope_ce);

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Closure", php_inspector_closure_methods);
	php_inspector_closure_ce = 
		zend_register_internal_class_ex(&ce, php_inspector_scope_ce);

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

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Operand", php_inspector_operand_methods);
	php_inspector_operand_ce = 
		zend_register_internal_class(&ce);
	php_inspector_operand_ce->create_object = php_inspector_operand_create;

	memcpy(&php_inspector_operand_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_operand_handlers.offset = XtOffsetOf(php_inspector_operand_t, std);
	php_inspector_operand_handlers.free_obj = php_inspector_operand_destroy;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(inspector)
{
#if defined(COMPILE_DL_INSPECTOR) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(inspector)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "inspector support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ inspector_module_entry
 */
zend_module_entry inspector_module_entry = {
	STANDARD_MODULE_HEADER,
	"inspector",
	NULL,
	PHP_MINIT(inspector),
	NULL,
	PHP_RINIT(inspector),
	NULL,
	PHP_MINFO(inspector),
	PHP_INSPECTOR_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_INSPECTOR
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif
ZEND_GET_MODULE(inspector)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
