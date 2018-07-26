<?php

require "_CURL_Library.php";

// 깨지는 곳
// $url = 'http://www.gamecodi.com/';

// 오류나는곳
// $url = 'https://www.google.co.kr/';


// 잘 되는곳
$url = 'https://www.naver.com/';
// $url = 'https://www.daum.net/';

$return = http_post($url, $postFields);

if($return['code'] === 200)
    echo $return['body'] . '<br>';

else
    echo $return['code'] . '<br>';    

?>