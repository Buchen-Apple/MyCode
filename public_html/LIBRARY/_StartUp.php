<?php
// -------------
// 모든 컨텐츠 php의 상단에는 해당 php가 들어간다.
// -------------


// 각종 인클루드
require_once('_Error_Handling_LIbrary.php');
require_once('_DB_Library.php');
require_once('_DB_Config.php');
require_once('_LOG_Profile.php');
require_once('_ErrorCode.php');

// 1. 프로파일링 객체 생성
$PF = Profiling::getInstance($cnf_PROFILING_LOG_URL, $_SERVER["PHP_SELF"]);  

// 2. 게임로그 객체 생성
$GameLog = GAMELog::getInstance($cnf_GAME_LOG_URL); 


// ResponseJSON() 함수 원형
// 모든 php 종료 직전에, 해당 함수를 이용해 [JSON 인코딩 - 로그 - 결과전송] 절차 진행
function ResponseJSON($Response, $accountNo)
{
    // 인코딩
    $return = json_encode($Response);

    // 시스템로그에 제이슨 저장
    // LOG_System($accountNo, $_SERVER["PHP_SELF"], $return);

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

    // 실패 패킷 전송
    ResponseJSON($Response, $AccountNo);

    // ---------------------------------------
    // cleanup 체크.
    // 이 안에서는 [DB 연결 해제, 프로파일러 보내기, 게임로그 보내기]를 한다.
    $_SERVER = $GLOBALS["_SERVER"];
    require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_Clenup.php");
    // --------------------------------------

    exit;
}

// 페이지 프로파일링 시작
$PF->startCheck(PF_PAGE); 
?>