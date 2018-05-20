--TEST--
InspectorFile no inspecting current file
--FILE--
<?php
use Inspector\InspectorFile;

try {
	new class(__FILE__) extends InspectorFile {
		public function onResolve() {
			
		}
	};
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK

