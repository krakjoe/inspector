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

#include "inspector.h"
#include "iterator.h"
#include "node.h"
#include "operand.h"

/* {{{ */
PHP_METHOD(Inspector, __construct)
{
	zend_class_entry *scope = NULL;
	zval *function = NULL;
	zend_function *found = NULL;

	if (0) {
InvalidArgumentException:
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
			"%s expects (Closure closure) or (string function) or (string class, string method)",
			ZSTR_VAL(EX(func)->common.function_name));
		return;
	}

	switch (ZEND_NUM_ARGS()) {
		case 1: if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &function) != SUCCESS) {
			return;
		} break;

		case 2: if (zend_parse_parameters(ZEND_NUM_ARGS(), "Cz", &scope, &function) != SUCCESS) {
			return;
		} break;
		
		default:
			goto InvalidArgumentException;
	}

	if (Z_TYPE_P(function) == IS_STRING) {
		found = php_inspector_find(scope, Z_STR_P(function));	
	} else if (Z_TYPE_P(function) == IS_OBJECT && instanceof_function(Z_OBJCE_P(function), zend_ce_closure)) {
		found = (zend_function*) zend_get_closure_method_def(function);
	} else goto InvalidArgumentException;

	php_inspector_construct(getThis(), found);
}

PHP_METHOD(Inspector, getStatics) {
	php_inspector_t *inspector = php_inspector_this();
	
	if (inspector->ops->static_variables) {
		RETURN_ARR(zend_array_dup(inspector->ops->static_variables));
	}
}

PHP_METHOD(Inspector, getConstants) {
	php_inspector_t *inspector = php_inspector_this();

	if (inspector->ops->last_literal) {
		uint32_t it = 0;

		array_init(return_value);
		for (it = 0; it < inspector->ops->last_literal; it++) {
			if (add_next_index_zval(return_value, &inspector->ops->literals[it]) == SUCCESS)
				Z_TRY_ADDREF(inspector->ops->literals[it]);
		}
	}
} 

PHP_METHOD(Inspector, getVariables) {
	php_inspector_t *inspector = php_inspector_this();

	if (inspector->ops->last_var) {
		uint32_t it = 0;
		
		array_init(return_value);
		for (it = 0; it < inspector->ops->last_var; it++) {
			add_next_index_str(return_value, 
				zend_string_copy(inspector->ops->vars[it]));
		}
	}
} /* }}} */

/* {{{ */
PHP_METHOD(FileInspector, __construct)
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

	zend_destroy_file_handle(&fh);
	
	php_inspector_construct(getThis(), (zend_function*) ops);

	destroy_op_array(ops);
	efree(ops);
} 

static zend_function_entry php_inspector_file_methods[] = {
	PHP_ME(FileInspector, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
static zend_function_entry php_inspector_methods[] = {
	PHP_ME(Inspector, __construct, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Inspector, getStatics, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Inspector, getConstants, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Inspector, getVariables, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ */
static zend_function_entry php_inspector_node_methods[] = {
	PHP_ME(Node, getType, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Node, getOperand, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Node, getExtendedValue, NULL, ZEND_ACC_PUBLIC)
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
	
	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Inspector", php_inspector_methods);
	php_inspector_ce = 
		zend_register_internal_class(&ce);
	php_inspector_ce->create_object = php_inspector_create;
	php_inspector_ce->get_iterator  = php_inspector_iterate;
	zend_class_implements(php_inspector_ce, 1, zend_ce_traversable);

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "FileInspector", php_inspector_file_methods);
	php_inspector_file_ce = 
		zend_register_internal_class_ex(&ce, php_inspector_ce);

	memcpy(&php_inspector_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_handlers.offset = XtOffsetOf(php_inspector_t, std);
	php_inspector_handlers.free_obj = php_inspector_destroy;

	INIT_NS_CLASS_ENTRY(ce, "Inspector", "Node", php_inspector_node_methods);
	php_inspector_node_ce = 
		zend_register_internal_class(&ce);
	php_inspector_node_ce->create_object = php_inspector_node_create;
	zend_declare_class_constant_long(php_inspector_node_ce, ZEND_STRL("OP1"), PHP_INSPECTOR_NODE_OP1);
	zend_declare_class_constant_long(php_inspector_node_ce, ZEND_STRL("OP2"), PHP_INSPECTOR_NODE_OP2);
	zend_declare_class_constant_long(php_inspector_node_ce, ZEND_STRL("RESULT"), PHP_INSPECTOR_NODE_RESULT);

	memcpy(&php_inspector_node_handlers, 
		zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_inspector_node_handlers.offset = XtOffsetOf(php_inspector_node_t, std);
	php_inspector_node_handlers.free_obj = php_inspector_node_destroy;

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
