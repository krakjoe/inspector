--TEST--
InspectorFile expired guard
--FILE--
<?php

$include = sprintf("%s/pending.inc", realpath(__DIR__));

$file = new class($include) extends Inspector\InspectorFile {
	public function onResolve() {}
};

include ($include);

try {
	$file->getEntryInstruction();
} catch (\ReflectionException $e) {
	echo "OK\n";
}
?>
--EXPECT--
Hello World
OK
