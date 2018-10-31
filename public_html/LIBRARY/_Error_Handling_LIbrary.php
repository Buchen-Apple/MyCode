<?php
/*
PHP 에러를 핸들링하여 시스템 로그로 남기는 코드

 * 필수!! _LOG_GameAndSystem.php 가 include 되어있어야 함. (LOG_SYSTEM() 사용 예정)
*/

/*
에러 핸들러 등록.

E_ALL : 모든 에러를 출력. php.ini에서도 설정 가능
에러 발생 시, ERROR_Handler() 함수 호출. (그렇게 되도록 등록함)
예외 발생 시, EXCEPTION_Handler() 함수 호출 (그렇게 되도록 등록함)

설정 이후에는, 문법 에러 외에는 화면에 에러가 출력되지 않음. 다 받아서 로그로 남김 (시스템 로그)
*/

require_once('_LOG_GameAndSystem.php');

error_reporting(E_ALL);
set_error_handler('ERROR_Handler');
set_exception_handler('EXCEPTION_Handler');

// ---------------------
// 에러값들
// 1 - 에러 아님. 성공
// 9000 - major, minor버전 중 하나라도 다를 시 발생
// 9001 - 회원가입 중, 아이디 중복
// 9002 - 회원가입 중, 닉네임 중복
// 9003 - 로그인 중, ID나 pass가 일치하지 않는 유저임
// 9004 - 세션키 갱신 중, AccountNo나 SessionKey가 일치하는 않는 유저임.
// 9005 - SocreGet 중, AccountNo나 SessionKey가 일치하는 않는 유저임.
// 9006 - UpdateGet 중, AccountNo나 SessionKey가 일치하는 않는 유저임.


// -------------
// PHP 에러 로그 처리
// set_error_handler()로 등록한 함수이며, 함수 포인터이다.
// 함수의 형태가 정해져 있다.
// -------------
function ERROR_Handler($errno, $errstr, $errfile, $errline)
{
    // 전역으로 $g_AccountNo가 선언되어있다는 가정.
    // 누구로 인한 에러인가 체크용도.
    global $g_AccountNo;

    // 유저가 존재하는지 체크
    // 만약, 아직 유저가 뭐 하다가 난 에러가 아니라, 그냥 오류가 난거면 이게 없을수도 있음.
    if(isset($g_AccountNo) == FALSE)
        $g_AccountNo = -1;

    // 에러 메시지 만들기
    $ErrorMsg = "($errno) FILE: $errfile / LINE: $errline / MSG:  $errstr";

    // 파일로 남긴다.
    $myfile = fopen("MYErrorfileERROR_Handler.txt", "w") or die("Unable to open file!");
    $txt = "ERROR_Handler --> $ErrorMsg \n";
    fwrite($myfile, $txt);
    fclose($myfile);

    // 만든 에러를 DB에 저장
    LOG_System($g_AccountNo, 'Error_Handler',  $ErrorMsg);
    exit;
}

// -------------
// PHP 예외 로그 처리
// set_exception_handler()로 등록한 함수이며, 함수 포인터이다.
// 함수의 형태가 정해져 있다.
// -------------
function EXCEPTION_Handler($Exc)
{
    global $g_AccountNo;
    
    if(isset($g_AccountNo) == FALSE)
        $g_AccountNo = -1;

    // 예외 메시지와 예외 코드 넘김
    $ExcMsg = "{$Exc->getMessage()} / errorCode : {$Exc->getCode()}";
    LOG_System($g_AccountNo, 'Exception_Handler',  $ExcMsg);
    exit;
}




?>