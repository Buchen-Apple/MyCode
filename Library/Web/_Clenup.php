<?php
// -------------
// 모든 컨텐츠 php의 하단에는 해당 php가 들어간다.
// -------------

// DB_Library와 에러 핸들링 인클루드
require_once('/../LIBRARY/_Error_Handling_LIbrary.php');
require_once('_DB_Library.php');


// DB와 연결 해제
DB_Disconnect();

// 프로파일링 저장
$PF->LOG_Save();

// 게임로그 저장
$GameLog->SaveLog();


?>