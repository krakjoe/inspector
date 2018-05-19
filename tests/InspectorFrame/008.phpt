--TEST--
InspectorFrame getVariable out of bounds
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($a, $b) {
	$a + $b;
};

$inspector = new InspectorFunction($function);

$opline = $inspector->getInstruction(2);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		if (!$frame->getVariable(42)) {
			echo "OK";
		}
	}
};

$function(1, 2);
?>
--EXPECT--
OK

