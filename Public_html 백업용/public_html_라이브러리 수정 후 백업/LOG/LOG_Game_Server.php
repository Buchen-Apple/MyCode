<?php
//------------------------------------------------------------
// 게임 로그 남기는 서버
//
// POST 방식으로 로그덩어리 (LogChunk) 에 JSON 포멧으로 들어오면 이를 배열로 풀어서 하나하나 저장한다.
// 게임로그는 하나하나 개별적인 저장이 아닌 컨텐츠 별로 관련 로그를 한방에 모아서 저장한다.
//
// array(	array(MemberNo, LogType, LogCode, Param1, Param2, Param3, Param4, ParamString),
//			array(MemberNo, LogType, LogCode, Param1, Param2, Param3, Param4, ParamString));
//
// 2차원 배열로 한 번에 여러개의 로그가 몰아서 들어옴.
// // $AccountNo, $LogType, $LogCode, $Param1, $Param2, $Param3, $Param4, $ParamString 가 넘어온다.
//------------------------------------------------------------

require_once('/../LIBRARY/_DB_Config.php');
require_once('/../LIBRARY/_LOG_GameAndSystem.php');

// 1. $_POST['LogChunk'] 존재유무 체크
if( !isset($_POST['LogChunk']))
    return 0;

// 2. Json 디코딩
$JsonResult = json_decode($_POST['LogChunk'], true);

// 3. 1개 이상의 완성된 배열이 있는지 확인 (로그 1개 이상 있는지)
$ArrayCount = count($JsonResult);
if($ArrayCount == 0)
    return 0;

// 4. DB와 연결
$LogSave_DB = mysqli_connect($Log_DB_IP, $Log_DB_ID, $Log_DB_Password, $Log_DB_Name, $Log_DB_PORT);
if(!$LogSave_DB)
{
    // DB 연결중 문제가 생겼으면 시스템로그 남김
    $errorstring = 'Unable to connect to MySQL' . mysqli_connect_error($LogSave_DB). ' ErrorNo : ' .  mysqli_connect_errno($LogSave_DB);
    LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);

    exit;
}

// 5. 테이블 이름 생성
// 월 별로 테이블 새로 생성하기 위해 이름을 date에 따라 결정
$Table_Name = 'GameLog_' . date('Ym');

// 6. 배열의 수 만큼 반복하며 쿼리 생성
// $LogChunk[N] 의 ['AccountNo'] ['LogType'] .... 데이터를 뽑아서 INSERT 쿼리 생성
$Query = "INSERT INTO `$Table_Name` (`date`, `AccountNo`, `LogType`, `LogCode`, `Param1`, `Param2`, `Param3`, `Param4`, `ParamString`) VALUES ";

$i = 0;
$Query .= "(NOW(), {$JsonResult[$i]['AccountNo']} , {$JsonResult[$i]['LogType']}, {$JsonResult[$i]['LogCode']}, 
            {$JsonResult[$i]['Param1']}, {$JsonResult[$i]['Param2']}, {$JsonResult[$i]['Param3']}, {$JsonResult[$i]['Param4']}, '{$JsonResult[$i]['ParamString']}')";

$i++;

// 첫 쿼리는 이미 만들어져 있음. 두번째 Insert부터 추가한다.
// VALUE(), (), () 형태로 들어가는데.. () 사이에 ,가 최초에는 없어야해서 첫 번째는 미리 만들어둠.
for(; $i < $ArrayCount; ++$i)
{
    $Query .= ", (NOW(), {$JsonResult[$i]['AccountNo']} , {$JsonResult[$i]['LogType']}, 
                {$JsonResult[$i]['LogCode']}, {$JsonResult[$i]['Param1']}, {$JsonResult[$i]['Param2']}, {$JsonResult[$i]['Param3']}, {$JsonResult[$i]['Param4']}, '{$JsonResult[$i]['ParamString']}')";
}

// 7. 쿼리 날림

// 쿼리문, 시스템 로그에 남기기
LOG_System($JsonResult[0]['AccountNo'], $_SERVER['PHP_SELF'], $Query);

// 쿼리 날림
$Result = mysqli_query($LogSave_DB, $Query);


// 8. 테이블이 없으면 테이블 생성 후 다시 쿼리 날림
if( !$Result && mysqli_errno($LogSave_DB) == 1146)
{
    // 테이블 만들기 쿼리 날림. 테이블이 없는것은 월이 변했다는 것
    mysqli_query($LogSave_DB, "CREATE TABLE $Table_Name LIKE GameLog_template");

    // 원래 날리려던 쿼리 다시 날림
    $Result = mysqli_query($LogSave_DB, $Query);
}

// 7번 쿼리날림의 Result시, 테이블이 있었을 경우 해당 if문 즉시 체크
// 테이블이 없었으면 8번 생성 후 쿼리날림 다음에, 쿼리 결과문으로 여기서 체크
if(!$Result)
{
    // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
    $errorstring = 'DB Query Erro' . mysqli_error($LogSave_DB). ' ErrorNo : ' .  mysqli_errno($LogSave_DB) . " / [Query] {$Query}";
    LOG_System($JsonResult[0]['AccountNo'], $_SERVER['PHP_SELF'], $errorstring);
    
    mysqli_close($LogSave_DB);
    exit;
}

// 9. DB와 연결 해제
if($LogSave_DB)
    mysqli_close($LogSave_DB);

?>