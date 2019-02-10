--TEST--
InspectorInstruction getRelative
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a, $b) {
	$a + $b;
}

$inspector = new InspectorFunction("foo");

$opline = $inspector->getEntryInstruction();

printf("0x%X", $opline->getAddress());
?>
--EXPECTF--
0x%s
