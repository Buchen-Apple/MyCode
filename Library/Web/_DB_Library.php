<?php
// -------------------------------
// DB Library
// -------------------------------

require_once('_DB_Config.php');

// DB Connect
// 성공시 true / 실패시 false 리턴
function DB_Connect()
{
    global $DB_IP;
    global $DB_ID;
    global $DB_Password;
    global $DB_Name;   

    global $g_DB;   

    $g_DB = mysqli_connect($DB_IP, $DB_ID, $DB_Password, $DB_Name);

    if(!$g_DB)
    {
        // DB 연결중 문제가 생겼으면 시스템로그 남김
        $errorstring = 'Unable to connect to MySQL : ' . mysqli_connect_error($g_DB). ' ErrorNo : ' .  mysqli_connect_errno($g_DB);
        LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);

        exit;
    }
}

// DB Disconnect
function DB_Disconnect()
{
    global $g_DB;
    mysqli_close($g_DB);
}

// DB_Query 함수
function DB_Query($Query)
{
    global $g_DB;
    global $PF;

    // 쿼리문, 시스템 로그에 남기기
    LOG_System(-1, $_SERVER['PHP_SELF'], $Query);

    // 쿼리문 프로파일링 시작
    $PF->startCheck(PF_MYSQL_QUERY);    

    $Result = mysqli_query($g_DB, $Query);

    // 쿼리문 프로파일링 끝
    $PF->stopCheck(PF_MYSQL_QUERY, $Query);
    if(!$Result)
    {
        // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
        $errorstring = 'DB Query Erro : ' . mysqli_error($g_DB). ' ErrorNo : ' .  mysqli_errno($g_DB) . " / [Query] {$Query}";
        LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);

        exit;
    }

    return $Result;
}


// 트랜잭션 쿼리 함수
// Select 쿼리 시에는 사용하지 않음.
// DB연결시 autocommit을 사용하지 않기 때문에 DB가 변경되는 쿼리는 무조건 이를 통해야 함
function DB_TransactionQuery($qurArray)
{
    global $g_DB;
    global $PF;

    // 트랜잭션 걸기
    mysqli_begin_transaction($g_DB);

    $profilingString = '[T_QUERY] ';

    $PF->startCheck(PF_MYSQL_QUERY);    // 쿼리 시간수집 프로파일링 -----------

    foreach($qurArray as $Value)
    {
        // 이번 쿼리가 혹시 숫자는 아닌지 체크. 쿼리는 문자만 들어와야하니까.
        if(is_string($Value))
        {        
            $profilingString .= "$Value // ";    

            // 여기왔으면 문자라는 것이니, 쿼리 날린다.           
            if(!mysqli_query($g_DB, $Value))
            {         
                 // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
                $errorstring = 'DB TransactionQuery Error : ' . mysqli_error($g_DB). ' ErrorNo : ' .  mysqli_errno($g_DB) . " / [Query] {$Value}";
                LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);

                DB_TransactionFail();
                exit;

            }
        }

        // 숫자라면 에러 출력
        else
        {
            // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
            $errorstring = 'DB TransactionQuery Error(Not String) : ' . mysqli_error($g_DB). ' ErrorNo : ' .  mysqli_errno($g_DB) .  " / [Query] {$Value}";
            LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);

            DB_TransactionFail();
            exit;
        }
    }

    // 트랜잭션 커밋 이후에는 LAST_INSERT_ID()시, 값이 안나옴.
    // 그러니, 트랜잭션 커밋 전에 Insert ID를 구해서 돌려준다. 있던 없던.
    //
    // 만약, 다양한 데이터를 추가하고 이에대한 insert_id가 모두 필요하면 별도로 함수 제작 필요
    // insert 쿼리 시, insert_id를 모두 뽑아내서 배열에 저장한 후 그걸 리턴하는 형태 등.. 이 함수에서는 지원 안함.
    $insert_ID = mysqli_insert_id($g_DB); // mysql의 LAST_INSERT_ID() 함수를 호출하는 것.
    mysqli_commit($g_DB);

    $PF->stopCheck(PF_MYSQL_QUERY, $profilingString);     // 쿼리 시간수집 프로파일링 끝 -----------

    // 쿼리문, 시스템 로그에 남기기
    LOG_System(-1, $_SERVER['PHP_SELF'], $profilingString);
    
    return $insert_ID;
}

// 트랜잭션 실패 시 호출하는 함수
function DB_TransactionFail()
{
    // 롤백시킨다.
    global $g_DB;
    mysqli_rollback($g_DB);

    // 롤백 로그 남김
    $errorstring = 'DB TransactionQuery Error(rollback)';
    LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);
}


?>