--TEST--
InspectorOpline getExtendedValue
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorOperand;

$inspector = 
	new InspectorFunction(function($a, $b) {
	# 0 ZEND_RECV
	# 1 ZEND_RECV
	(string) $a; # 2 ZEND_CAST
});

$opline = 
	$inspector->getOpline(2);

var_dump($opline->getOpcodeName());
var_dump($opline->getExtendedValue());
?>
--EXPECT--
string(9) "ZEND_CAST"
string(6) "string"

