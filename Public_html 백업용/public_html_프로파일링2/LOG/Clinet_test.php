<?php
require_once('/../LIBRARY/_LOG_GameAndSystem.php');

// 로그를 수집하는 프로파일링 객체
$PF = Profiling::getInstance($cnf_PROFILING_LOG_URL, $_SERVER['PHP_SELF']);   

// ----------------------
// 시스템 로그 테스트
// ----------------------

// 시스템 로그 전송
LOG_System(10, $_SERVER['PHP_SELF'], "시스템로그 테스트");




// ----------------------
// 게임 로그 테스트
// ----------------------

// 게임 로그 객체 선언
$GameLog = GAMELog::getInstance($cnf_GAME_LOG_URL);

// 게임 로그 수집하기. $AccountNo, $Type, $Code, $Param1, $Param2, $Param3, $Param4, $ParamString
$GameLog->AddLog(10, 1, 2, 0, 0, 0, 99, 'GameLogTest1');
$GameLog->AddLog(11, 1, 2, 0, 0, 8, 0, 'GameLogTest2');
$GameLog->AddLog(12, 1, 2, 10, 0, 0, 0, 'GameLogTest3');
$GameLog->AddLog(13, 1, 2, 0, 7, 0, 0, 'GameLogTest4');
$GameLog->AddLog(14, 1, 2, 0, 10, 0, 5, 'GameLogTest5');

// 게임로그 서버로 로그 전송
$GameLog->SaveLog();




// ----------------------
// 프로파일링 테스트
// ----------------------
// 페이지 시간 체크 시작
$PF->startCheck(PF_PAGE);


// 쿼리 시간 체크 테스트
$PF->startCheck(PF_MYSQL_QUERY);
usleep(100000);      // 0.1초
$PF->stopCheck(PF_MYSQL_QUERY, 'Sleep1');

$PF->startCheck(PF_MYSQL_QUERY);
usleep(500000);      // 0.5초
$PF->stopCheck(PF_MYSQL_QUERY, 'Sleep2');

$PF->startCheck(PF_MYSQL_QUERY);
usleep(50000);      // 0.05초
$PF->stopCheck(PF_MYSQL_QUERY, 'Sleep3');


// 로그 시간 체크 테스트
$PF->startCheck(PF_LOG);
usleep(10000);      // 0.01초
$PF->stopCheck(PF_LOG, 'Game_Log1');

$PF->startCheck(PF_LOG);
usleep(800000);      // 0.8초
$PF->stopCheck(PF_LOG, 'System_Log1');


// API 시간 테스트
$PF->startCheck(PF_EXTAPI);
usleep(100000);      // 0.1초
$PF->stopCheck(PF_EXTAPI, 'Google_Login_API');

$PF->startCheck(PF_EXTAPI);
usleep(700000);      // 0.7초
$PF->stopCheck(PF_EXTAPI, 'Naver_News_API');

// 페이지 시간 체크 끝.
$PF->stopCheck(PF_PAGE, 'TotalPage');

// 현재까지 수집된 모든 로그 세이브
echo 'LogSave Start!!<br>';
$PF->LOG_Save();

echo 'LogSave end!!<br>';

?>