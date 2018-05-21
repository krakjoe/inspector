--TEST--
InspectorInstruction getExtendedValue ASSIGN MOD
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$function = function($mod) {
	$assign["dim"] %= $mod;
};

$inspector = new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_ASSIGN_MOD);

if ($opline->getExtendedValue() == InspectorInstruction::ZEND_ASSIGN_DIM) {
	echo "OK";
}
?>
--EXPECT--
OK

