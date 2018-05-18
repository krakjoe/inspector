--TEST--
InspectorFrame getInstruction on InspectorBreakPoint
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($a, $b) {
	$a + $b;
};

$inspector = new InspectorFunction($function);

$opline = $inspector->getInstruction(2);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		$opline = $frame->getInstruction();

		if ($opline->getOpcode() == InspectorInstruction::ADD) {
			echo "OK";
		}
	}
};

$function(1, 2);
?>
--EXPECT--
OK

