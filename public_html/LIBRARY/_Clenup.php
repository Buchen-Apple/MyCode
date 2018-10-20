<?php
// -------------
// 모든 컨텐츠 php의 하단에는 해당 php가 들어간다.
// -------------

// 프로파일링 저장
global $PF;
$PF->LOG_Save();

 // 게임로그 저장
global $GameLog;
$GameLog->SaveLog();

?>