--TEST--
InspectorBreakPoint exists
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

try {
	$disallowed = new BreakPoint($opline);
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
