<?php

require "_CURL_Library.php";

// 깨지는 곳 (브라우저의 인코딩 문제였음)
// $url = 'http://www.gamecodi.com/';

// 오류나는곳
// $url = 'https://www.google.co.kr/';

// 잘 되는곳
// $url = 'https://www.naver.com/';
// $url = 'https://www.daum.net/';

$url = 'http://127.0.0.1/CURL_TEST_SHOW.php';
$postFields = array ("IP" => $_SERVER['REMOTE_ADDR'],  "comment" => "헬로헬로");

$return = http_post($url, $postFields);

if($return['code'] === 200)
    echo $return['body'] . '<br>';

else
    echo $return['code'] . '<br>';    

?>