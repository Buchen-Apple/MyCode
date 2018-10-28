<?php
// ******************************
// get_matchmakinh.php 에서 사용할 Library.
// ******************************
require_once('_Error_Handling_LIbrary.php');
require_once('_StartUp.php');
require_once('_LOG_Config.php');
require_once('_LOG_Profile.php');
require_once('_DB_Library.php');
require_once('_DB_Config.php');
require_once('_ErrorCode.php');
require_once('_CURL_Library.php');


// sbDBAPI를 이용해 Seelct.account.php에서, 유저 데이터를 받아오는 함수.
//
// Parameter : Request 할 데이터(array)
// return : 성공 시, 알아온 유저 데이터 리턴. jsonDecode까지 한 후 리턴한다.
//        : 실패 시 클라에게 에러 리턴 후 exit
function shDBAPI_GetUserData($Request)
{ 
    // 1. select.accountdb에게 Request (CURL 이용)
    // $ret에는 $ret['Code'], $ret['body']로 나눠서 들어가 있음.
    $ret = http_post("http://127.0.0.1/Contents/Select_account.php", json_encode($Request));


    // 2. HTTP 에러 체크
    if($ret['code'] != 200)
    {
         // 실패 응답 보낸 후 php 종료 (sbDB API 서버 HTTP 에러)
         global $cnf_SHDB_API_HTTP_ERROR;
         OnError($cnf_SHDB_API_HTTP_ERROR);  
    }


    // 3. 제이슨으로 결과 가져오기
    $retBody = json_decode($ret['body'], true);


    // 4. 컨텐츠 에러 체크    
    // 재시도 요망 ----------------
    global $cnf_CONTENT_ERROR_NOT_FIND_ACCOUNTNO_ACCOUNTTBL;    // Index.allocate에는 있으나, data.account에 없음. 롤백 후 해당 에러 리턴 (-11)
    global $cnf_CONTENT_ERROR_NOT_FIND_ACCOUNTNO_CONTENTSTBL;   // Index.allocate에는 있으나, data.contents에 없음. 롤백 후 해당 에러 리턴 (-12)   
    if($retBody['result'] == $cnf_CONTENT_ERROR_NOT_FIND_ACCOUNTNO_ACCOUNTTBL || 
        $retBody['result'] == $cnf_CONTENT_ERROR_NOT_FIND_ACCOUNTNO_CONTENTSTBL)
    {
         // 실패 응답 보낸 후 php 종료 (재시도 요망)
         global $cnf_LOBBY_RETRY;
         OnError($cnf_LOBBY_RETRY);  
    }

    // 가입정보 없음 ----------------
    global $cnf_CONTENT_ERROR_SELECT_NOT_CREATE_USER;    // 회원가입 자체가 안된 상태 (-10)
    if($retBody['result'] == $cnf_CONTENT_ERROR_SELECT_NOT_CREATE_USER)
    {
         // 실패 응답 보낸 후 php 종료 (가입정보 없음 체크)
         global $cnf_LOBBY_NOT_FIND_JOIN_INFO;
         OnError($cnf_LOBBY_NOT_FIND_JOIN_INFO);  
    }


    // shDB API 서버 기타 에러 (위 에러 외 컨텐츠 에러)  ----------------
    global $cnf_COMPLETE;                   // 성공 (1)
    if($retBody['result'] != $cnf_COMPLETE)
    {
         // 실패 응답 보낸 후 php 종료 (sbDB API 서버 기타 에러)
         global $cnf_SHDB_API_ERROR;
         OnError($cnf_SHDB_API_ERROR);  
    }

    // 5. 결과 리턴
    return $retBody;
}


// MatchMaking 서버의 정보 얻어오기
//
// Parameter : accountno
// return : 성공 시, 알아온 MatchMaking 서버의 Info
//        : 실패 시, 클라에게 에러 리턴 후 exit
function GetMatchMakingServer($accountno)
{
    // 1. 접속할 MatchMakingDB의 Slave 정보 선택
    global $Matchmaking_SlaveCount;
    $Index = rand() % $Matchmaking_SlaveCount;

    // 2. MatchMakingDB에 Connect
    global $Matchmaking_Slave_DB_IP;
    global $Matchmaking_Slave_DB_ID;
    global $Matchmaking_Slave_DB_Password;
    global $Matchmaking_Slave_DB_PORT;
    global $Matchmaking_Slave_DB_Name;
    $MatchDB;

    if(DB_Connect($MatchDB, $Matchmaking_Slave_DB_IP[$Index], $Matchmaking_Slave_DB_ID[$Index], 
        $Matchmaking_Slave_DB_Password[$Index], $Matchmaking_Slave_DB_Name[$Index], $Matchmaking_Slave_DB_PORT[$Index]) === false)
    {
        // 연결에 실패하면, 실패 응답 보내고 exit (DB Data 연결 에러)
        global $cnf_DB_DATA_CONNECT_ERROR;
        OnError($cnf_DB_DATA_CONNECT_ERROR);  
    }

    // 3. 연결이 잘 됐으면, matchmaking_ststus.server에서 heartbeat가 일정 이상인 서버를 제외시키고, 가장 최근의 서버 하나만 알아온다.
    $CompTime = '00:07:00'; // 최근 7분 내.
    $Result = DB_Query("SELECT * FROM `matchmaking_status`.`server` WHERE TIMEDIFF(NOW(), `heartbeat`) < '$CompTime' ORDER BY `heartbeat` DESC LIMIT 1", $MatchDB, $accountno);


    // MatchMakingDB 연결 해제
    DB_Disconnect($MatchDB);

    // 쿼리 실패했을 경우
    if($Result === false)
    {
        // 실패 응답 보내고 exit (DB 쿼리 에러)
        global $cnf_DB_QUERY_ERROR;
        OnError($cnf_DB_QUERY_ERROR);
    }

    // 4. 성공했으면 결과 빼옴
    $Data = mysqli_fetch_Assoc($Result);

    // 만약, 데이터가 null이라면, 접속할 서버가 하나도 없는것.
    if($Data == null)
    {
         // 실패 응답 보내고 exit (접속 가능한 서버 없음)
         global $cnf_LOBBY_MATCHMAKING_SERVER_NOT_FIND;
         OnError($cnf_LOBBY_MATCHMAKING_SERVER_NOT_FIND);
    }

    // 5. 결과까지 있으면 진짜 접속할 서버 결정.
    mysqli_free_Result($Result);  
    return $Data;
}

?>