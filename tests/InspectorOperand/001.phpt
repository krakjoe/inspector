--TEST--
InspectorOperand isUnused
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getOpline(2);

if (!$opline->getOperand(InspectorOpline::OP1)->isUnused()) {
	echo "OK";
}
?>
--EXPECT--
OK
