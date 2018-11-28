<?php
// -------------------------------------------------------------------
// shDB_Info의 Slave 중 하나에 Connect하는 함수
//
// Parameter : 없음
// return : 연결된 DB. 실패 시 내부에서 클라에게 에러 리턴
// -------------------------------------------------------------------
function shDB_Info_Slave_Connect()
{
    // 1. shdb_info의, 어떤 Slave에게 접속할 것인지 결정
    global $Info_SlaveCount;
    global $Info_Slave_DB_IP;
    global $Info_Slave_DB_ID;
    global $Info_Slave_DB_Password;
    global $Info_Slave_DB_PORT;
    global $Info_Slave_DB_Name;  
    
    $Index = rand() % $Info_SlaveCount;      

    // 2. DB에 접속
    $shDB_Info_ConnectDB;
    if(DB_Connect($shDB_Info_ConnectDB, $Info_Slave_DB_IP[$Index], $Info_Slave_DB_ID[$Index], $Info_Slave_DB_Password[$Index], $Info_Slave_DB_Name[$Index], $Info_Slave_DB_PORT[$Index]) === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DBInfo Slave연결 에러. 따로 에러 없어서 그냥 DBIndexSlave 연결 에러로 보냄)
        global $cnf_DB_INDEX_SLAVE_CONNECT_ERROR;
        OnError($cnf_DB_INDEX_SLAVE_CONNECT_ERROR);      
    }    

    // 3. 결과 리턴
    return $shDB_Info_ConnectDB;
}
// -------------------------------------------------------------------
// shDB_Index의 Slave 중 하나에 Connect하는 함수
//
// Parameter : 없음
// return : 연결된 DB. 실패 시 내부에서 클라에게 에러 리턴
// -------------------------------------------------------------------
function shDB_Index_Slave_Connect()
{
    // 1. shdb_Index의, 어떤 Slave에게 접속할 것인지 결정
    global $Index_SlaveCount;
    global $Index_Slave_DB_IP;
    global $Index_Slave_DB_ID;
    global $Index_Slave_DB_Password;
    global $Index_Slave_DB_PORT;
    global $Index_Slave_DB_Name;  
  
    $Index = rand() % $Index_SlaveCount;   
    
    // 2. Master DB에 연결
    $shDB_Index_ConnectDB;
    if(DB_Connect($shDB_Index_ConnectDB, $Index_Slave_DB_IP[$Index], $Index_Slave_DB_ID[$Index], $Index_Slave_DB_Password[$Index], $Index_Slave_DB_Name[$Index], $Index_Slave_DB_PORT[$Index]) === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DBIndex Slave연결 에러)
        global $cnf_DB_INDEX_SLAVE_CONNECT_ERROR;
        OnError($cnf_DB_INDEX_SLAVE_CONNECT_ERROR);    
    }    

    // 3. 결과 리턴
    return $shDB_Index_ConnectDB; 
}
// -------------------------------------------------------------------
// shDB_Index의 Master에 Connect하는 함수
//
// Parameter : 없음
// return : 연결된 DB. 실패 시 내부에서 클라에게 에러 리턴
// -------------------------------------------------------------------
function shDB_Index_Master_Connect()
{
    // 1. Master 셋팅
    global $Index_Master_DB_IP;
    global $Index_Master_DB_ID;
    global $Index_Master_DB_Password;
    global $Index_Master_DB_PORT;
    global $Index_Master_DB_Name; 

    // 2. Master DB에 연결
    $shDB_Index_ConnectDB;
    if(DB_Connect($shDB_Index_ConnectDB, $Index_Master_DB_IP, $Index_Master_DB_ID, $Index_Master_DB_Password, $Index_Master_DB_Name, $Index_Master_DB_PORT) === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DBIndex Master연결 에러)
        global $cnf_DB_INDEX_MASETER_CONNECT_ERROR;
        OnError($cnf_DB_INDEX_MASETER_CONNECT_ERROR);   
    }   

    // 3. 결과 리턴
    return $shDB_Index_ConnectDB;   
}

