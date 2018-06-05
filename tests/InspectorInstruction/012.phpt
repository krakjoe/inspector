--TEST--
InspectorInstruction getRelative
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$func = function($a, $b) {
	$a + $b;
};

$inspector = new InspectorFunction($func);

$opline = $inspector->getEntryInstruction();

printf("0x%X", $opline->getAddress());
?>
--EXPECTF--
0x%s
