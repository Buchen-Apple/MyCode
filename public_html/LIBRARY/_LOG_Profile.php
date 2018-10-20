<?php

/*
1. 프로파일링 분류
PF_PAGE             - 전체 페이지 처리 시간
PF_LOG              - 로그 처리 시간
PF_MYSQL_CONN       - MySQL 연결 처리 시간
PF_MYSQL_QUERY      - MySQL 쿼리 처리 시간
PF_EXTAPI           - 외부 API 처리시간

2. 로그 내용은 로그 웹서버로 URL 전송된다. 로그 웹서버의 LOG_Profile.php와 연동.

3. 외부에서 사용법
$PF = Profiling::getInstance("http://xx.xx.xx.xx/Log/LOG_Profile.php", "$_SERVER['PHP_SELF']")
$PF->startCheck(PF_PAGE);

*/


// ----------------------
// define 값들
// ----------------------
define('PF_TYPE_START', 1);     // 프로파일링 define 시작. 경계처리 위한것

define('PF_PAGE', 1);             
define('PF_LOG', 2);    
define('PF_MYSQL_CONN', 3);    
define('PF_MYSQL_QUERY', 4);    
define('PF_EXTAPI', 5); 

define('PF_TYPE_END', 5);       // 프로파일링 define 끝. 경계처리 위한것.


// ----------------------
// 프로파일링 클래스
// 프로파일링의 'PF_LOG'는 시스템로그, 게임로그 총 2개에 관련된 것을 수집한다 (로그 남기는 시간)
// ----------------------
class Profiling
{
    // 로그를 남길지 말지 결정할 flag. false면 안남긴다.
    // 확률에 따라 true로 변경된다
    private $LOG_FLAG   = FALSE;

    // 로그를 남길 서버의 URL
    private $LOG_URL    = '';

    // $_PROFILING['Type']['start] : 프로파일링 시작 시간' 
    // $_PROFILING['Type']['sum']  : 프로파일링 총 합 시간.
    private $_PROFILING = array();

    // 해당 url을 보낸 php. $_SERVER['PHP_SELF'] 가 들어간다.
    private $_ACTION    = '';

    // 프로파일링 종료 시, 로그 타입이 PF_MYSQL_QUERY라면, 이 변수에 저장된다.
    private $QUERY = '';

    // 프로파일링 종료 시, 로그 타입이 PF_MYSQL_QUERY가 아니라면, 이 변수에 저장.
    private $COMMENT = '';

    // 커널모드와 유저모드 시간 저장.
    // 윈도우에서는 작동 X
    private $_ru_utime_start =0;
    private $_ru_stime_start =0;

    // --------------
    // 생성자
    // --------------
    private function __construct()
    {
        // function_exists("함수이름") : ( ) 안의 함수가 존재하는 함수인지 알아내는 함수. 존재하면 true반환 
        if(function_exists("getrusage"))
        {
            // getrusage() 함수는 시스템의 기본 사용시간을 얻어낸다.
            // 윈도우에서는 동작 X
            $dat = getrusage();

            $this->_ru_utime_start = $dat['ru_utime.tv_sec'] * 1e6 + $dat['ru_utime.tv_usec']; // 사용된 유저시간 저장
            $this->_ru_stime_start = $dat['ru_stime.tv_sec'] * 1e6 + $dat['ru_stime.tv_usec']; // 사용된 시스템 시간
        }
        else
        {
            $this->_ru_utime_start = 0;
            $this->_ru_stime_start = 0;
        }

        // 그 외 각종 값들 초기화
        $this->_PROFILING[PF_PAGE]['start']             =0;
        $this->_PROFILING[PF_PAGE]['sum']               =0;

        $this->_PROFILING[PF_LOG]['start']              =0;
        $this->_PROFILING[PF_LOG]['sum']                =0;

        $this->_PROFILING[PF_MYSQL_CONN]['start']       =0;
        $this->_PROFILING[PF_MYSQL_CONN]['sum']         =0;

        $this->_PROFILING[PF_MYSQL_QUERY]['start']      =0;
        $this->_PROFILING[PF_MYSQL_QUERY]['sum']        =0;

        $this->_PROFILING[PF_EXTAPI]['start']           =0;
        $this->_PROFILING[PF_EXTAPI]['sum']             =0;

    }

    // ---------------
    // 객체 복사, 객체 직렬화, 객체 역직렬화 함수들을 private로 막기
    // 이걸 해야, 안정적. 원하지 않는 복사 등이 일어나지 않음.
    // ---------------
    private function __clone() {}
    private function __sleep() {}
    private function __wakeup() {} 

    // --------------
    // 싱글톤 객체 얻기
    // $SaveURL : 프로파일링 로그를 저장할 서버의 URL. / LOG_Profile.php의 위치
    // $ActionPath : 해당 로그가 발생한 URL. / $_SERVER['PHP_SELF'] 이다.
    // $LogRate : 로그 저장 확률. 0~100 / 디폴트 100
    // --------------
    static function getInstance($SaveURL, $ActionPath, $LogRate = 100)
    {
        static $instance;

        // $instance가 존재하지 않다면, 새로 만든다.
        if(!isset($instance))
            $instance = new Profiling();

        // 액션 path가 있으면 넣어준다.
        if($ActionPath != '')
            $instance->_ACTION = $ActionPath;
        
        // 로그 저장 URL 셋팅
        $instance->LOG_URL = $SaveURL;

        // $LogRate에 따라 로그 저장할지 말지 결정. 플래그 변경 파트
        if(rand() % 100 < $LogRate)
            $instance->LOG_FLAG = TRUE;

        // php 클래스는, 리턴 시 복사값이 아니라 그냥 그 변수의 참조자가 나간다.
        // 그렇다고 int형이 참조자가 나간다거나 그런건 아님
        // 동적할당했으니 그냥 당연한거..

        return $instance;
    }

