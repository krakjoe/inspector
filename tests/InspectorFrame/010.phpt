--TEST--
InspectorOpline getBreakPoint
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($a, $b) {
	$a + $b;
};

$inspector = new InspectorFunction($function);

$opline = $inspector->getOpline(2);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		$opline = $frame->getOpline();

		if ($opline->getBreakPoint() == $this) {
			echo "OK";
		}
	}
};

$function(1, 2);
?>
--EXPECT--
OK

