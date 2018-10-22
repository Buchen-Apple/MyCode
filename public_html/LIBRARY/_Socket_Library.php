<?php

// ------------------------
// http 프로토콜로 데이터 전송하는 라이브러리
// php에서 소켓을 열어 URL 호출 후 끝낸다
//
// 본래 php에서는 CURL 라이브러리를 사용하여 외부 URL 호출을 하지만
// 이는 비동기 호출이 되지 않음. URL 호출 결과가 올 때까지 블럭이 걸림.
//
// 그래서 직접 소켓을 열고 웹서버에 데이터 전송 후 바로 종료.
// 물론 상대 서버로 데이터를 전송하기까지는 블럭이 걸림. 하지만 웹 서버의 처리 시간까지 대기하지 않는다는 장점.
// 
// 결과가 필요없는 경우에만 사용(로그 서버 등..)
//
// $url = http:// or https:// 가 포함된 전체 url
// $params : ['key'] = value타입으로 배열 형태로 데이터 입력. array('id' => 'test1', 'pass' => 'test1');
// $type : GET / POST
// ---------------------------

function http_request($url, $params, $type='POST')
{
    // 1. 받은 param값들 셋팅
    // 입력된 params를 key와 value로 분리하여 
    // post_param 이라는 배열로 key = value 타입으로 생성
    // 혹시나 value가 배열인 경우는 ,로 나열한다.
    foreach($params as $key => $value)
    {
        // value가 배열이면, ','를 기준으로 나열되어 있으니 한줄로 만든다.
        if(is_array($value))
            $value = implode(',', $value);
        
        $post_param[] = $key . '=' . urlencode($value);
    }

    // $post_param에는 [0] id=test1 / [1] pass=test1 형태로 들어간다. (key=value형태로 들어감)
    // 이를 기준으로 하나의 스트링으로 붙인다.
    // $post_string안에는 변수가 들어가 있는 느낌.
    $post_string = implode('&', $post_param);

    // 2. URL 분석
    // 입력된 url을 요소별로 분석.
    $parts = parse_url($url);

    // 3. 소켓 오픈
    // http, https에 따라 소켓 접속 타임 설정.
    if($parts['scheme'] == 'http')
        $fp = fsockopen($parts['host'], isset($parts['port'])?parts['port']:80, $errno, $errstr, 10);
     
    else if($parts['scheme'] == 'https')
        $fp = fsockopen("ssl://" . $parts['host'], isset($parts['port'])?parts['port']:443, $errno, $errstr, 30);

    // 에러 처리
    if(!$fp)
    {
        // 에러 로그 실제로는 화면 출력하면 안됨
        echo $errstr($errno) . '<br>\n';
        return 0;
    }

    // 4. Get / Post인지에 따라 http 프로토콜 생성
    $ContensLength = strlen($post_string);

    // Get 타입의 경우 URL에 Param값 붙임
    if($type == 'GET')
    {
        $parts['path'] .= '?' . $post_string;
        $ContensLength = 0; // GET방식일때는 이거 필요 없음. post일때만 의미 있음.
    }

    // HTTP 프로토콜 생성
    $out = "$type " . $parts['path'] . " HTTP//1.1\r\n";
    $out .= "Host: " . $parts['host'] . "\r\n";
    $out .= "Content-Type: application/x-www-form-urlencoded\r\n";
    $out .= "Content-length:" . $ContensLength . "\r\n";
    $out .= "Connection: Close\r\n\r\n";

    // post방식이면서, 인자로 받은 파라미터가 존재한다면, 이 뒤에 Param을 붙임
    if($type == "POST" && isset($post_string))
        $out .= $post_string;

    // 5. 만든 http 전송.
    // fwirte함수는 전송한 바이트 수를 반환하거나 FALSE 오류 발생 시 이를 반환한다.
    $Result = fwrite($fp, $out);

    // echo 'out : ' . $out;

    // 바로 끊어버리는 경우, 서버측에서 이를 무시하는 경우도 있음.
    // 대표적으로 카페24.
    // fread를 1회 호출해서 조금이라도 받는것으로 해결
    // $Response = fread($fp, 1000);
    // echo $Response . '<br>';

    // 6. 소켓 닫기
    fclose($fp);

    // 7. 결과를 함수 밖으로 리턴한다.
    return $Result;
}

?>