<?php

require "_Socket_Library.php";

//$url = 'http://www.gamecodi.com/';

$url = 'http://127.0.0.1/CURL_TEST_SHOW.php';
$Param = array ("IP" => $_SERVER['REMOTE_ADDR'],  "comment" => "헬로헬로");

$return = curl_request($url, $Param, 'POST');

if($return == false)
{
    echo 'fwrite 실패! <br>';
    exit;
}

echo "<br><br>$return 바이트 write성공 <br>";

?>