<?php
if(!isSet($_GET['key'])){
die("No key set.");
}
$key = $_GET['key'];


// ADD YOUR KEYS HERE
$permitted_keys = array("AA-BB-CC-DD", "EE-FF-00-11", "22-33-44-55");


if(in_array($key, $permitted_keys)){
        die("true");
}else{
        die("false");
}