--TEST--
InspectorFrame getVariable
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

function foo($a, $b) {
	$a + $b;
};

$inspector = new InspectorFunction("foo");

$opline = $inspector->getInstruction(2);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		$parameters = [
			$frame->getVariable(0),
			$frame->getVariable(1)
		];

		if ($parameters[0] == 1 &&
		    $parameters[1] == 2) {
			echo "OK";
		}
	}
};

function test($function) {
	$function(1, 2);
}

test("foo");
?>
--EXPECT--
OK

