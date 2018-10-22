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

    // 연결된 DB 객체 얻기 (Slave)
    function DB_ConnectObject()
    {
        return $this->shDB_Info_ConnectDB;
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

            // 실패 시, 실패 응답 보낸 후 php 종료 (DBIndex Slave연결 에러)
            global $cnf_DB_INDEX_SLAVE_CONNECT_ERROR;
            OnError($cnf_DB_INDEX_SLAVE_CONNECT_ERROR);  
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

             // 실패 시, 실패 응답 보낸 후 php 종료 (DBIndex Master연결 에러)
             global $cnf_DB_INDEX_MASETER_CONNECT_ERROR;
             OnError($cnf_DB_INDEX_MASETER_CONNECT_ERROR);  
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

    // 연결된 DB 객체 얻기 (Slave)
    function DB_ConnectObject()
    {
        return $this->shDB_Index_ConnectDB;
    }

    // 연결된 DB 객체 얻기 (Slave)
    function DB_ConnectObject_MASTER()
    {
         return $this->shDB_Index_ConnectDB_MASTER;
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
// Parameter : returnCode(실패 result)
// return : 없음
// ---------------------------------
function OnError($result, $AccountNo = -1)
{

    $Response['result'] = $result;

    // 실패 패킷 전송
    ResponseJSON($Response, $AccountNo);

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
// Parameter : 없음
// return : 성공 시 fetch한 데이터 (result 아님)
//        : 실패 시 내부에서 응답패킷 보낸 후 exit
// -------------------------------------------------------------------
function GetAvailableDBno()
{
    $shDB_Info = CshDB_Info_Contents::getInstance(); // 생성과 동시에 연결된다.  
    $Result = $shDB_Info->DB_Query("SELECT * FROM `shDB_Info`.`available` ORDER BY `available` DESC limit 1");
    if($Result === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR);  
    }

    $dbData = mysqli_fetch_Assoc($Result);
    mysqli_free_result($Result); 

    return $dbData;
}

// -------------------------------------------------------------------
// shDB_Index.Allocate 테이블에 유저 추가하기
// '마스터'한테 Write한다.
//
// Parameter : 요청자의 email, dbno
// return : 성공 시, 해당 유저에게 할당된 AccountNo 리턴.
//          문제 발생 시, 상황에 맞춰 실패 응답을 클라에게 보낸 후 exit
// -------------------------------------------------------------------
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
         // 실패 패킷 전송 후 php 종료하기 (DB 쿼리 에러)
         global $cnf_DB_QUERY_ERROR;
         OnError($cnf_DB_QUERY_ERROR);  
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
        OnError($cnf_CONTENT_ERROR_CREATE_EMAIL_DUPLICATE);           
    }        


    // 3. 존재하지 않는다면 Insert (Master에게 Write)
    if($shDB_Index->DB_Query_MASTER("INSERT INTO `shDB_Index`.`allocate`(`email`, `dbno`) VALUES ('$email', $dbno )") === false)
    {
        // 실패했다면, 실패 패킷 전송 후 php 종료하기 (DB 쿼리 에러)
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR);  
    }

    // 4. 해당 유저에게 할당된 AccountNo 리턴
    return $shDB_Index->LAST_INSERT_ID();
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
        // 만약, 에러가 1062(중복값)라면, 중복 응답 리턴.
        if(mysqli_errno($shDB_Data) == 1062)
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
        // 만약, 에러가 1062(중복값)라면, 중복 응답 리턴.
        if(mysqli_errno($shDB_Data) == 1062)
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
// Parameter : dbno, available
// return : 없음
// -----------------------------------------------------------------
function MinusAvailable($dbno, $Available)
{
     // dbno의 available을 1 감소.  
     $shDB_Info = CshDB_Info_Contents::getInstance();
     if($shDB_Info->DB_Query("UPDATE `shDB_Info`.`available` SET available = $Available - 1 WHERE dbno = $dbno") === false)
     {
         // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
         global $cnf_DB_QUERY_ERROR;
         OnError($cnf_DB_QUERY_ERROR, $AccountNo);  
     }   

}



