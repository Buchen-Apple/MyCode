<?php
// -------------
// 모든 컨텐츠 php의 상단에는 해당 php가 들어간다.
// -------------


// DB_Library와 에러 핸들링 인클루드
require_once('/../LIBRARY/_Error_Handling_LIbrary.php');
require_once('_DB_Library.php');

// 1. 프로파일링 객체 생성
$PF = Profiling::getInstance($cnf_PROFILING_LOG_URL, $_SERVER['PHP_SELF']);  


// 2. 게임로그 객체 생성
$GameLog = GAMELog::getInstance($cnf_GAME_LOG_URL); 


// 3. 클라이언트에서 받은 RAW 데이터를 \r\n으로 분리해서 받음
$Body = explode("\r\n", file_get_contents('php://input'));



// 4. DB연결 후, systemDB의 버전 정보 얻어오기
DB_Connect();

$Query = "SELECT * FROM `systemDB`";
$Result = DB_Query($Query);

// major / minor
$DBVersion = mysqli_fetch_Assoc($Result);

// 리소스 해제
mysqli_free_result($Result);



// 5. 클라가 보낸 버전과 비교
// 다르면 접속 끊음.
if($DBVersion['VersionMajor'] != $Body[0] || $DBVersion['VersionMinor'] != $Body[1])
{
    $Response['resultCode'] = 9000;
    $Response['resultMsg'] = 'DISCORDANCE VERSION!'; // 버전 불일치
    $Response['accountNo'] = $Body[2];

    echo json_encode($Response);
    exit;
}

?>