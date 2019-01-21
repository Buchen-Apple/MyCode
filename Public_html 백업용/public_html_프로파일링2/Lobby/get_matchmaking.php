<?php
// ******************************
// MatchMaking 서버(스테이트 풀)를 알아와서 돌려준다.
// ******************************

// ---------------------------------------
// startUp 체크.
// 이 안에서는 [ 프로파일러 생성, 게임로그 생성]를 한다.
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_StartUp.php");
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_Lobby_Library.php");
// ---------------------------------------

// 1. 클라이언트에서 받은 RAW 데이터를 \r\n으로 분리해서 받음
$Body = explode("\r\n", file_get_contents('php://input'));


// 2. 컨텐츠 부분 decoding
// 컨텐츠쪽 파라미터가 안왔을 경우, 실패패킷 보냄
if(isset($Body[0]) === false)
{
     // 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
     global $cnf_ERROR_PARAMETER;
     OnError($cnf_ERROR_PARAMETER);  
}

$Content_Body = json_decode($Body[0], true);

// 3. Parameter 체크
// accountno가 왔는지 체크
if(isset($Content_Body['accountno']) === false)
{
    // 안왔으면 email이 왔는지 체크
    if(isset($Content_Body['email']) === true)
    {
        // 이메일이 왔다면, 이메일 형태가 맞는지 확인
        if(filter_var(current($Content_Body), FILTER_VALIDATE_EMAIL) === false)
        {
            // 형태가 다르면 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
            global $cnf_ERROR_PARAMETER;
            OnError($cnf_ERROR_PARAMETER);  
        }
    }

    // 이메일 마저도 안왔으면 파라미터 에러 리턴.
    else
    {
        // 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
        global $cnf_ERROR_PARAMETER;
        OnError($cnf_ERROR_PARAMETER);  
    }

    // 이메일이 잘 왔으면 DataKey를 email로 설정.
    $DataKey = 'email';
}
else
{
    $DataKey = 'accountno';
}

// sessionKey가 왔는지 체크
if(isset($Content_Body['sessionkey']) === false)
{
    // 안 왔으면 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
    global $cnf_ERROR_PARAMETER;
    OnError($cnf_ERROR_PARAMETER);  
}



// 4. sbdb/Select.account.php에서 유저 정보 얻어오기
// Reqeust 데이터 셋팅
$Request = array();
$Request[$DataKey] = $Content_Body[$DataKey];


// Select.account.php로 Request
$Response = shDBAPI_GetUserData($Request);

// 5. 세션키를 검사한다.
if($Response['sessionkey'] != $Content_Body['sessionkey'])
{
     // 실패 패킷 전송 후 php 종료하기 (인증 오류)
     global $cnf_LOBBY_TOKEN_ERROR;
     OnError($cnf_LOBBY_TOKEN_ERROR);  
}

// 6. 메치메이킹 DB에서, 이번에 접속할 메치메이킹 서버를 알아온다.
$ServerInfo = GetMatchMakingServer($Response['accountno']);



// 7. 정보를 돌려준다.
// 해당 함수는 [인코딩, 로깅, 돌려줌] 까지 한다
$ClientResponse['result'] = $cnf_COMPLETE;
$ClientResponse['serverno'] = intval($ServerInfo['serverno']);
$ClientResponse['ip'] = $ServerInfo['ip'];
$ClientResponse['port'] = intval($ServerInfo['port']);

ResponseJSON($ClientResponse, $Response['accountno']);


// ---------------------------------------
// cleanup 체크.
// 이 안에서는 [DB 연결 해제, 프로파일러 보내기, 게임로그 보내기]를 한다.
// 이 안에서 프로파일링 저장 시 accountno를 쓰기때문에, 그 전용으로 만든다. accountno를 모르는 경우에는 이 변수자체를 안만든다.
$ClearAccountNo = $Response['accountno'];
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_Clenup.php");
// --------------------------------------

?>