--TEST--
InspectorInstruction getRelative
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorFrame;
use Inspector\InspectorBreakPoint;

$func = function($a, $b) {
	$a + $b;
};

$inspector = new InspectorFunction($func);

$opline = $inspector->findLastInstruction(InspectorInstruction::ZEND_RETURN);

if ($opline->getRelative(-1) instanceof InspectorInstruction) {
	echo "OK";
}

$func(1, 2);
?>
--EXPECT--
OK
