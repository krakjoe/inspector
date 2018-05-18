--TEST--
InspectorOperand getValue
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

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame) {
		$opline = $frame->getInstruction();

		$op1 = $opline->getOperand(InspectorOperand::OP1);
		
		if ($op1->getValue($frame) == 1) {
			echo "OK";
		}
	}
};

$function(1, 2);
?>
--EXPECT--
OK
