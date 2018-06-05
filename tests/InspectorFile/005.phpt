--TEST--
InspectorFile pending guard
--FILE--
<?php

$file = new class("some.php") extends Inspector\InspectorFile {
	public function onResolve() {}
};

try {
	$file->getEntryInstruction();
} catch (\ReflectionException $e) {
	echo "OK\n";
}
?>
--EXPECT--
OK
