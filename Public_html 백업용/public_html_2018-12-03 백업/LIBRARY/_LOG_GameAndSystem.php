<?php
require('_LOG_Config.php');
require('_Socket_Library.php');

// 시스템 로그 함수
function LOG_System($AccountNo, $Action, $Message, $Level = 1)
{   
     // 전역으로 선언된 변수 or 객체들을 함수 안에서 사용하기 위해 선언.
    global $cnf_SYSTEM_LOG_URL;
    global $cnf_LOG_LEVEL;

    // 로그 레벨이, 로그 저장 레벨보다 크면 남기지 않음.
    if($cnf_LOG_LEVEL < $Level)
        return;

    // 만약, 아직 회원가입 안한? 혹은 AccountNo가 발급되지 않은 유저?
    if($AccountNo < 0)
    {
        if(array_key_exists("HTTP_X_FORWARDED_FOR", $_SERVER))
            $AccountNo = $_SERVER["HTTP_X_FORWARDED_FOR"];

        else if(array_key_exists("REMOTE_ADDR", $_SERVER))
            $AccountNo = $_SERVER["REMOTE_ADDR"];

        else
            $AccountNo = "local";
    }
   
    $postField = array('AccountNo' => $AccountNo, 'Action' => $Action, 'Message' => $Message);

    http_request($cnf_SYSTEM_LOG_URL, $postField);   

    return true;
}
?>