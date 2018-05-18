--TEST--
InspectorOperand getNumber t/var
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

$function = function($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getInstruction(2);

$result = $opline->getOperand(InspectorOperand::RESULT);

if ($result->isTemporaryVariable() &&
    $result->getNumber() == 1) {
	echo "OK";
}
?>
--EXPECT--
OK
