--TEST--
InspectorClass::getMethods returns array<InspectorMethod>
--FILE--
<?php
use Inspector\InspectorClass;
use Inspector\InspectorMethod;

class Test {
	public function method() {
		
	}
}

$inspector = 
	new InspectorClass(Test::class);

foreach ($inspector->getMethods() as $method) {
	if (!$method instanceof InspectorMethod) {
		echo "FAIL\n";
	}
}

echo "OK";
?>
--EXPECT--
OK
