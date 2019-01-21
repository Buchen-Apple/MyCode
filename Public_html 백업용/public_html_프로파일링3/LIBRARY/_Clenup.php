<?php
// -------------
// 모든 컨텐츠 php의 하단에는 해당 php가 들어간다.
// -------------

// 프로파일링 저장
global $ClearAccountNo;
global $PF;

// 페이지 프로파일링 스탑
$PF->stopCheck(PF_PAGE); 

// ClearAccountNo 변수가 존재한다면, 해당 값을 넣는다.
if(isset($ClearAccountNo))
{   
    $PF->LOG_Save($ClearAccountNo);
}

// 없으면 아무것도 안넣는다 (아무것도 안넣으면 -1 이다)
else
{   
    $PF->LOG_Save();
}

// 게임로그 저장
global $GameLog;
$GameLog->SaveLog();

?>