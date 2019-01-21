<?php
require('_LOG_GameAndSystem.php');

error_reporting(E_ALL);
set_error_handler('ERROR_Handler');
set_exception_handler('EXCEPTION_Handler');

// PHP 에러 로그 처리. set_error_handler()로 등록한 함수이며, 함수 포인터이다. 함수의 형태가 정해져 있다.
function ERROR_Handler($errno, $errstr, $errfile, $errline)
{
    // 에러 메시지 만들기
    $ErrorMsg = "($errno) FILE: $errfile / LINE: $errline / MSG:  $errstr";

    // 파일로 남긴다.
    $myfile = fopen("MYErrorfileERROR_Handler.txt", "w") or die("Unable to open file!");
    $txt = "ERROR_Handler --> $ErrorMsg \n";
    fwrite($myfile, $txt);
    fclose($myfile);

    // 만든 에러를 DB에 저장
    LOG_System(-1, 'Error_Handler',  $ErrorMsg);
    exit;
}
// PHP 예외 로그 처리. set_exception_handler()로 등록한 함수이며, 함수 포인터이다. 함수의 형태가 정해져 있다.
function EXCEPTION_Handler($Exc)
{
    // 예외 메시지와 예외 코드 넘김
    LOG_System(-1, 'Exception_Handler',  "{$Exc->getMessage()} / errorCode : {$Exc->getCode()}");
    exit;
}
?>