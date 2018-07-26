<?php

require "_Socket_Library.php";

$url = 'http://www.gamecodi.com/';

// $url = "http://127.0.0.1/3_Login.php";
// $Param = array('id' => 'song', 'password' => '1234');

$return = curl_request($url, $Param, 'GET');

if($return == false)
{
    echo 'fwrite 실패! <br>';
    exit;
}

echo "<br><br>$return 바이트 write성공 <br>";

?>