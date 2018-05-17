--TEST--
InspectorBreakPoint hit returns refcounted
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

class BreakPoint extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		return $frame;
	}
}

$break = new BreakPoint($opline);

$function(1, 2);

echo "OK";
?>
--EXPECT--
OK
