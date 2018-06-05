<?php
/**
* doc
**/
function map($a, $b, ...$c) : void {
	try {
		if ([1,2,3,4]) {
			foreach ($a as $k => $v) {
				$b($k, $v);
			}
		}
	} catch(Throwable $t) {}
}
?>
