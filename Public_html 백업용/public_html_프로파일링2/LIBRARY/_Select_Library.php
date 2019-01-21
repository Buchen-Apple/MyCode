<?php
// 인자로 받은 accountno 혹은 email을 이용해 해당 유저의 dbno와 AccountNo를 알아온다.
function SearchUser($TempKey, $TempValue)
{ 
    // 1. shdb_Index의, 어떤 Slave에게 접속할 것인지 결정
    global $Index_SlaveCount, $Index_Slave_DB_IP, $Index_Slave_DB_ID, $Index_Slave_DB_Password, $Index_Slave_DB_PORT, $Index_Slave_DB_Name;  
   
    $Index = rand() % $Index_SlaveCount;   
     
    // 2. Slave에게 연결
    $shDB_Index;
    if(DB_Connect($shDB_Index, $Index_Slave_DB_IP[$Index], $Index_Slave_DB_ID[$Index], $Index_Slave_DB_Password[$Index], $Index_Slave_DB_Name[$Index], $Index_Slave_DB_PORT[$Index]) === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DBIndex Slave연결 에러)
        global $cnf_DB_INDEX_SLAVE_CONNECT_ERROR;
        OnError($cnf_DB_INDEX_SLAVE_CONNECT_ERROR);    
    }   

    // 3. Update 쿼리문을 만든다.
    $Key = mysqli_real_escape_string($shDB_Index, $TempKey);
    $Value = mysqli_real_escape_string($shDB_Index, $TempValue);    

    // 쿼리문 만들기
    if($Key == 'accountno')
        $Query = "SELECT `accountno`, `dbno` FROM `shDB_Index`.`allocate` WHERE accountno = $Value";

    else if($Key == 'email')
        $Query = "SELECT `accountno`,`dbno` FROM `shDB_Index`.`allocate` WHERE email = '$Value'";

    // 둘 다 아니면 파라미터 에러
    else
    {
        DB_Disconnect($shDB_Index);

        // 시스템로그 남김
        $errorstring = 'SearchUser -> ParameterError. Value : ' . "$Value";
        LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);
        
        // 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
        global $cnf_ERROR_PARAMETER;
        OnError($cnf_ERROR_PARAMETER);  
    }  

    // 4. shDB_Index.allocate로 쿼리문 날림(슬레이브에게)  
    $Result = DB_Query($Query, $shDB_Index);
    DB_Disconnect($shDB_Index);

    if($Result === false)
    {     
       // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
       global $cnf_DB_QUERY_ERROR;
       OnError($cnf_DB_QUERY_ERROR);        
    }   
    $Data = mysqli_fetch_Assoc($Result);   

    // 5. 회원가입되지 않은 유저라면, 에러 리턴.
    if($Data === null)
    {
        // 시스템로그 남김 (회원가입되지 않은 유저)
        if($Key == 'accountno')
        {
            $errorstring = 'SearchUser -> Not User Create. accountno : ' . "$Value";
            LOG_System($Value, $_SERVER['PHP_SELF'], $errorstring);
        }

        else if($Key == 'email')
        {
            $errorstring = 'SearchUser -> Not User Create. email : ' . "$Value";
            LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);
        }       

        // 실패 패킷 전송 후 php 종료하기 회원가입 자체가 안되어 있음)
        global $cnf_CONTENT_ERROR_SELECT_NOT_CREATE_USER;
        OnError($cnf_CONTENT_ERROR_SELECT_NOT_CREATE_USER); 
    }   
    
    mysqli_free_result($Result);    

    // 6. dbno와 AccountNo 리턴    
    return $Data;
}
// dbno를 이용해 해당 유저의 정보가 있는 shDB_Data의 접속 정보를 알아온다. (db의 Ip, Port 등..)
function shDB_Data_ConnectInfo($dbno, $accountno)
{
    // 1. shdb_info의, 어떤 Slave에게 접속할 것인지 결정
    global $Info_SlaveCount, $Info_Slave_DB_IP,  $Info_Slave_DB_ID, $Info_Slave_DB_Password, $Info_Slave_DB_PORT, $Info_Slave_DB_Name;  
    
    $Index = rand() % $Info_SlaveCount;      

    // 2. DB에 접속
    $shDB_Info;
    if(DB_Connect($shDB_Info, $Info_Slave_DB_IP[$Index], $Info_Slave_DB_ID[$Index], $Info_Slave_DB_Password[$Index], $Info_Slave_DB_Name[$Index], $Info_Slave_DB_PORT[$Index]) === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DBInfo Slave연결 에러. 따로 에러 없어서 그냥 DBIndexSlave 연결 에러로 보냄)
        global $cnf_DB_INDEX_SLAVE_CONNECT_ERROR;
        OnError($cnf_DB_INDEX_SLAVE_CONNECT_ERROR);      
    }   

    // 3. dbno를 이용해, shDB_Info.dbconnect에서 db의 정보를 가져온다. (Slave 에게) 
    $Result = DB_Query("SELECT * FROM `shDB_Info`.`dbconnect` WHERE dbno = $dbno", $shDB_Info, $accountno);
    DB_Disconnect($shDB_Info);

    if($Result === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR, $accountno);  
    }

    // 4. 결과 빼오기
    $dbInfoData = mysqli_fetch_Assoc($Result);
    mysqli_free_result($Result); 

    // 5. 접속 정보가 들어있는 array 리턴
    return $dbInfoData;
}
// 인자로 받은 정보를 이용해 shDB_Data에 Connect 하기
function shDB_Data_Conenct($dbInfoData)
{
    // 1. 찾아온 DB로 커넥트 
    $shDB_Data;
    if(DB_Connect($shDB_Data, $dbInfoData['ip'], $dbInfoData['id'], $dbInfoData['pass'], $dbInfoData['dbname'], $dbInfoData['port']) === false)
    {
        // 시스템로그 남김 (shDB_Data 연결 실패)
        $errorstring = 'shDB_Data_Conenct -> DBConnect Error';
        LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);

        // 실패 시, 실패 응답 보낸 후 php 종료 (DBData 연결 에러)
        global $cnf_DB_DATA_CONNECT_ERROR;
        OnError($cnf_DB_DATA_CONNECT_ERROR,$Data['accountno']);  
    }

    return $shDB_Data;
}
// shDB_Data에서 데이터를 Select
function shDB_Data_Select($accountNo, &$shDB_Data, $dbname, $TBLName)
{
    // 1. SELECT 쿼리문 만들어서 날리기
    $Result = DB_Query("SELECT * FROM `$dbname`.`$TBLName` WHERE `accountno` = $accountNo" , $shDB_Data, $accountNo);

    // 2. 쿼리 날리는데 실패했으면, 실패 이유에 따라 응답패킷 보낸다.
    if($Result === false)
    {
        $Error = mysqli_errno($shDB_Data);
        DB_Disconnect($shDB_Data);

        // -- 컬럼이 존재하지 않을 경우 (1054 에러)
        if($Error == 1054)
        {
            // 실패 응답 보낸 후 php 종료 (컬럼 존재하지 않음)
            global $cnf_DB_NOT_FIND_COLUMN;
            OnError($cnf_DB_NOT_FIND_COLUMN, $accountNo);  
        }      
        
        // 테이블이 존재하지 않을 경우 (1146 에러)
        else if($Error == 1146)
        {
            // 실패 응답 보낸 후 php 종료 (컬럼 존재하지 않음)
            global $cnf_DB_NOT_FOUNT_TABLE;
            OnError($cnf_DB_NOT_FOUNT_TABLE, $accountNo);  
        }  

        // 그 외는 일반 DB 쿼리 에러
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR, $accountNo);  
    }

    // 3. 쿼리에 성공했으면, fetch로 데이터 뽑아온 후 Free.
    $RetData = mysqli_fetch_Assoc($Result);
    mysqli_free_Result($Result);
      
    // 4. 혹시 없는 유저인가 체크
    if($RetData === null)
    {
        // 존재하지 않는다면, shDB_Index.allocate에서 해당 accountno의 유저 삭제.
        // 정상 삭제 후 실패 패킷 리턴.
        DB_Disconnect($shDB_Data);

        // 시스템로그 남김
        $errorstring = "shDB_Data_AccountNoCheck--> Not FIND AccountNo(`$dbname`.`$TBLName`)!! IndexDB Rollback";
        LOG_System($accountNo, $_SERVER['PHP_SELF'], $errorstring);   
        
        // shDB_Index.allocate 롤백. (Master에게)
        global $Index_Master_DB_IP, $Index_Master_DB_ID, $Index_Master_DB_Password, $Index_Master_DB_PORT, $Index_Master_DB_Name; 

        // Master DB에 연결
        $shDB_Index;
        if(DB_Connect($shDB_Index, $Index_Master_DB_IP, $Index_Master_DB_ID, $Index_Master_DB_Password, $Index_Master_DB_Name, $Index_Master_DB_PORT) === false)
        {
            // 실패 시, 실패 응답 보낸 후 php 종료 (DBIndex Master연결 에러)
            global $cnf_DB_INDEX_MASETER_CONNECT_ERROR;
            OnError($cnf_DB_INDEX_MASETER_CONNECT_ERROR);   
        }  

        $Result = DB_Query("DELETE FROM `shDB_Index`.`allocate` WHERE `accountno` = $accountNo", $shDB_Index, $accountNo);
        DB_Disconnect($shDB_Index);

        if($Result === false)
        {
            // Master에게 쿼리 남기다 실패하면 실패 패킷 전송 후 php 종료 (DB 쿼리 에러)
            global $cnf_DB_QUERY_ERROR;
            OnError($cnf_DB_QUERY_ERROR); 
        }            

        // 롤백이 잘 됐으면, 실패 패킷 전송 후 php 종료하기 (accountTBL에 accountno가 없음 or ContentsTBL에 AccountNo가 없음)
        // ---검사한 테이블이 accountTBL이었을 경우
        if($TBLName = 'account')
        {
            global $cnf_CONTENT_ERROR_NOT_FIND_ACCOUNTNO_ACCOUNTTBL;
            OnError($cnf_CONTENT_ERROR_NOT_FIND_ACCOUNTNO_ACCOUNTTBL);      
        }    

        // ---검사한 테이블이 contentsTBL이었을 경우
        else if($TBLName = 'contents')
        {
            global $cnf_CONTENT_ERROR_NOT_FIND_ACCOUNTNO_CONTENTSTBL;
            OnError($cnf_CONTENT_ERROR_NOT_FIND_ACCOUNTNO_CONTENTSTBL);     
        }
       
    }

    // 5. 있는 유저라면 정보 리턴
    return $RetData;
}
?>