--TEST--
InspectorFrame getSymbols
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

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		$symbols = $frame->getPrevious()->getPrevious()->getSymbols();
		if (isset($symbols["function"])) {
			echo "OK";
		}
	}
};

function test($function) {
	$function(1, 2);
}

test($function);
?>
--EXPECT--
OK

