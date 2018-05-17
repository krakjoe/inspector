--TEST--
InspectorClass::getMethod
--FILE--
<?php
use Inspector\InspectorClass;

class Test {
	public function method() {

	}
}

$inspector = 
	new InspectorClass(Test::class);

$inspector = $inspector->getMethod("method");

if ($inspector instanceof \ReflectionMethod) {
	echo "OK";
}
?>
--EXPECT--
OK
