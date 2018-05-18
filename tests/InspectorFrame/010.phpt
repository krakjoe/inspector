--TEST--
InspectorInstruction getBreakPoint
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
		$opline = $frame->getInstruction();

		if ($opline->getBreakPoint() == $this) {
			echo "OK";
		}
	}
};

$function(1, 2);
?>
--EXPECT--
OK

