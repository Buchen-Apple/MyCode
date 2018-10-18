<?php
/*
로그함수 및 클래스
외부 URL을 호출하여 로그를 남기도록 한다.

* 필수!!
_LOG_Config.php가 include되어 있어야 하며, 거기에는 다음 변수가 전역으로 있어야 한다.

$cnf_SYSTEM_LOG_URL  
$cnf_GAME_LOG_URL 

* 필수!!
로그 부분에도 프로파일링 들어가야 하므로, 해당 파일을 사용하는 소스에서는 _LOG_Profile.php가 include 되어야 함
프로파일링 객체가 전역으로 존재해야 한다.
*/
require_once('_LOG_Config.php');
require_once('_LOG_Profile.php');
require_once('_Socket_Library.php');

// ---------------------------
// 시스템 로그 함수
// ---------------------------
function LOG_System($AccountNo, $Action, $Message, $Level = 1)
{   
     // 전역으로 선언된 변수 or 객체들을 함수 안에서 사용하기 위해 선언.
    global $cnf_SYSTEM_LOG_URL;
    global $cnf_LOG_LEVEL;
   
    global $PF; 

    // 로그 레벨이, 로그 저장 레벨보다 크면 남기지 않음.
    if($cnf_LOG_LEVEL < $Level)
        return;
    
    // $PF가 true라면(존재한다면) 프로파일링 시작
    if($PF)
        $PF->startCheck(PF_LOG);

    // 만약, 아직 회원가입 안한? 혹은 AccountNo가 발급되지 않은 유저?
    if($AccountNo < 0)
    {
        if(array_key_exists('HTTP_X_FORWARDED_FOR', $_SERVER))
            $AccountNo = $_SERVER['HTTP_X_FORWARDED_FOR'];

        else if(array_key_exists('REMOTE_ADDR', $_SERVER))
            $AccountNo = $_SERVER['REMOTE_ADDR'];

        else
            $AccountNo = 'local';
    }

   
    $postField = array('AccountNo' => $AccountNo, 'Action' => $Action, 'Message' => $Message);

    http_request($cnf_SYSTEM_LOG_URL, $postField, 'POST');

    // $PF가 true라면(존재한다면) 프로파일링 종료
    if($PF)
        $PF->stopCheck(PF_LOG);   
}






// ---------------------------
// 게임 로그 클래스
// ---------------------------
class GAMELog
{
    private $LOG_URL = '';
    private $LogArray = array();

    // 싱글톤 객체얻기
    // $SaveURL - 게임로그를 저장할 서버의 URL / Log_Game.php의 위치
    static function getInstance($SaveURL)
    {
        static $instance;

        if( !isset($instance))
            $instance = new GAMELog;

        // 로그 저장 URL 셋팅
        $instance->LOG_URL = $SaveURL;

        return $instance;
    }

    // 로그 누적
    // 맴버변수 $LogArray에 누적 저장시킨다.
    // $AccountNo, $LogType, $LogCode, $Param1, $Param2, $Param3, $Param4, $ParamString
    function AddLog($AccountNo, $Type, $Code, $Param1 = 0, $Param2 = 0, $Param3 = 0, $Param4 = 0, $ParamString = '')
    {
        array_Push($this->LogArray, array("AccountNo"   => $AccountNo,
                                          "LogType"     => $Type,
                                          "LogCode"     => $Code,
                                          "Param1"      => $Param1,
                                          "Param2"      => $Param2,
                                          "Param3"      => $Param3,
                                          "Param4"      => $Param4,
                                          "ParamString" => $ParamString));
    }

    // 로그 저장하기.
    // 맴버변수 $LogArray에 쌓인 로그를 한 방에 저장한다.
    function SaveLog()
    {
        // 배열 안에, 내용이 0개 이상이라면 _Socket_Library.php에 있는 
        // http_request() 함수를 이용해서 게임 로그 서버로 json 전송.
        if(count($this->LogArray) > 0)
            http_request($this->LOG_URL, array("LogChunk" => json_encode($this->LogArray), 'POST'));
    }
}

?>