--TEST--
InspectorInstruction getOpcode/getOpcodeName
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a) {}

$inspector = 
	new InspectorFunction("foo");

$opline = $inspector->getInstruction();

var_dump($opline->getOpcodeName(), 
	 $opline->getOpcode());
?>
--EXPECTF--
string(%d) "%s"
int(%d)
