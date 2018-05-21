--TEST--
InspectorInstruction getExtendedValue FE JMP
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;

$inspector = 
	new InspectorFunction(function($a, $b) {
	foreach ($a as $b) {
		
	}
});

$opline = $inspector
	->findFirstInstruction(InspectorInstruction::ZEND_FE_FETCH_R);

var_dump($opline->getExtendedValue());
?>
--EXPECTF--
int(%d)

