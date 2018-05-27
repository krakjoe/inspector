--TEST--
InspectorClass onResolve/purge
--FILE--
<?php
use Inspector\InspectorClass;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$include = sprintf("%s/purge.inc", dirname(__FILE__));

$inspector = new class("test") extends InspectorClass {
	public function onResolve() {
		$method = $this->getMethod("add");
		
		$instruction = $method->getEntryInstruction();

		unset($this->hook);

		$this->hook = new class($instruction) extends InspectorBreakPoint {
			public function hit(InspectorFrame $frame) {
				echo "OK\n";
			}
		};
	}

	private $hook;
};

include($include); # defines Test

Test::add(1,2);

InspectorClass::purge([get_class($inspector)]); # deletes Test, but not InspectorClass for Test

include($include); # defines Test

Test::add(3,4);
?>
--EXPECT--
OK
OK
