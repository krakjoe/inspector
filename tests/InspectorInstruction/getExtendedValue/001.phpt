--TEST--
InspectorInstruction getExtendedValue
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;

$inspector = 
	new InspectorFunction(function($a, $b) {
	# 0 ZEND_RECV
	# 1 ZEND_RECV
	(string) $a; # 2 ZEND_CAST
});

$opline = 
	$inspector->getInstruction(2);

var_dump($opline->getOpcodeName());
var_dump($opline->getExtendedValue());
?>
--EXPECT--
string(9) "ZEND_CAST"
string(6) "string"

