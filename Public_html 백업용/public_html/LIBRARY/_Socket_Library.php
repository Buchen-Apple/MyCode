<?php
function http_request($url, $params, $type='POST')
{
    // 받은 param값들 셋팅
    foreach($params as $key => $value)
    {
        // value가 배열이면, ','를 기준으로 나열되어 있으니 한줄로 만든다.
        if(is_array($value))
             $value = implode(',', $value);
            
        $post_param[] = $key . '=' . urlencode($value);
    }

    // $post_param에는 [0] id=test1 / [1] pass=test1 형태로 들어간다. (key=value형태로 들어감)
    $post_string = implode('&', $post_param);

    // 2. URL 분석
    // 입력된 url을 요소별로 분석.
    $parts = parse_url($url);

    // 3. 소켓 오픈
    // port 설정.
    if(isset($parts['port']))
        $TempPort = $parts['port'];

    else
        $TempPort = 80;

   
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
    $fp = fsockopen($parts['host'], $TempPort, $errno, $errstr, 10);     

    // 에러 처리
    if(!$fp)
    {
        // 파일로 남김
        $myfile = fopen("MYErrorfile.txt", "w") or die("Unable to open file!");
        $txt = "http_request() --> fsockopen() error : $TempHost : $TempPort --> $errstr ($errno) \n";
        fwrite($myfile, $txt);
        fclose($myfile);     

        exit;
    }   

    // fwirte함수는 전송한 바이트 수를 반환하거나 FALSE 오류 발생 시 이를 반환한다.
    $Result = fwrite($fp, $out); 
    
    fclose($fp);

    // FALSE면 false 리턴
    if($Result === false)
        return false;

    // FALSE가 아니면 Write 한 바이트 리턴
    return $Result;   
}
?>