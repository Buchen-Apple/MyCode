<?php
// -------------
// 모든 컨텐츠 php의 상단에는 해당 php가 들어간다.
// -------------


// DB_Library와 에러 핸들링 인클루드
require_once('/../LIBRARY/_Error_Handling_LIbrary.php');
require_once('_DB_Library.php');
require_once('_DB_Config.php');
require_once('_LOG_Profile.php');

// 1. 프로파일링 객체 생성
$PF = Profiling::getInstance($cnf_PROFILING_LOG_URL, $_SERVER['PHP_SELF']);  


// 2. 게임로그 객체 생성
$GameLog = GAMELog::getInstance($cnf_GAME_LOG_URL); 


// 3. 클라이언트에서 받은 RAW 데이터를 \r\n으로 분리해서 받음
$Body = explode("\r\n", file_get_contents('php://input'));


// 4. 지정된 DB에 연결 후, system의 버전 정보 얻어오기
$g_StartUpDB = null;
DB_Connect($g_StartUpDB, $StartUP_DB_IP, $StartUP_DB_ID, $StartUP_DB_Password, $StartUP_DB_Name, $StartUP_DB_PORT, $Body[2]);

$Query = "SELECT * FROM `systemDB`";
$g_StartResult = DB_Query($Query, $g_StartUpDB, $Body[2]);

// major / minor
$DBVersion = mysqli_fetch_Assoc($g_StartResult);

// 리소스 해제
//mysqli_free_result($g_StartResult);

// SystemDB와 연결 해제
//DB_Disconnect($g_StartUpDB);



// 5. 클라가 보낸 버전과 비교
// 다르면 접속 끊음.
if($DBVersion['VersionMajor'] != $Body[0] || $DBVersion['VersionMinor'] != $Body[1])
{
    $Response['resultCode'] = 9000;
    $Response['resultMsg'] = 'DISCORDANCE VERSION!'; // 버전 불일치
    $Response['accountNo'] = $Body[2];

    ResponseJSON($Response, $Response['accountNo']);

    exit;
}



// ResponseJSON() 함수 원형
// 모든 php 종료 직전에, 해당 함수를 이용해 [JSON 인코딩 - 로그 - 결과전송] 절차 진행
function ResponseJSON($Response, $accountNo)
{
    // 인코딩
    $return = json_encode($Response);

    // 시스템로그에 제이슨 저장
    LOG_System($accountNo, $_SERVER['PHP_SELF'], $return);

    // 결과 전송
    echo $return;
}


?>