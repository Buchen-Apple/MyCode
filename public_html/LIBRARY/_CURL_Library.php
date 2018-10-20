<?php
// ----------------------------------
// CURL 이용
// http 프로토콜로 데이터 전송하는 라이브러리
// ----------------------------------

// 사용법

/* 
$postField = array (이 안에는 post로 보낼 정보가 있을 시 셋팅한다. 정해진 양식은 없다. 
                    그냥 보낼 데이터가 있으면 key value 형태로 여기에 넣는다. )

$postField = array ("IP"        => $_SERVER['REMOTE_ADDR'], 
                    "Query"     => $SELECT * FROM `abc`,
                    "comment"   => "헬로헬로")

$Response = http_post("http://url...path.php", $postField);

$Response에 배열로 BODY와 ResultCode가 들어오도록 http_post함수를 디자인한다.

$Response['body'] << 결과 body
$Response['code'] << 결과 code
*/

// 결과값을 '배열'로 리턴한다.
function http_post($url, $postFields)
{
    // ---------------
    // HTTP 셋팅하기
    $ci = curl_init();

    curl_setopt($ci, CURLOPT_USERAGENT, "TEST");        // 보낼때 헤더의 "USER-AGENT"에 붙을 이름.
    curl_setopt($ci, CURLOPT_CONNECTTIMEOUT, 30);       // 연결 대기할 동안, 대기할 시간.(초) 0이면 무한대기.
    curl_setopt($ci, CURLOPT_TIMEOUT, 30);              // 입력한 시간동안 작업이 완료되지 않으면 강제 해제. (초)
    curl_setopt($ci, CURLOPT_RETURNTRANSFER, TRUE);     // curl_exec()함수 호출 결과를 리턴받을 것인지. true면 받음
    curl_setopt($ci, CURLOPT_SSL_VERIFYPEER, FALSE);    // 인증서 체크 여부. https에 사용되는 SSL 인증서를 의미하는듯. true면 체크 / false면 체크안함.
    curl_setopt($ci, CURLOPT_HEADER, FALSE);            // 헤더 출력 여부. TRUE시 헤더 출력한다.
    curl_setopt($ci, CURLOPT_POST, TRUE);               // post 모드 수행 여부. 이걸 true로 하면 x-Forward-For를 사용한다.

    // 인자로 받은 $postFields가 비어있지 않다면, post형식이니 보낼 데이터 셋팅
    if( !empty($postFields))
        curl_setopt($ci, CURLOPT_POSTFIELDS, $postFields);   

    curl_setopt($ci, CURLOPT_URL, $url);                // 데이터를 보낼곳의 url을 curl객체에 넣기
    $Response = array();

    // ---------------
    // HTTP 전송하기 후 결과 받기
    $Response['body'] = curl_exec($ci);
    $Response['code'] = curl_getinfo($ci, CURLINFO_HTTP_CODE);

    // ---------------
    // CURL 해제
    curl_close($ci);

    return $Response;
}







?>