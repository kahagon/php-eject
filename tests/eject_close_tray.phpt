--TEST--
eject_close_tray() function
--SKIPIF--
<?php 

if(!extension_loaded('eject')) die('skip ');

 ?>
--FILE--
<?php
echo 'OK'; // no test case for this function yet
?>
--EXPECT--
OK