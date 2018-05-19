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

#include "src/class.h"
#include "src/method.h"
#include "src/function.h"
#include "src/instruction.h"
#include "src/operand.h"

#include "src/break.h"
#include "src/frame.h"

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(inspector)
{
	PHP_MINIT(inspector_instruction_interface)(INIT_FUNC_ARGS_PASSTHRU);

	PHP_MINIT(inspector_class)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_method)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_function)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_instruction)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_operand)(INIT_FUNC_ARGS_PASSTHRU);

	PHP_MINIT(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(inspector_frame)(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */

/* {{{ */
PHP_MSHUTDOWN_FUNCTION(inspector)
{
	PHP_MSHUTDOWN(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);
	
	return SUCCESS;
} /* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(inspector)
{
#if defined(COMPILE_DL_INSPECTOR) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	PHP_RINIT(inspector_break)(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */

/* {{{ */
PHP_RSHUTDOWN_FUNCTION(inspector)
{
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
