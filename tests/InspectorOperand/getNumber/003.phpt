--TEST--
InspectorOperand getNumber variable
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

$function = function() {
	$a = sprintf("hello");
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getInstruction(2);

$result = $opline->getOperand(InspectorOperand::RESULT);

if ($result->isVariable() || $result->isTemporaryVariable() &&
    $result->getNumber() > -1) {
	echo "OK";
} else {
	var_dump($result->isVariable(), $result->isTemporaryVariable(), $result->getNumber());
}
?>
--EXPECT--
OK
