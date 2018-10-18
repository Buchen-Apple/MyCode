<?php
// ******************************
// Create.php
// Update_account.php
// Update_Contents.php
// Select_account.php
// Select_Contents.php 에서 사용할 Library.
// ******************************
require_once('/../LIBRARY/_Error_Handling_LIbrary.php');
require_once('_LOG_Config.php');
require_once('_LOG_Profile.php');
require_once('_DB_Library.php');
require_once('_DB_Config.php');
require_once('ContentsErrorCode.php');

////////////////////////////////////
// 클래스
////////////////////////////////////

// ---------------------------------
// shDB_Info 클래스
// shDB_Info DB에 연결, 해제, Query 날리기 등 가능
// ---------------------------------
class CshDB_Info_Contents
{
    // shDB_Info의 Mysql 정보. (Slave)
    private $shDB_Info_IP = '';
    private $shDB_Info_ID = '';
    private $shDB_Info_Password = '';
    private $shDB_Info_Name = '';
    private $shDB_Info_Port = 0;

    // shDB와 연결된 연결정보. (Slave)
    private $shDB_Info_ConnectDB = 0;

    // 프로파일링 객체
    private $m_PF = 0; 


    // --------------
    // 생성자 (싱글톤이기 때문에 private 생성자)
    // --------------
    private function __construct()
    {   
        // 1. shdb_info의, 어떤 Slave에게 접속할 것인지 결정해 멤버변수에 셋팅
        global $Info_SlaveCount;

        global $Info_Slave_DB_IP;
        global $Info_Slave_DB_ID;
        global $Info_Slave_DB_Password;
        global $Info_Slave_DB_PORT;
        global $Info_Slave_DB_Name;
       
        $Index = rand() % $Info_SlaveCount;
        
        $this->shDB_Info_IP = $Info_Slave_DB_IP[$Index];
        $this->shDB_Info_ID = $Info_Slave_DB_ID[$Index];
        $this->shDB_Info_Password = $Info_Slave_DB_Password[$Index];
        $this->shDB_Info_Name = $Info_Slave_DB_Name[$Index];
        $this->shDB_Info_Port = $Info_Slave_DB_PORT[$Index];
        

        // 2. 프로파일링 얻어오기
        global $cnf_PROFILING_LOG_URL;      
        $this->m_PF = Profiling::getInstance($cnf_PROFILING_LOG_URL, $_SERVER['PHP_SELF']);

        // 3. DB에 연결
        // DB에 접속
        $this->shDB_Info_ConnectDB = mysqli_connect($this->shDB_Info_IP, $this->shDB_Info_ID, $this->shDB_Info_Password, $this->shDB_Info_Name, $this->shDB_Info_Port);

        if(!$this->shDB_Info_ConnectDB)
        {        
            // DB 연결중 문제가 생겼으면 시스템로그 남김
            $errorstring = 'Unable to connect to MySQL(Info_SlaveConnect Function) : ' . mysqli_connect_error($this->shDB_Info_ConnectDB). ' ErrorNo : ' .  mysqli_connect_errno($this->shDB_Info_ConnectDB);
            LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

            require_once('/../LIBRARY/_Clenup.php');

            // 실패 시 exit
            exit;
        }        
    }

     // ---------------
    // 객체 복사, 객체 직렬화, 객체 역직렬화 함수들을 private로 막기
    // 이걸 해야, 안정적. 원하지 않는 복사 등이 일어나지 않음.
    // ---------------
    private function __clone() {}
    private function __sleep() {}
    private function __wakeup() {} 
    
    // --------------
    // 소멸자
    // --------------
    function __destruct()
    {
        // 1. DB와 연결 해제한다.
        mysqli_close($this->shDB_Info_ConnectDB);
    }    


    // ---------------------
    // 외부에서 사용 가능 함수
    // ---------------------

