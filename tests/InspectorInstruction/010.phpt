--TEST--
InspectorInstruction getOpcodeName on BreakPoint instruction
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorFrame;
use Inspector\InspectorBreakPoint;

$func = function($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($func);

$opline = 
	$inspector->findFirstInstruction(InspectorInstruction::ZEND_ADD);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame) {
		if ($frame->getInstruction()->getOpcodeName() == "ADD") {
			echo "OK";
		}
	}
};

$func(1, 2);
?>
--EXPECT--
OK
