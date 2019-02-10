--TEST--
InspectorOperand getValue
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

function foo($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction("foo");

$opline = $inspector->getInstruction(2);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame) {
		$opline = $frame->getInstruction();

		$op1 = $opline->getOperand(InspectorOperand::OP1);
		
		if ($op1->getValue($frame) == 1) {
			echo "OK";
		}
	}
};

foo(1, 2);
?>
--EXPECT--
OK