    // 싱글톤 객체 얻기
    static function getInstance()
    {
        static $instance;

        // $instance가 존재하지 않다면, 새로 만든다.
        if(!isset($instance))
            $instance = new CshDB_Info_Contents();

        // php 클래스는, 리턴 시 복사값이 아니라 그냥 그 변수의 참조자가 나간다.
        // 그렇다고 int형이 참조자가 나간다거나 그런건 아님
        // 동적할당했으니 그냥 당연한거..
        return $instance;
    }

    // 쿼리 날리기
    //
    // Parameter : Query, AccountNO(디폴트 -1)
    // return : 성공 시 result 리턴
    //        : 실패 시 false 리턴
    function DB_Query($Query, $AccountNo = -1)
    {
        // 쿼리문, 시스템 로그에 남기기
        LOG_System($AccountNo, $_SERVER['PHP_SELF'], $Query);

        // 쿼리문 프로파일링 시작
        $this->m_PF->startCheck(PF_MYSQL_QUERY);    

        $Result = mysqli_query($this->shDB_Info_ConnectDB, $Query);

        // 쿼리문 프로파일링 끝
        $this->m_PF->stopCheck(PF_MYSQL_QUERY, $Query);
        if(!$Result)
        {
            // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
            $errorstring = 'DB Query Erro : ' . mysqli_error($this->shDB_Info_ConnectDB). ' ErrorNo : ' .  mysqli_errno($this->shDB_Info_ConnectDB) . " / [Query] {$Query}";
            LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

            return false;
        }

        return $Result;
    }

    // 가장 최근에 Insert된 자료의 ID 반환.
    function LAST_INSERT_ID()
    {
        return mysqli_insert_id($this->shDB_Info_ConnectDB);
    }


}


// ---------------------------------
// shDB_Index의 클래스
// shDB_Index DB에 연결, 해제, Query 날리기 등 가능
// 
// 내부에서, Master와 Slave가 따로 Connect 된다.
// ---------------------------------
class CshDB_Index_Contents
{      
    // 프로파일링 객체
    private $m_PF = 0; 

    // ---------- Slave 정보
    // shDB_Info의 Mysql 정보. (Slave)
    private $shDB_Index_IP = '';
    private $shDB_Index_ID = '';
    private $shDB_Index_Password = '';
    private $shDB_Index_Name = '';
    private $shDB_Index_Port = 0;
  
    // shDB와 연결된 연결정보. (Slave)
    private $shDB_Index_ConnectDB = 0;


    // ---------- Master 정보
    // shDB_Info의 Mysql 정보. (Master)
    private $shDB_Index_IP_MASTER = '';
    private $shDB_Index_ID_MASTER = '';
    private $shDB_Index_Password_MASTER = '';
    private $shDB_Index_Name_MASTER = '';
    private $shDB_Index_Port_MASTER = 0;
  
    // shDB와 연결된 연결정보. (Master)
    private $shDB_Index_ConnectDB_MASTER = 0;

    // --------------
    // 생성자 (싱글톤이기 때문에 private 생성자)
    // --------------
    private function __construct()
    {     
         // 1. 프로파일링 얻어오기
         global $cnf_PROFILING_LOG_URL;   
         $this->m_PF = Profiling::getInstance($cnf_PROFILING_LOG_URL, $_SERVER['PHP_SELF']);
        

        // 2. shdb_Index의, 어떤 Slave에게 접속할 것인지 결정해 멤버변수에 셋팅
        global $Index_SlaveCount;

        global $Index_Slave_DB_IP;
        global $Index_Slave_DB_ID;
        global $Index_Slave_DB_Password;
        global $Index_Slave_DB_PORT;
        global $Index_Slave_DB_Name;
       
        $Index = rand() % $Index_SlaveCount;
        
        $this->shDB_Index_IP = $Index_Slave_DB_IP[$Index];
        $this->shDB_Index_ID = $Index_Slave_DB_ID[$Index];
        $this->shDB_Index_Password = $Index_Slave_DB_Password[$Index];
        $this->shDB_Index_Name = $Index_Slave_DB_Name[$Index];
        $this->shDB_Index_Port = $Index_Slave_DB_PORT[$Index];
              

        // 3. Slave DB에 연결
        // DB에 접속
        $this->shDB_Index_ConnectDB = mysqli_connect($this->shDB_Index_IP, $this->shDB_Index_ID, $this->shDB_Index_Password, $this->shDB_Index_Name, $this->shDB_Index_Port);

        if(!$this->shDB_Index_ConnectDB)
        {        
            // DB 연결중 문제가 생겼으면 시스템로그 남김
            $errorstring = 'Unable to connect to MySQL(Index_SlaveConnect Function) : ' . mysqli_connect_error($this->shDB_Index_ConnectDB). ' ErrorNo : ' .  mysqli_connect_errno($this->shDB_Index_ConnectDB);
            LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

            require_once('/../LIBRARY/_Clenup.php');

            // 실패 시 exit
            exit;
        }        


        // 4. Master 셋팅
        global $Index_Master_DB_IP;
        global $Index_Master_DB_ID;
        global $Index_Master_DB_Password;
        global $Index_Master_DB_PORT;
        global $Index_Master_DB_Name;

        $this->shDB_Index_IP_MASTER = $Index_Master_DB_IP;
        $this->shDB_Index_ID_MASTER = $Index_Master_DB_ID;
        $this->shDB_Index_Password_MASTER = $Index_Master_DB_Password;
        $this->shDB_Index_Name_MASTER = $Index_Master_DB_Name; 
        $this->shDB_Index_Port_MASTER = $Index_Master_DB_PORT;


        // 3. Master DB에 연결
        // DB에 접속
        $this->shDB_Index_ConnectDB_MASTER = mysqli_connect($this->shDB_Index_IP_MASTER, $this->shDB_Index_ID_MASTER, $this->shDB_Index_Password_MASTER, $this->shDB_Index_Name_MASTER, $this->shDB_Index_Port_MASTER);

        if(!$this->shDB_Index_ConnectDB_MASTER)
        {        
            // DB 연결중 문제가 생겼으면 시스템로그 남김
            $errorstring = 'Unable to connect to MySQL(Index_SlaveConnect Function) : ' . mysqli_connect_error($this->shDB_Index_ConnectDB_MASTER). ' ErrorNo : ' .  mysqli_connect_errno($this->shDB_Index_ConnectDB_MASTER);
            LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

            require_once('/../LIBRARY/_Clenup.php');

            // 실패 시 exit
            exit;
        }   
    }

     // ---------------
    // 객체 복사, 객체 직렬화, 객체 역직렬화 함수들을 private로 막기
    // 이걸 해야, 안정적. 원하지 않는 복사 등이 일어나지 않음.
    // ---------------
    private function __clone() {}
    private function __sleep() {}
    private function __wakeup() {} 



     // --------------
    // 소멸자
    // --------------
    function __destruct()
    {
        // 1. Slave DB와 연결 해제한다.
        mysqli_close($this->shDB_Index_ConnectDB);

        // 2. Master DB와 연결 해제한다.
        mysqli_close($this->shDB_Index_ConnectDB_MASTER);


    }    


    // ---------------------
    // 외부에서 사용 가능 함수
    // ---------------------

    // 싱글톤 객체 얻기
    static function getInstance()
    {
        static $instance;

        // $instance가 존재하지 않다면, 새로 만든다.
        if(!isset($instance))
            $instance = new CshDB_Index_Contents();

        // php 클래스는, 리턴 시 복사값이 아니라 그냥 그 변수의 참조자가 나간다.
        // 그렇다고 int형이 참조자가 나간다거나 그런건 아님
        // 동적할당했으니 그냥 당연한거..
        return $instance;
    }

    // Slave에게 쿼리 날리기
    //
    // parameter : 쿼리, AccountNo(디폴트 -1)
    // return : 성공 시, result 리턴. 
    //          실패 시 false 리턴
    function DB_Query($Query, $AccountNo = -1)
    {
        // 쿼리문, 시스템 로그에 남기기
        LOG_System($AccountNo, $_SERVER['PHP_SELF'], $Query);

        // 쿼리문 프로파일링 시작
        $this->m_PF->startCheck(PF_MYSQL_QUERY);    

        $Result = mysqli_query($this->shDB_Index_ConnectDB, $Query);

        // 쿼리문 프로파일링 끝
        $this->m_PF->stopCheck(PF_MYSQL_QUERY, $Query);
        if(!$Result)
        {
            // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
            $errorstring = 'DB Query Erro : ' . mysqli_error($this->shDB_Index_ConnectDB). ' ErrorNo : ' .  mysqli_errno($this->shDB_Index_ConnectDB) . " / [Query] {$Query}";
            LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

            return false;             
        }

        return $Result;
    }

    // Master에게 쿼리 날리기
    //
    // parameter : 쿼리, AccountNo(디폴트 -1)
    // return : 성공 시, result 리턴. 
    //          실패 시 false 리턴
    function DB_Query_MASTER($Query, $AccountNo = -1)
    {
        // 쿼리문, 시스템 로그에 남기기
        LOG_System($AccountNo, $_SERVER['PHP_SELF'], $Query);

        // 쿼리문 프로파일링 시작
        $this->m_PF->startCheck(PF_MYSQL_QUERY);    

        $Result = mysqli_query($this->shDB_Index_ConnectDB_MASTER, $Query);

        // 쿼리문 프로파일링 끝
        $this->m_PF->stopCheck(PF_MYSQL_QUERY, $Query);
        if(!$Result)
        {
            // DB에 쿼리 날리는 중, 문제가 생겼으면 시스템로그 남김
            $errorstring = 'DB Query Erro : ' . mysqli_error($this->shDB_Index_ConnectDB_MASTER). ' ErrorNo : ' .  mysqli_errno($this->shDB_Index_ConnectDB_MASTER) . " / [Query] {$Query}";
            LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);

            return false;
        }

        return $Result;

    }

    // 가장 최근에 Insert된 자료의 ID 반환. (마스터 기준.)
    // 슬레이브에게는 Insert하지 않으니 의미 없음!
    //
    // return : Master에 가장 최근에 Insert한 ID.
    function LAST_INSERT_ID()
    {
        return mysqli_insert_id($this->shDB_Index_ConnectDB_MASTER);
    }    
}




////////////////////////////////////
// 특수 기능 함수
////////////////////////////////////

// ---------------------------------
// 에러 발생 시, 실패패킷 전송 후 exit하는 함수
//
// Parameter : returnCode, accountNo(디폴트 -1). email(디폴트 -1), dbno(디폴트 -1)
// return : 없음
// ---------------------------------
function OnError($result, $accountNo = -1, $email = -1, $dbno = -1)
{
    // 이메일 중복 result 만들기
    $Response['result'] = $result;
    $Response['accountno'] = $accountNo; // 아직 유저 no 모름.
    $Response['email'] = $email;
    $Response['dbno'] = $dbno;

    // 전송
    ResponseJSON($Response, $accountNo);

    // ---------------------------------------
    // cleanup 체크.
    // 이 안에서는 [DB 연결 해제, 프로파일러 보내기, 게임로그 보내기]를 한다.
    require_once('/../LIBRARY/_Clenup.php');
    // --------------------------------------

    exit;
}





////////////////////////////////////
// 컨텐츠 기능 함수
////////////////////////////////////

// -------------------------------------------------------------------
// shDB_Info.available 테이블에서 가장 여유분이 많은 dbno를 알아온다.
// 
// Parameter : 요청자의 email
// return : 성공 시 fetch한 데이터 (result 아님)
//        : 실패 시 내부에서 응답패킷 보낸 후 exit
// -------------------------------------------------------------------
function GetAvailableDBno($email)
{
    $shDB_Info = CshDB_Info_Contents::getInstance(); // 생성과 동시에 연결된다.  
    $Result = $shDB_Info->DB_Query("SELECT * FROM `shDB_Info`.`available` ORDER BY `available` DESC limit 1");
    if($Result === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB Index 할당 에러)
        global $cnf_CONTENT_ERROR_CREATE_DB_INDEX_ALLOCATE;
        OnError($cnf_CONTENT_ERROR_CREATE_DB_INDEX_ALLOCATE, -1, $email, -1);  
    }

    $dbData = mysqli_fetch_Assoc($Result);
    mysqli_free_result($Result); 

    return $dbData;
}

// shDB_Index.Allocate 테이블에 유저 추가하기
// '마스터'한테 Write한다.
//
// Parameter : 요청자의 email, dbno
// return : 성공 시, 해당 유저에게 할당된 AccountNo 리턴.
//          문제 발생 시, 상황에 맞춰 실패 응답을 클라에게 보낸 후 exit
function UserInsert($email, $dbno)
{
    // 1. shDB_Index의 싱글톤 알아옴.
    $shDB_Index = CshDB_Index_Contents::getInstance();
    
    // 2. email을 이용해 존재하는지 확인. (Slave에게 read)
    $Query = "SELECT EXISTS (select * from `shdb_index`.`allocate` where email=  '$email') as success";
    $Result = $shDB_Index->DB_Query($Query);

    // 만약, 무언가의 이유로 실패했다면, 실패 패킷 전송 후 php 종료
    if($Result === false)
    {
         // 실패 패킷 전송 후 php 종료하기 (인덱스 할당 에러)
         global $cnf_CONTENT_ERROR_CREATE_DB_INDEX_ALLOCATE;
         OnError($cnf_CONTENT_ERROR_CREATE_DB_INDEX_ALLOCATE, -1, $email, $dbno);  
    }

    $Data = mysqli_fetch_Assoc($Result);

    mysqli_free_Result($Result);

    // 이미, 존재한다면 실패 패킷 만들어서 클라에게 응답.
    if($Data['success'] == 1)
    {
        // 이미 가입된 이메일의 유저가 또 가입요청을 했음. 시스템로그 남김
        $errorstring = 'email Duplicate : ' . " / [Query] {$Query}";
        LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);            

        // 실패 패킷 전송 후 php 종료하기
        global $cnf_CONTENT_ERROR_CREATE_EMAIL_DUPLICATE;
        OnError($cnf_CONTENT_ERROR_CREATE_EMAIL_DUPLICATE, -1, $email, $dbno);           
    }        


    // 3. 존재하지 않는다면 Insert (Master에게 Write)
    if($shDB_Index->DB_Query_MASTER("INSERT INTO `shDB_Index`.`allocate`(`email`, `dbno`) VALUES ('$email', $dbno )") === false)
    {
        // 실패했다면, 실패 패킷 전송 후 php 종료하기 (인덱스 할당 에러)
        global $cnf_CONTENT_ERROR_CREATE_DB_INDEX_ALLOCATE;
        OnError($cnf_CONTENT_ERROR_CREATE_DB_INDEX_ALLOCATE, -1, $email, $dbno);  
    }

    // 4. 해당 유저에게 할당된 AccountNo 리턴
    return $shDB_Index->LAST_INSERT_ID();
}


