<?php
// DB Connect
//
// Parameter : DB변수(out), IP, ID, Pass, Name, Port, AccountNo(디폴트 -1)
// return : 연결 성공 시 true
//        : 연결 실패 시 false. 
function DB_Connect(&$ConnectDB, $DB_IP, $DB_ID, $DB_Password, $DB_Name, $DB_Port, $AccountNo = -1)
{
    global $PF;
    // 프로파일링 시작
    $PF->startCheck(PF_MYSQL_CONN); 

    $ConnectDB = mysqli_connect($DB_IP, $DB_ID, $DB_Password, $DB_Name, $DB_Port);

    // 프로파일링 끝
    $PF->stopCheck(PF_MYSQL_CONN); 

    if(!$ConnectDB)
    {                
        $errno = mysqli_connect_errno($ConnectDB);
        $errStr = mysqli_connect_error($ConnectDB);

        $myfile = fopen("MYErrorfile.txt", "w") or die("Unable to open file!");
        $txt = "$errno : $errStr\n";
        fwrite($myfile, $txt);
        fclose($myfile);

        // DB 연결중 문제가 생겼으면 시스템로그 남김
        $errorstring = 'Unable to connect to MySQL : ' . mysqli_connect_error($ConnectDB). ' ErrorNo : ' .  mysqli_connect_errno($ConnectDB);
        LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

        return false;
    }

    return true;
}
// DB Disconnect
//
// Parameter : DB변수(out)
// return : 없음
function DB_Disconnect(&$ConnectDB)
{
    mysqli_close($ConnectDB);
}
// DB_Query 함수
//
// Parameter : 쿼리, DB변수(out), AccountNo(디폴트 -1)
// return : 성공 시 result
//        : 실패 시 false
function DB_Query($Query, &$ConnectDB, $AccountNo = -1)
{ 
    global $PF;

    // 쿼리문 프로파일링 시작
    $PF->startCheck(PF_MYSQL_QUERY);    

    $Result = mysqli_query($ConnectDB, $Query);

    // 쿼리문 프로파일링 끝
    $PF->stopCheck(PF_MYSQL_QUERY, $Query);

    if(!$Result)
    {
        // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
        $errorstring = 'DB Query Erro : ' . mysqli_error($ConnectDB). ' ErrorNo : ' .  mysqli_errno($ConnectDB) . " / [Query] {$Query}";
        LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

        return false;
    }

    return $Result;
}
?>