// -------------------------------------------------------------------
// shDB_Info.available 테이블에서 가장 여유분이 많은 dbno를 알아온다.
// 
// Parameter : 없음
// return : 성공 시 fetch한 데이터 (result 아님)
//        : 실패 시 내부에서 응답패킷 보낸 후 exit
// -------------------------------------------------------------------
function GetAvailableDBno()
{
    // 1. shDB_Info의 Slave에게 연결
    $shDB_Info = shDB_Info_Slave_Connect();

    // 2. 쿼리 날리기
    $Result = DB_Query("SELECT * FROM `shDB_Info`.`available` ORDER BY `available` DESC limit 1", $shDB_Info);
    DB_Disconnect($shDB_Info);

    if($Result === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR);  
    }   

    // 3. 결과 뽑아오기
    $dbData = mysqli_fetch_Assoc($Result);
    mysqli_free_result($Result); 

    // 4. 만약, available 찾아온 db의 available 수치가 0일 경우
    if($dbData['available'] == 0)
    {
         // 실패 응답 보낸 후 php 종료 (DB Index 할당 에러)
         global $cnf_CONTENT_ERROR_CREATE_DB_INDEX_ALLOCATE;
         OnError($cnf_CONTENT_ERROR_CREATE_DB_INDEX_ALLOCATE);  
    }

    // 5. 결과 리턴
    return $dbData;
}
// -------------------------------------------------------------------
// shDB_Index.Allocate 테이블에 유저 추가하기
// '마스터'한테 Write한다.
//
// Parameter : 요청자의 email, dbno, (out)요청자의 email을 안전하게 가져온 후 보관할 변수
// return : 성공 시, 해당 유저에게 할당된 AccountNo 리턴.
//          문제 발생 시, 상황에 맞춰 실패 응답을 클라에게 보낸 후 exit
// -------------------------------------------------------------------
function UserInsert($Tempemail, $dbno, &$safeEmail)
{
    // 1. shDB_Index의 Slave에 연결 후, TempEmail을 안전하게 가져오기.
    // 밖에서는 연결된 DB까 없기 때문에 못가져옴.
    $shDB_Index_Slave = shDB_Index_Slave_Connect();
    $email = mysqli_real_escape_string($shDB_Index_Slave, $Tempemail);   
    $safeEmail = $email;
    
    // 2. email을 이용해 존재하는지 확인. (Slave에게 read)
    $Query = "SELECT EXISTS (select * from `shdb_index`.`allocate` where email = '$email') as success";
    $Result = DB_Query($Query, $shDB_Index_Slave);
  
    DB_Disconnect($shDB_Index_Slave);

    // 만약, 무언가의 이유로 실패했다면, 실패 패킷 전송 후 php 종료
    if($Result === false)
    { 
        // 실패 패킷 전송 후 php 종료하기 (DB 쿼리 에러)
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR);  
    }
   
    // 3. 데이터 빼오기
    $Data = mysqli_fetch_Assoc($Result);
    mysqli_free_Result($Result);

    // 4. 이미, 존재한다면 실패 패킷 만들어서 클라에게 응답.
    if($Data['success'] == 1)
    {
        // 이미 가입된 이메일의 유저가 또 가입요청을 했음. 시스템로그 남김
        $errorstring = 'email Duplicate : ' . " / [Query] {$Query}";
        LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);            

        // 실패 패킷 전송 후 php 종료하기
        global $cnf_CONTENT_ERROR_CREATE_EMAIL_DUPLICATE;
        OnError($cnf_CONTENT_ERROR_CREATE_EMAIL_DUPLICATE);           
    }        

    // 5. 존재하지 않는다면 Insert (Master에게 Write).
    // shDB_Index_Master의 Master에 연결
    $shDB_Index_Master = shDB_Index_Master_Connect();
    $Result = DB_Query("INSERT INTO `shDB_Index`.`allocate`(`email`, `dbno`) VALUES ('$email', $dbno )", $shDB_Index_Master);  
    if($Result === false)
    {
        DB_Disconnect($shDB_Index_Master);

        // 실패했다면, 실패 패킷 전송 후 php 종료하기 (DB 쿼리 에러)
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR);  
    }
    // 6. 해당 유저에게 할당된 AccountNo 알아오기
    $LastAccountNo = mysqli_insert_id($shDB_Index_Master);

    // 7. Master DB 연결 해제
    DB_Disconnect($shDB_Index_Master);

    // 8. LastAccountNo 리턴
    return $LastAccountNo;
}
// -------------------------------------------------------------------
// dbno를 이용해 해당 유저의 정보가 있는 shDB_Data의 접속 정보를 알아온다. (db의 Ip, Port 등..)
// 
// Parameter : dbno, accountno
// return : 성공 시, 접속 정보가 들어있는 array
//        : 실패 시, 내부에서 실패 패킷 보낸다.
// -------------------------------------------------------------------
function shDB_Data_ConnectInfo($dbno, $accountno)
{
    // 1. shDB_Info의 Slave에게 연결
    $shDB_Info = shDB_Info_Slave_Connect();

    // 2. dbno를 이용해, shDB_Info.dbconnect에서 db의 정보를 가져온다. (Slave 에게) 
    $Result = DB_Query("SELECT * FROM `shDB_Info`.`dbconnect` WHERE dbno = $dbno", $shDB_Info, $accountno);
    DB_Disconnect($shDB_Info);

    if($Result === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR, $accountno);  
    }

    // 3. 결과 빼오기
    $dbInfoData = mysqli_fetch_Assoc($Result);
    mysqli_free_result($Result); 

    // 4. 접속 정보가 들어있는 array 리턴
    return $dbInfoData;
}
// -------------------------------------------------------------------
// 인자로 받은 정보를 이용해 shDB_Data에 Connect 하기
//
// Parameter : DB 정보
// return : 연결된 DB 변수
// -------------------------------------------------------------------
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
// -----------------------------------------------------------------
// 지정된 shDB_Data의 account, contents에 내용 반영 (Insert)
// **Create.php에서만 사용된다.**
// 에러 발생 시 실패 응답도 보냄
// 
// Parameter : 연결된 DB 변수, DB의 이름, 요청자의 email, 요청자의 AccountNo
// return : 없음
// -----------------------------------------------------------------
function shDB_Data_CreateInsert($shDB_Data, $dbName, $email, $AccountNo)
{   
    // 1. account 테이블에 Insert
    if(DB_Query("INSERT INTO `$dbName`.`account`(`accountno`, `email`) VALUES ($AccountNo, '$email' )", $shDB_Data, $AccountNo) === false)
    {        
        $Error = mysqli_errno($shDB_Data);
        DB_Disconnect($shDB_Data);

        // 만약, 에러가 1062(중복값)라면, 중복 응답 리턴.
        if($Error == 1062)
        {
            // 시스템로그 남김
            $errorstring = 'DataSet--> accountTBL Value Duplicate.';
            LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

            // 실패 응답 보낸 후 php 종료 (DB 데이터 삽입 에러)
            global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
            OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo);  
        }

        // 그게 아니라면 DB 쿼리 에러
        else
        {
            // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
            global $cnf_DB_QUERY_ERROR;
            OnError($cnf_DB_QUERY_ERROR, $AccountNo);  
        }
    }


    // 2.contents 테이블에 Insert
    if(DB_Query("INSERT INTO `$dbName`.`contents`(`accountno`) VALUES ($AccountNo)", $shDB_Data, $AccountNo) === false)
    {
        $Error = mysqli_errno($shDB_Data);
        DB_Disconnect($shDB_Data);

        // 만약, 에러가 1062(중복값)라면, 중복 응답 리턴.
        if($Error == 1062)
        {
            // 시스템로그 남김
            $errorstring = 'DataSet--> contentsTBL Value Duplicate.';
            LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

            // 실패 응답 보낸 후 php 종료 (DB 데이터 삽입 에러)
            global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
            OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo);  
        }

        // 그게 아니라면 DB 쿼리 에러
        else
        {
            // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
            global $cnf_DB_QUERY_ERROR;
            OnError($cnf_DB_QUERY_ERROR, $AccountNo);  
        } 
    }   
}
// -----------------------------------------------------------------
// shDB_Index.allocate의 available을 1 감소.
//
// Parameter : dbno
// return : 없음
// -----------------------------------------------------------------
function MinusAvailable($dbno)
{
     // 1. shdb_info의 Master 셋팅
     global $Info_Master_DB_IP;
     global $Info_Master_DB_ID;
     global $Info_Master_DB_Password;
     global $Info_Master_DB_PORT;
     global $Info_Master_DB_Name;    
 
     // 2. DB에 접속
     $shDB_Info_Master;
     if(DB_Connect($shDB_Info_Master, $Info_Master_DB_IP, $Info_Master_DB_ID, $Info_Master_DB_Password, $Info_Master_DB_Name, $Info_Master_DB_PORT) === false)
     {
          // 실패 시, 실패 응답 보낸 후 php 종료 (DBInfo Master연결 에러. 따로 없어서 그냥 DBIndex Master 연결 에러 보냄)
          global $cnf_DB_INDEX_MASETER_CONNECT_ERROR;
          OnError($cnf_DB_INDEX_MASETER_CONNECT_ERROR);      
     }   


    // 3., dbno의 available을 1 감소.  
    $Result = DB_Query("UPDATE `shDB_Info`.`available` SET available = available - 1 WHERE dbno = $dbno", $shDB_Info_Master);
    DB_Disconnect($shDB_Info_Master);

    if($Result === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR, $AccountNo);  
    }   
}
?>