// -----------------------------------------------------------------
// 지정된 shDB_Data에 Connect 후, account, contents에 내용 반영
// shDB_Info.available의 available을 1 감소도 시킨다.
// 에러 발생 시 실패 응답도 보냄
// 
// Parameter : DBno, 해당 DB의 남은 available, 회원가입 요청자의 email, 회원가입 요청자의 AccountNo
// return : 없음
// -----------------------------------------------------------------
function DataSet($dbno, $DBAvailable, $email, $AccountNo)
{
    // 1. dbno를 이용해, shDB_Info.dbconnect에서 db의 정보를 가져온다.
    $shDB_Info = CshDB_Info_Contents::getInstance();
    $Result = $shDB_Info->DB_Query("SELECT * FROM `shDB_Info`.`dbconnect` WHERE dbno = $dbno");
    if($Result === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 데이터 삽입 에러)
        global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
        OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo, $email, $dbno);  
    }

    $dbInfoData = mysqli_fetch_Assoc($Result);
    mysqli_free_result($Result); 

    
    // 2. 찾아온 DB로 커넥트 
    $shDB_Data;
    if(DB_Connect($shDB_Data, $dbInfoData['ip'], $dbInfoData['id'], $dbInfoData['pass'], $dbInfoData['dbname'], $dbInfoData['port']) === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 데이터 삽입 에러)
        global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
        OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo, $email, $dbno);  
    }


    // 3. account 테이블에, 동일한 accountno나 email이 존재하는지 확인
    $Query = "SELECT EXISTS (select * from `{$dbInfoData['dbname']}`.`account` where accountno =  $AccountNo OR email = '$email') as success";
    $Result = DB_Query($Query, $shDB_Data, $AccountNo);
    if($Result === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 데이터 삽입 에러)
        global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
        OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo, $email, $dbno);  
    }

    $Data = mysqli_fetch_Assoc($Result);
    mysqli_free_Result($Result);

    // 이미, 존재한다면 실패 패킷 만들어서 클라에게 응답.
    if($Data['success'] == 1)
    {
        // 시스템로그 남김
        $errorstring = 'accountTBL Duplicate : ' . " / [Query] {$Query}";
        LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);            

        // 실패 패킷 전송 후 php 종료하기
        global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
        OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo, $email, $dbno);  
    }  


    // 4. 존재하지 않는다면, account 테이블에 Insert
    if(DB_Query("INSERT INTO `{$dbInfoData['dbname']}`.`account`(`accountno`, `email`) VALUES ($AccountNo, '$email' )", $shDB_Data, $AccountNo) === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 데이터 삽입 에러)
        global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
        OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo, $email, $dbno);  
    }


    // 5. Contents 테이블에, 동일한 accountno이 존재하는지 확인
    $Query = "SELECT EXISTS (select * from `{$dbInfoData['dbname']}`.`contents` where accountno =  $AccountNo) as success";
    $Result = DB_Query($Query, $shDB_Data, $AccountNo);
    if($Result === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 데이터 삽입 에러)
        global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
        OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo, $email, $dbno);  
    }

    $Data = mysqli_fetch_Assoc($Result);
    mysqli_free_Result($Result);

     // 이미, 존재한다면 실패 패킷 만들어서 클라에게 응답.
     if($Data['success'] == 1)
     {
         // 시스템로그 남김
         $errorstring = 'contentsTBL Duplicate : ' . " / [Query] {$Query}";
         LOG_System($AccountNo, $_SERVER['PHP_SELF'], $errorstring);            
 
         // 실패 패킷 전송 후 php 종료하기
         global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
         OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo, $email, $dbno);  
     }  
    

    // 6. 존재하지 않는다면, contents 테이블에 Insert
    if(DB_Query("INSERT INTO `{$dbInfoData['dbname']}`.`contents`(`accountno`) VALUES ($AccountNo)", $shDB_Data, $AccountNo) === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 데이터 삽입 에러)
        global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
        OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo, $email, $dbno);  
    }

    // 7. 이번에 유저를 접속시킨 dbno의 available을 1 감소.    
    if($shDB_Info->DB_Query("UPDATE `shDB_Info`.`available` SET available = $DBAvailable - 1 WHERE dbno = $dbno") === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 데이터 삽입 에러)
        global $cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT;
        OnError($cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT, $AccountNo, $email, $dbno);  
    }


    // 8. 연결했던 DB Disconenct
    DB_Disconnect($shDB_Data);
}







?>