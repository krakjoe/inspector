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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_closures.h"

#include "ext/standard/info.h"

#include "php_inspector.h"
#include "src/strings.h"
#include "src/file.h"
#include "src/class.h"
#include "src/method.h"
#include "src/function.h"
#include "src/instruction.h"
#include "src/operand.h"

#include "src/break.h"
#include "src/frame.h"

static void (*zend_execute_function)(zend_execute_data *);

ZEND_DECLARE_MODULE_GLOBALS(inspector);

static void php_inspector_globals_init(zend_inspector_globals *PIG) {
	memset(PIG, 0, sizeof(*PIG));
}

static zend_op_array* php_inspector_execute(zend_execute_data *execute_data) {
	zend_op_array *ops = &EX(func)->op_array;
	zend_string   *name = ops->filename;
	HashTable     *pending;

	if (UNEXPECTED(zend_hash_num_elements(&PIG(pending).file))) {
		pending = php_inspector_table(
				PHP_INSPECTOR_ROOT_PENDING, 
				PHP_INSPECTOR_TABLE_FILE, 
				name, 0);

		if (UNEXPECTED(pending)) {
			zend_hash_apply_with_argument(
				pending, 
				(apply_func_arg_t) 
					php_inspector_function_resolve, ops);

			php_inspector_table_drop(
				PHP_INSPECTOR_ROOT_PENDING, 
				PHP_INSPECTOR_TABLE_FILE, 
				ops->filename);
		}
	}

	ZEND_HASH_FOREACH_STR_KEY_PTR(php_inspector_table_select(
				PHP_INSPECTOR_ROOT_PENDING, 
				PHP_INSPECTOR_TABLE_FUNCTION), name, pending) {

		zend_function *function = 
			(zend_function*)
				zend_hash_find_ptr(EG(function_table), name);

		if (!function) {
			continue;
		}

		if (ZEND_HASH_GET_APPLY_COUNT(pending) == 0) {
			ZEND_HASH_INC_APPLY_COUNT(pending);

			zend_hash_apply_with_argument(
				pending, 
				(apply_func_arg_t) 
					php_inspector_function_resolve, (zend_op_array*) function);

			ZEND_HASH_DEC_APPLY_COUNT(pending);

			php_inspector_table_drop(
				PHP_INSPECTOR_ROOT_PENDING, 
				PHP_INSPECTOR_TABLE_FUNCTION, name);
		}
	} ZEND_HASH_FOREACH_END();

zend_execute_function_forward:
	zend_execute_function(execute_data);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(inspector)
{
	ZEND_INIT_MODULE_GLOBALS(inspector, php_inspector_globals_init, NULL);

	PHP_MINIT(inspector_instruction_interface)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_strings)(INIT_FUNC_ARGS_PASSTHRU);

	PHP_MINIT(inspector_class)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_method)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_function)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_file)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_instruction)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_operand)(INIT_FUNC_ARGS_PASSTHRU);

	PHP_MINIT(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_frame)(INIT_FUNC_ARGS_PASSTHRU);

	zend_execute_function = zend_execute_ex;
	zend_execute_ex = php_inspector_execute;

	return SUCCESS;
}
/* }}} */

/* {{{ */
PHP_MSHUTDOWN_FUNCTION(inspector)
{
	PHP_MSHUTDOWN(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MSHUTDOWN(inspector_strings)(INIT_FUNC_ARGS_PASSTHRU);

	zend_execute_ex = zend_execute_function;

	return SUCCESS;
} /* }}} */

static void php_inspector_table_free(zval *zv) {
	zend_hash_destroy(Z_PTR_P(zv));
	efree(Z_PTR_P(zv));
}

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(inspector)
{
#if defined(COMPILE_DL_INSPECTOR) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	PHP_RINIT(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);

	{
		zend_hash_init(&PIG(pending).file, 8, NULL, php_inspector_table_free, 0);
		zend_hash_init(&PIG(pending).function, 8, NULL, php_inspector_table_free, 0);
		zend_hash_init(&PIG(pending).class, 8, NULL, php_inspector_table_free, 0);

		zend_hash_init(&PIG(registered).file, 8, NULL, php_inspector_table_free, 0);
		zend_hash_init(&PIG(registered).function, 8, NULL, php_inspector_table_free, 0);
		zend_hash_init(&PIG(registered).class, 8, NULL, php_inspector_table_free, 0);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ */
PHP_RSHUTDOWN_FUNCTION(inspector)
{
	{
		zend_hash_destroy(&PIG(pending).class);
		zend_hash_destroy(&PIG(pending).function);
		zend_hash_destroy(&PIG(pending).file);

		zend_hash_destroy(&PIG(registered).class);
		zend_hash_destroy(&PIG(registered).function);
		zend_hash_destroy(&PIG(registered).file);
	}

	PHP_RSHUTDOWN(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);

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

static const zend_module_dep inspector_deps[] = {
	ZEND_MOD_REQUIRED("Reflection")
        ZEND_MOD_END
};

/* {{{ inspector_module_entry
 */
zend_module_entry inspector_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	inspector_deps,
	"inspector",
	NULL,
	PHP_MINIT(inspector),
	PHP_MSHUTDOWN(inspector),
	PHP_RINIT(inspector),
	PHP_RSHUTDOWN(inspector),
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
