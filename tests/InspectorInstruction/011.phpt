--TEST--
InspectorInstruction getRelative
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorFrame;
use Inspector\InspectorBreakPoint;

function foo($a, $b) {
	$a + $b;
}

$inspector = new InspectorFunction("foo");

$opline = $inspector->findLastInstruction(InspectorInstruction::ZEND_RETURN);

if ($opline->getRelative(-1) instanceof InspectorInstruction) {
	echo "OK";
}

foo(1, 2);
?>
--EXPECT--
OK
