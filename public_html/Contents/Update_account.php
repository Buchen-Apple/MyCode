<?php
// ******************************
// shDB_Data.account에 정보를 Update 한다.
// ******************************

// ---------------------------------------
// startUp 체크.
// 이 안에서는 [프로파일러 생성, 게임로그 생성]를 한다.
$_SERVER = $GLOBALS["_SERVER"];
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_StartUp.php");
require_once($_SERVER['DOCUMENT_ROOT']. "/LIBRARY/_Content_Library.php");
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


// 3. 해당 유저가 저장되어있는 dbno와 해당 유저의 accountNo를 알아온다.
// Value가 Email일 수도 있기 때문에 AccountNo도 같이 리턴 받는다.
// Key, Value를 인자로 던진다.
$Data = SearchUser($DataKey , $Content_Body[$DataKey]);

// email 혹은 accountno 파라미터 삭제. 
unset($Content_Body[$DataKey]);


// 4. 접속할 shDB_Data 알아온 후 Conenct
$DBInfo = shDB_Data_ConnectInfo($Data['dbno'], $Data['accountno']);
$shDB_Data = shDB_Data_Conenct($DBInfo);


// 5. 해당 DB의 TBL에, 인자로 던진 AccountNo가 존재하는지 확인
// 이미 존재한다면 내부에서 롤백 후 실패 응답까지 보낸 후 exit
shDB_Data_AccountNoCheck($Data['accountno'], $shDB_Data, $DBInfo['dbname'], 'account');


// 6. 해당 DB의 TBL에, Update
shDB_Data_Update($Data['accountno'], $shDB_Data, $DBInfo['dbname'], 'account', $Content_Body);


// 7. Disconenct
DB_Disconnect($shDB_Data);

// 8. 돌려줄 결과 셋팅 (여기까지 오면 정상적인 결과)
$Response['result'] = $cnf_COMPLETE;


// 9. 결과 돌려주기
// 해당 함수는 [인코딩, 로깅, 돌려줌] 까지 한다
ResponseJSON($Response, $Data['accountno']);


// ---------------------------------------
// cleanup 체크.
// 이 안에서는 [DB 연결 해제, 프로파일러 보내기, 게임로그 보내기]를 한다.
// 이 안에서 프로파일링 저장 시 accountno를 쓰기때문에, 그 전용으로 만든다. accountno를 모르는 경우에는 이 변수자체를 안만든다.
$ClearAccountNo = $Data['accountno'];
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_Clenup.php");
// --------------------------------------
?>