// -------------------------------------------------------------------
// 인자로 받은 accountno 혹은 email을 이용해 해당 유저의 dbno와 AccountNo를 알아온다.
//
// Parameter : Key, Value
// return : 해당 유저의 dbno, AccountNo 리턴 (밖에서는 $Data['dbno'], $Data['accountno']와 같은 형식으로 사용.)
//        : 실패 시, 적절한 실패 응답을 클라에게 보낸다.
// -------------------------------------------------------------------
function SearchUser($TempKey, $TempValue)
{ 
    $shDB_Index = CshDB_Index_Contents::getInstance();

    // 1. Update 쿼리문을 만든다.
    // Key, Value 안전하게 가져오기
    $Key = mysqli_real_escape_string($shDB_Index->DB_ConnectObject(), $TempKey);
    $Value = mysqli_real_escape_string($shDB_Index->DB_ConnectObject(), $TempValue);    

    // 쿼리문 만들기
    if($Key == 'accountno')
        $Query = "SELECT `accountno`, `dbno` FROM `shDB_Index`.`allocate` WHERE accountno = $Value";

    else if($Key == 'email')
        $Query = "SELECT `accountno`,`dbno` FROM `shDB_Index`.`allocate` WHERE email = '$Value'";

    // 둘 다 아니면 파라미터 에러
    else
    {
        // 시스템로그 남김
        $errorstring = 'SearchUser -> ParameterError. Value : ' . "$Value";
        LOG_System(-1, $_SERVER['PHP_SELF'], $errorstring);
        
        // 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
        global $cnf_CONTENT_ERROR_PARAMETER;
        OnError($cnf_CONTENT_ERROR_PARAMETER);  
    }  



   // 2. shDB_Index.allocate로 쿼리문 날림  
   $Result = $shDB_Index->DB_Query($Query);
   if($Result === false)
   {
       // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
       global $cnf_DB_QUERY_ERROR;
       OnError($cnf_DB_QUERY_ERROR);        
   }   

    $Data = mysqli_fetch_Assoc($Result);   



    // 3. 회원가입되지 않은 유저라면, 에러 리턴.
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

    // 4. dbno와 AccountNo 리턴    
    return $Data;
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
    $shDB_Info = CshDB_Info_Contents::getInstance();

    // dbno를 이용해, shDB_Info.dbconnect에서 db의 정보를 가져온다. (Slave 에게) 
    $Result = $shDB_Info->DB_Query("SELECT * FROM `shDB_Info`.`dbconnect` WHERE dbno = $dbno");
    if($Result === false)
    {
        // 실패 시, 실패 응답 보낸 후 php 종료 (DB 쿼리 에러)
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR, $accountno);  
    }

    $dbInfoData = mysqli_fetch_Assoc($Result);
    mysqli_free_result($Result); 

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

// -------------------------------------------------------------------
// 해당 db에 accountNo가 있는지 확인.
// shDB_Data 전용
//
// Parameter : accountNo, 연결된 DB 변수(Out), DB 이름, 테이블 이름
// return : 없음
//        : 실패 시, 내부에서 적절한 실패 응답 보낸 후 Exit;
// -------------------------------------------------------------------
function shDB_Data_AccountNoCheck($accountNo, &$shDB_Data, $dbname, $TBLName)
{ 
    // 1. shDB_Data.$TBLName에 해당 accountNo가 있는지 확인 (Slave 에게)
    $Query = "SELECT EXISTS (SELECT * FROM `$dbname`.`$TBLName` WHERE accountno = $accountNo) AS success";
    $Result = DB_Query($Query , $shDB_Data, $accountNo);
    $AccountNoCheck = mysqli_fetch_Assoc($Result);

    mysqli_free_Result($Result);

    // 존재하지 않는다면, shDB_Index.allocate에서 해당 accountno의 유저 삭제.
    // 정상 삭제 후 실패 패킷 리턴.
    if($AccountNoCheck['success'] == 0)
    {
        // 시스템로그 남김
        $errorstring = "shDB_Data_AccountNoCheck--> Not Found AccountNo(`$dbname`.`$TBLName`)!! IndexDB Rollback";
        LOG_System($accountNo, $_SERVER['PHP_SELF'], $errorstring);   
        
        // shDB_Index.allocate 롤백. (Master에게)
        $shDB_Index = CshDB_Index_Contents::getInstance();
        if($shDB_Index->DB_Query_MASTER("DELETE FROM `shDB_Index`.`allocate` WHERE `accountno` = $accountNo") === false)
        {
            // Master에게 쿼리 남기다 실패하면 실패 패킷 전송 후 php 종료 (DB 쿼리 에러)
            global $cnf_DB_QUERY_ERROR;
            OnError($cnf_DB_QUERY_ERROR); 
        }

        // 롤백이 잘 됐으면, 실패 패킷 전송 후 php 종료하기 (accountTBL에 accountno가 없음 or ContentsTBL에 AccountNo가 없음)
        // ---검사한 테이블이 accountTBL이었을 경우
        if($TBLName = 'account')
        {
            global $cnf_CONTENT_ERROR_NOT_FOUND_ACCOUNTNO_ACCOUNTTBL;
            OnError($cnf_CONTENT_ERROR_NOT_FOUND_ACCOUNTNO_ACCOUNTTBL);      
        }    

        // ---검사한 테이블이 contentsTBL이었을 경우
        else if($TBLName = 'contents')
        {
            global $cnf_CONTENT_ERROR_NOT_FOUND_ACCOUNTNO_CONTENTSTBL;
            OnError($cnf_CONTENT_ERROR_NOT_FOUND_ACCOUNTNO_CONTENTSTBL);     
        }

    }   
}

