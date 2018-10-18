<?php
// ---------------------------
// 시스템 로그 남기는 서버.
// $AccountNo, $Action, $Message 가 넘어온다.
// ---------------------------

require_once('/../LIBRARY/_DB_Config.php');

// 1. DB와 연결
// 여기서마저 연결하다가 오류나면, 이젠 아파치의 error로그를 볼 수 밖에 없음.
$LogSave_DB = mysqli_connect($Log_DB_IP, $Log_DB_ID, $Log_DB_Password, $Log_DB_Name, $Log_DB_PORT);

// 테이블 이름 설정
// 월 별로 테이블 새로 생성하기 위해 이름을 date에 따라 결정
$Table_Name = 'SystemLog_' . date('Ym');

// 2. 값이 존재하지 않는 인자들을 셋팅한다.
if( !isset($_POST['AccountNo']) )
    $_POST['AccountNo'] = 'None';

if( !isset($_POST['Action']) )
    $_POST['Action'] = 'None';

if( !isset($_POST['Message']) )
    $_POST['Message'] = 'None';


// 3. Param값들 가져옴
$AccountNo = mysqli_real_escape_string($LogSave_DB, $_POST['AccountNo']);
$Action = mysqli_real_escape_string($LogSave_DB, $_POST['Action']);
$Message = mysqli_real_escape_string($LogSave_DB, $_POST['Message']);




// 4. 테이블로 쿼리 날린다.
$Query = "INSERT INTO $Table_Name (`date`, `AccountNo`, `Action`, `Message`) VALUES(NOW(), '$AccountNo', '$Action', '$Message')";

$Result = mysqli_query($LogSave_DB, $Query);




// 5. 쿼리에 실패했는데, 테이블이 없어서 난 에러(1146) 라면, 테이블 생성 후 다시 보냄
if( !$Result && mysqli_errno($LogSave_DB) == 1146)
{
    // 테이블 만들기 쿼리 날림. 테이블이 없는것은 월이 변했다는 것
    mysqli_query($LogSave_DB, "CREATE TABLE $Table_Name LIKE SystemLog_template");

    // 원래 날리려던 쿼리 다시 날림
    mysqli_query($LogSave_DB, $Query);
}

// 여기까지 왔는데도 문제가 있다면
if(!$Result)
{
    // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
    $errorstring = '[DB Query Erro(LOG_System_Server.php)] ' . mysqli_error($LogSave_DB). ' ErrorNo : ' .  mysqli_errno($LogSave_DB) . " / [Query] {$Query}";
    $Result = mysqli_query($LogSave_DB, $errorstring);
    
    mysqli_close($LogSave_DB);
    exit;
}




// 6. DB와 연결 해제
if($LogSave_DB)
    mysqli_close($LogSave_DB);

?>