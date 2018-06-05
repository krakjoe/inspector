--TEST--
InspectorFile onResolve/purge
--FILE--
<?php
$include = sprintf("%s/purge.inc", dirname(__FILE__));

$file = new class($include) extends Inspector\InspectorFile {
	public function onResolve() {
		unset($this->hook);

		$this->hook = new class($this->findFirstInstruction(Inspector\InspectorInstruction::ZEND_ADD)) extends
				Inspector\InspectorBreakPoint {

			public function hit(Inspector\InspectorFrame $frame) {
				echo "OK\n";
			}
		};
	}
};

$a = 10;
$b = 20;

try {
	$file->getEntryInstruction();
} catch (\ReflectionException $e) {
	echo "OK\n";
}

include($include);

Inspector\InspectorFile::purge([__FILE__]);

include($include);

try {
	$file->getEntryInstruction();
} catch (\ReflectionException $e) {
	echo "OK\n";
}
?>
--EXPECT--
OK
OK
OK
OK
