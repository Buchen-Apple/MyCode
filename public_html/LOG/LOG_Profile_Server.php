<?php
//------------------------------------------------------------
// 프로파일링 로그 남기는 함수 
//
// POST 방식으로 프로파일링 로그를 저장한다.
//
// $_POST['IP']			: 유저
// $_POST['MemberNo']	: 유저
// $_POST['Action']		: 액션구분
//
// $_POST['T_Page']			: Time Page
// $_POST['T_Mysql_Conn']	: Time Mysql Connect
// $_POST['T_Mysql']		: Time Mysql
// $_POST['T_ExtAPI']		: Time API
// $_POST['T_Log']			: Time Log
// $_POST['T_ru_u']			: Time user time used 
// $_POST['T_ru_s']			: Time system time used 
// $_POST['Query']			: Mysql 이 있는 경우 쿼리문
// $_POST['Comment']		: 그 외 기타 멘트
//------------------------------------------------------------
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_Error_Handling_LIbrary.php");
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_DB_Config.php");
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_LOG_GameAndSystem.php");

// 1. Param중 입력 안한 값이 있으면 0이나 None을 넣는다.
if( !isset($_POST['IP']))					$_POST['IP'] 			= 'None';
if( !isset($_POST['AccountNo']))			$_POST['AccountNo'] 	= 0;
if( !isset($_POST['Action']))				$_POST['Action'] 		= 'None';
if( !isset($_POST['T_Page']))				$_POST['T_Page'] 		= 0;
if( !isset($_POST['T_MYsql_Conn']))			$_POST['T_MYsql_Conn'] 	= 0;
if( !isset($_POST['T_Mysql_Query']))		$_POST['T_Mysql_Query'] = 0;
if( !isset($_POST['T_ExtAPI']))				$_POST['T_ExtAPI'] 		= 0;
if( !isset($_POST['T_LOG']))				$_POST['T_LOG'] 		= 0;
if( !isset($_POST['T_ru_u']))				$_POST['T_ru_u'] 		= 0;
if( !isset($_POST['T_ru_s']))				$_POST['T_ru_s'] 		= 0;
if( !isset($_POST['Query']))				$_POST['Query'] 		= 'None';
if( !isset($_POST['Comment']))				$_POST['Comment'] 		= 'None';


// 2. DB 연결
$LogSave_DB = mysqli_connect($Log_DB_IP, $Log_DB_ID, $Log_DB_Password, $Log_DB_Name, $Log_DB_PORT);
if(!$LogSave_DB)
{
    // DB 연결중 문제가 생겼으면 시스템로그 남김
    $errorstring = 'Unable to connect to MySQL' . mysqli_connect_error($LogSave_DB). ' ErrorNo : ' .  mysqli_connect_errno($LogSave_DB);
    LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);

    exit;
}

// 3. 안전하게 POST 가져옴
$IP 			= mysqli_real_escape_string($LogSave_DB, $_POST['IP'] );
$AccountNo 		= mysqli_real_escape_string($LogSave_DB, $_POST['AccountNo'] );
$Action 		= mysqli_real_escape_string($LogSave_DB, $_POST['Action'] );
$T_Page 		= mysqli_real_escape_string($LogSave_DB, $_POST['T_Page'] );
$T_MySQL_Conn 	= mysqli_real_escape_string($LogSave_DB, $_POST['T_MYsql_Conn'] );
$T_MySQL_Query 	= mysqli_real_escape_string($LogSave_DB, $_POST['T_Mysql_Query'] );
$T_ExtAPI 		= mysqli_real_escape_string($LogSave_DB, $_POST['T_ExtAPI'] );
$T_LOG 			= mysqli_real_escape_string($LogSave_DB, $_POST['T_LOG'] );
$T_ru_u 		= mysqli_real_escape_string($LogSave_DB, $_POST['T_ru_u'] );
$T_ru_s 		= mysqli_real_escape_string($LogSave_DB, $_POST['T_ru_s'] );
$Query 			= mysqli_real_escape_string($LogSave_DB, $_POST['Query'] );
$Comment 		= mysqli_real_escape_string($LogSave_DB, $_POST['Comment'] );

// 4. 테이블 이름 설정
// 월 별로 테이블 새로 생성하기 위해 이름을 date에 따라 결정
$Table_Name = 'ProfilingLog_' . date('Ym');

// 5. 쿼리문 만들어서 날리기
$Query = "INSERT INTO `$Table_Name` (`date`, `IP`, `AccountNo`, `Action`, `T_Page`, `T_Mysql_Conn`, `T_Mysql_Query`, `T_ExtAPI`, `T_Log`, `T_ru_u`, `T_ru_s`, `Query`, `Comment`) VALUES 
			(NOW(), '{$IP}', {$AccountNo}, '{$Action}', {$T_Page}, {$T_MySQL_Conn}, {$T_MySQL_Query}, {$T_ExtAPI}, {$T_LOG}, {$T_ru_u}, {$T_ru_s}, '{$Query}', '{$Comment}')";

// 쿼리문, 시스템 로그에 남기기
LOG_System($AccountNo, $_SERVER['PHP_SELF'], $Query);

$Result = mysqli_query($LogSave_DB, $Query);

// 6. 테이블이 없으면 만든 다음에 다시 날리기
if( !$Result && mysqli_errno($LogSave_DB) == 1146)
{
    // 테이블 만들기 쿼리 날림. 테이블이 없는것은 월이 변했다는 것
    mysqli_query($LogSave_DB, "CREATE TABLE $Table_Name LIKE ProfilingLog_template");

    // 원래 날리려던 쿼리 다시 날림
    $Result = mysqli_query($LogSave_DB, $Query);
}

// 새로 날린 후에도 문제가 있다면
if(!$Result)
{
    // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
    $errorstring = 'DB Query Erro' . mysqli_error($LogSave_DB). ' ErrorNo : ' .  mysqli_errno($LogSave_DB) . " / [Query] {$Query}";
    LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);
    
    mysqli_close($LogSave_DB);
    exit;
}

// 7. DB 연결 해제
if($LogSave_DB)
	mysqli_close($LogSave_DB);
	
// echo '<br>Log_Profile_Server.php : 데이터 잘 받아서 DB에 올렸음.<br>';

?>