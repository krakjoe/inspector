--TEST--
InspectorFrame getInstruction on InspectorBreakPoint
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

		var_dump($opline->getOpcode(), 
			 $opline->getOpcodeName());
	}
};

$function(1, 2);
?>
--EXPECTF--
int(%d)
string(%d) "ZEND_%s"

