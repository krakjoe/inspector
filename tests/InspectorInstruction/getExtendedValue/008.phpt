--TEST--
InspectorInstruction getExtendedValue ASSIGN MUL
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$function = function($mul) {
	$assign["dim"] *= $mul;
};

$inspector = new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_ASSIGN_MUL);

if ($opline->getExtendedValue() == InspectorInstruction::ZEND_ASSIGN_DIM) {
	echo "OK";
}
?>
--EXPECT--
OK

