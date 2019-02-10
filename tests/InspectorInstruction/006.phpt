--TEST--
InspectorInstruction getLine
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a, $b) {}

$inspector = new InspectorFunction("foo");

$opline = 
	$inspector->getInstruction();

var_dump($opline->getLine());
?>
--EXPECT--
int(5)