    // --------------
    // 프로파일링 시작. 특정 type에 대해 시작
    // 같은 type을 여러번 시작/중지하여 누적하기 가능
    // --------------
    function startCheck($Type)
    {
        // 로그 저장 FLAG가 FALSE라면, 로그 저장할 때가 아니니 걍 return;
        if($this->LOG_FLAG == FALSE)
            return;

        // 실수방지용. 타입을 벗어나면 안됨!
        if($Type < PF_TYPE_START || PF_TYPE_END < $Type)
            return;

        $this->_PROFILING[$Type]['start'] = microtime(TRUE);
    }

    // --------------
    // 프로파일링 중지
    // --------------
    function stopCheck($Type, $comment='')
    {
        // 로그 저장 FLAG가 FALSE라면, 로그 저장할 때가 아니니 걍 return;
        if($this->LOG_FLAG == FALSE)
            return;

        // 실수방지용. 타입을 벗어나면 안됨!
        if($Type < PF_TYPE_START || PF_TYPE_END < $Type)
            return;       

        // 현재 시간을 구한 후, 총 소요시간을 sum에 더한다.
        $endTime = microtime(TRUE);
        $this->_PROFILING[$Type]['sum'] += ($endTime - $this->_PROFILING[$Type]['start']);

        // 타입이 PF_MYSQL_QUERY라면, 쿼리에 저장
        if($Type == PF_MYSQL_QUERY)
            $this->QUERY .= $comment . '\n';
        
        // 타입이 PF_MYSQL_QUERY가 아니라면, comment에 저장
        else
            $this->COMMENT .= $comment . '\n';
    }

    // --------------
    // 프로파일링 로그 저장
    // --------------
    function LOG_Save($AccountNo = 0)
    {
        // 로그 저장 FLAG가 FALSE라면, 로그 저장할 때가 아니니 걍 return;
        if($this->LOG_FLAG == FALSE)
            return;


        // function_exists("함수이름") : ( ) 안의 함수가 존재하는 함수인지 알아내는 함수. 존재하면 true반환 
        if(function_exists("getrusage"))
        {
            // getrusage() 함수는 시스템의 기본 사용시간을 얻어낸다.
            // 윈도우에서는 동작 X
            $dat = getrusage();

            $_ru_utime_end = $dat['ru_utime.tv_sec'] * 1e6 + $dat['ru_utime.tv_usec']; 
            $_ru_stime_end = $dat['ru_stime.tv_sec'] * 1e6 + $dat['ru_stime.tv_usec']; 

            $utime = ($_ru_utime_end - $this->_ru_utime_start) / 1e6;       // 사용된 유저 시간
            $stime = ($_ru_stime_end - $this->_ru_stime_start) / 1e6;       // 사용된 PHP 시스템 시간
        }
        else
        {
            $utime = 0;
            $stime = 0;
        }

        // 로그 저장 URL 셋팅
        $postField = array("IP"             => $_SERVER['REMOTE_ADDR'],
                           "AccountNo"      => $AccountNo,
                           "Action"         => $this->_ACTION,
                           "T_Page"         => $this->_PROFILING[PF_PAGE]['sum'],
                           "T_MYsql_Conn"   => $this->_PROFILING[PF_MYSQL_CONN]['sum'],
                           "T_Mysql_Query"  => $this->_PROFILING[PF_MYSQL_QUERY]['sum'],
                           "T_ExtAPI"       => $this->_PROFILING[PF_EXTAPI]['sum'],
                           "T_LOG"          => $this->_PROFILING[PF_LOG]['sum'],
                           "T_ru_u"         => $utime,
                           "T_ru_s"         => $stime,
                           "Query"          => $this->QUERY,
                           "Comment"        => $this->COMMENT
                            );

        // 셋팅된 로그 전송하기.
        $Response = $this->http_Request($this->LOG_URL, $postField);           
        
    }

    private function http_Request($URL, $params)
    {
        foreach($params as $key => $val)
        {
            if(is_array($val))
                $val = implode(',', $val);
            
            $post_params[] = $key . '=' . urlencode($val);
        }

        $post_string = implode('&', $post_params);
        $parts = parse_url($URL);

        $fp = fsockopen($parts['host'], isset($parts['port']) ? $parts['port'] : 80, $errno, $error, 30);

        // 소켓 못열었으면 리턴 0
        if(!$fp)
            return 0;

        // HTTP 프로토콜 생성
        $out = "POST " . $parts['path'] . " HTTP//1.1\r\n";
        $out .= "Host: " . $parts['host'] . "\r\n";
        $out .= "Content-Type: application/x-www-form-urlencoded\r\n";
        $out .= "Content-length:" . strlen($post_string) . "\r\n";
        $out .= "Connection: Close\r\n\r\n";
        $out .= $post_string;

        // 보내기
        $Result = fwrite($fp, $out); 

        // 받기
        // $Response = fread($fp, 1000);
        // echo $Response;

        // 소켓 닫기
        fclose($fp);

        // 리턴
        return $Result;
    }



}


?>