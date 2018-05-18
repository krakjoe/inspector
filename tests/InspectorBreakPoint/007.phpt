--TEST--
InspectorBreakPoint hit throws
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getInstruction(2);

class BreakPoint extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		throw new RuntimeException();
	}
}

$break = new BreakPoint($opline);

try {
	$function(1, 2);
} catch (RuntimeException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
