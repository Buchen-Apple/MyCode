<?php
// -------------------------------
// DB Library
// -------------------------------

// DB Connect
//
// Parameter : DB변수(out), IP, ID, Pass, Name, Port, AccountNo(디폴트 -1)
// return : 연결 성공 시 true
//        : 연결 실패 시 false. 
function DB_Connect(&$ConnectDB, $DB_IP, $DB_ID, $DB_Password, $DB_Name, $DB_Port, $AccountNo = -1)
{
    $ConnectDB = mysqli_connect($DB_IP, $DB_ID, $DB_Password, $DB_Name, $DB_Port);

    if(!$ConnectDB)
    {        
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

    // 쿼리문, 시스템 로그에 남기기
    LOG_System($AccountNo, $_SERVER['PHP_SELF'], $Query);

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


// 트랜잭션 쿼리 함수
// Select 쿼리 시에는 사용하지 않음.
// DB연결시 autocommit을 사용하지 않기 때문에 DB가 변경되는 쿼리는 무조건 이를 통해야 함
//
// Parameter : 쿼리 Array, DB변수(out), AccountNo(디폴트 -1)
// return : 성공 시 InsertID
//        : 실패 시 false  
function DB_TransactionQuery($qurArray, &$ConnectDB, $AccountNo = -1)
{
    global $PF;

    // 트랜잭션 걸기
    mysqli_begin_transaction($ConnectDB);

    $profilingString = '[T_QUERY] ';

    $PF->startCheck(PF_MYSQL_QUERY);    // 쿼리 시간수집 프로파일링 -----------

    foreach($qurArray as $Value)
    {
        // 이번 쿼리가 혹시 숫자는 아닌지 체크. 쿼리는 문자만 들어와야하니까.
        if(is_string($Value))
        {        
            $profilingString .= "$Value // ";    

            // 여기왔으면 문자라는 것이니, 쿼리 날린다.           
            if(!mysqli_query($ConnectDB, $Value))
            {       
                $PF->stopCheck(PF_MYSQL_QUERY, $profilingString);     // 쿼리 시간수집 프로파일링 끝 ----------- 
                
                 // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
                $errorstring = 'DB TransactionQuery Error : ' . mysqli_error($ConnectDB). ' ErrorNo : ' .  mysqli_errno($ConnectDB) . " / [Query] {$Value}";
                LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

                DB_TransactionFail();

                return false;
            }
        }

        // 숫자라면 에러 출력
        else
        {
            $PF->stopCheck(PF_MYSQL_QUERY, $profilingString);     // 쿼리 시간수집 프로파일링 끝 -----------

            // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
            $errorstring = 'DB TransactionQuery Error(Not String) : ' . mysqli_error($ConnectDB). ' ErrorNo : ' .  mysqli_errno($ConnectDB) .  " / [Query] {$Value}";
            LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

            DB_TransactionFail();

            return false;
        }
    }

    // 트랜잭션 커밋 이후에는 LAST_INSERT_ID()시, 값이 안나옴.
    // 그러니, 트랜잭션 커밋 전에 Insert ID를 구해서 돌려준다. 있던 없던.
    //
    // 만약, 다양한 데이터를 추가하고 이에대한 insert_id가 모두 필요하면 별도로 함수 제작 필요
    // insert 쿼리 시, insert_id를 모두 뽑아내서 배열에 저장한 후 그걸 리턴하는 형태 등.. 이 함수에서는 지원 안함.
    $insert_ID = mysqli_insert_id($ConnectDB); // mysql의 LAST_INSERT_ID() 함수를 호출하는 것.
    mysqli_commit($ConnectDB);

    $PF->stopCheck(PF_MYSQL_QUERY, $profilingString);     // 쿼리 시간수집 프로파일링 끝 -----------

    // 쿼리문, 시스템 로그에 남기기
    LOG_System($AccountNo, $_SERVER['PHP_SELF'], $profilingString);
    
    return $insert_ID;
}

// 트랜잭션 실패 시 호출하는 함수
function DB_TransactionFail($profilingString, &$ConnectDB, $AccountNo = -1)
{
    // 롤백시킨다.
    mysqli_rollback($ConnectDB);

    // 롤백 로그 남김
    $errorstring = 'DB TransactionQuery Error(rollback)';
    LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

    // 현재까지 프로파일링 저장
    $PF->LOG_Save();

    // 현재까지 게임로그 저장
    $GameLog->SaveLog();

    // 현재까지 쿼리 저장
    LOG_System($AccountNo, $_SERVER['PHP_SELF'], $profilingString);
}


?>