// -------------------------------------------------------------------
// shDB_Data에 인자로 받은 데이터를 Update
// 
// Parameter : accountNo, 연결된 DB 변수(Out), DB 이름, 테이블 이름, Update할 데이터의 Key/Value 배열
// return : 없음
// -------------------------------------------------------------------
function shDB_Data_Update($accountNo, &$shDB_Data, $dbname, $TBLName, $Content_Body)
{
    // 1. Update 구문에서 Set 할 Text 제작.
    // 밖에서 Next할 떄 있다고 했으니 무조건 파라미터 1개는 있다고 가정
    $Key = mysqli_real_escape_string($shDB_Data, key($Content_Body));
    $Value = mysqli_real_escape_string($shDB_Data, current($Content_Body));
    $SetText = "`$Key` = '$Value'";

    // 키 1개 복사했으니 1칸 옆으로 넘겨둔다? 내부 배열 포인터가 어떤식인지 모르겠지만..이렇게 해야 됨
    next($Content_Body);

    while(true)
    {
        // next가 없으면, 다 뺀것.
        if(next($Content_Body) === false)
            break;
        
        $Key = mysqli_real_escape_string($shDB_Data, key($Content_Body));
        $Value = mysqli_real_escape_string($shDB_Data, current($Content_Body));
        $SetText .= " ,`$Key` = '$Value'";
    }
   

    // 2. Update 쿼리문 만들어서 날리기
    $Result = DB_Query("UPDATE `$dbname`.`$TBLName` SET $SetText WHERE accountno =  $accountNo" , $shDB_Data, $accountNo);

    // 3. 쿼리 날리는데 실패했으면, 실패 이유에 따라 응답패킷 보낸다.
    if($Result === false)
    {
        // -- 컬럼이 존재하지 않을 경우 (1054 에러)
        if(mysqli_errno($shDB_Data) == 1054)
        {
            // 실패 응답 보낸 후 php 종료 (컬럼 존재하지 않음)
            global $cnf_DB_NOT_FOUND_COLUMN;
            OnError($cnf_DB_NOT_FOUND_COLUMN, $accountNo);  
        }   
        
        // 테이블이 존재하지 않을 경우 (1146 에러)
        else if(mysqli_errno($shDB_Data) == 1146)
        {
            // 실패 응답 보낸 후 php 종료 (테이블 존재하지 않음)
            global $cnf_DB_NOT_FOUNT_TABLE;
            OnError($cnf_DB_NOT_FOUNT_TABLE, $accountNo);  
        }  

        // 그 외는 일반 DB 쿼리 에러
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR, $accountNo);  
    }
}

// -------------------------------------------------------------------
// shDB_Data에서 데이터를 Select
// 
// Parameter : accountNo, 연결된 DB 변수(Out), DB 이름, 테이블 이름
// return : Select 된 데이터 (result 아님. Fetch 후 데이터)
// -------------------------------------------------------------------
function shDB_Data_Select($accountNo, &$shDB_Data, $dbname, $TBLName)
{
    // 1. SELECT 쿼리문 만들어서 날리기
    $Result = DB_Query("SELECT * FROM `$dbname`.`$TBLName` WHERE `accountno` = $accountNo" , $shDB_Data, $accountNo);

    // 2. 쿼리 날리는데 실패했으면, 실패 이유에 따라 응답패킷 보낸다.
    if($Result === false)
    {
        // -- 컬럼이 존재하지 않을 경우 (1054 에러)
        if(mysqli_errno($shDB_Data) == 1054)
        {
            // 실패 응답 보낸 후 php 종료 (컬럼 존재하지 않음)
            global $cnf_DB_NOT_FOUND_COLUMN;
            OnError($cnf_DB_NOT_FOUND_COLUMN, $accountNo);  
        }      
        
        // 테이블이 존재하지 않을 경우 (1146 에러)
        else if(mysqli_errno($shDB_Data) == 1146)
        {
            // 실패 응답 보낸 후 php 종료 (컬럼 존재하지 않음)
            global $cnf_DB_NOT_FOUNT_TABLE;
            OnError($cnf_DB_NOT_FOUNT_TABLE, $accountNo);  
        }  

        // 그 외는 일반 DB 쿼리 에러
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR, $accountNo);  
    }

    // 3. 잘 됐으면, fetch로 데이터 뽑아온 후 Free까지 한 후 리턴한다.
    $RetData = mysqli_fetch_Assoc($Result);
    mysqli_free_Result($Result);

    return $RetData;
}



?>