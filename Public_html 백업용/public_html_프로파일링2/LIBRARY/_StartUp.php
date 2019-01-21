<?php
// -------------
// 모든 컨텐츠 php의 상단에는 해당 php가 들어간다.
// -------------

// 각종 인클루드
require('_Error_Handling_LIbrary.php');
require('_DB_Library.php');
require('_DB_Config.php');
require('_ErrorCode.php');
require('_LOG_Profile.php');

// 1. 프로파일링 객체 생성
$PF = Profiling::getInstance($cnf_PROFILING_LOG_URL, $_SERVER["PHP_SELF"], $cnf_PROFILING_LOG_RATE);  

// 게임로그 객체 생성
$GameLog = GAMELog::getInstance($cnf_GAME_LOG_URL); 

// 페이지 프로파일링 시작
$PF->startCheck(PF_PAGE); 

// ResponseJSON() 함수 원형
// 모든 php 종료 직전에, 해당 함수를 이용해 [JSON 인코딩 - 로그 - 결과전송] 절차 진행
function ResponseJSON($Response, $accountNo)
{
    // 인코딩
    $return = json_encode($Response);
    
    // 결과 전송
    echo $return;
}

// ---------------------------------
// 에러 발생 시, 실패패킷 전송 후 exit하는 함수
//
// Parameter : returnCode(실패 result)
// return : 없음
// ---------------------------------
function OnError($result, $AccountNo = -1)
{
    $Response['result'] = $result;   

    // ---------------------------------------
    // cleanup 체크.
    // 이 안에서는 [DB 연결 해제, 프로파일러 보내기, 게임로그 보내기]를 한다.
    require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_Clenup.php");
    // --------------------------------------

    // 실패 패킷 전송
    ResponseJSON($Response, $AccountNo);

    exit;
}


?>