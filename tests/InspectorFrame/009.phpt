--TEST--
InspectorOpline getOpcode/getOpcodeName on InspectorBreakPoint
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

		var_dump($opline->getOpcode(), 
			 $opline->getOpcodeName());
	}
};

$function(1, 2);
?>
--EXPECTF--
int(%d)
string(%d) "ZEND_%s"

