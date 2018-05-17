--TEST--
InspectorOperand isExtendedTypeUnused
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

if (!$opline->getOperand(InspectorOpline::OP1)->isExtendedTypeUnused()) {
	echo "OK";
}
?>
--EXPECT--
OK
