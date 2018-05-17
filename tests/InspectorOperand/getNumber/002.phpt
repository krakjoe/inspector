--TEST--
InspectorOperand getNumber t/var
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

$result = $opline->getOperand(InspectorOpline::RESULT);

if ($result->isTemporaryVariable() &&
    $result->getNumber() == 1) {
	echo "OK";
}
?>
--EXPECT--
OK
