--TEST--
InspectorInstruction getExtendedValue ASSIGN ADD
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$function = function($add) {
	$assign["dim"] += $add;
};

$inspector = new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_ASSIGN_ADD);

if ($opline->getExtendedValue() == InspectorInstruction::ZEND_ASSIGN_DIM) {
	echo "OK";
}
?>
--EXPECT--
OK

