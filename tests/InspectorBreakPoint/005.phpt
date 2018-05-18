--TEST--
InspectorBreakPoint getInstruction
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
	public function hit(InspectorFrame $frame){}
}

$break = new BreakPoint($opline);

if ($break->getInstruction() === $opline) {
	echo "OK";
}
?>
--EXPECT--
OK
