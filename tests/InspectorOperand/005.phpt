--TEST--
InspectorOperand getValue
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

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame) {
		$opline = $frame->getOpline();

		$op1 = $opline->getOperand(InspectorOpline::OP1);
		
		if ($op1->getValue($frame) == 1) {
			echo "OK";
		}
	}
};

$function(1, 2);
?>
--EXPECT--
OK
