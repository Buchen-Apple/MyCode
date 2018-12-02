<?php
require($_SERVER['DOCUMENT_ROOT']. "/LIBRARY/_StartUp.php");
require($_SERVER['DOCUMENT_ROOT']. "/LIBRARY/_Content_Library.php");

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
$Data = SearchUser($DataKey , $Content_Body[$DataKey]);

// 4. 접속할 shDB_Data 알아온 후 Conenct
$DBInfo = shDB_Data_ConnectInfo($Data['dbno'], $Data['accountno']);
$shDB_Data = shDB_Data_Conenct($DBInfo);

// 5. accountTBL에서 정보 Select.
$SelectData = shDB_Data_Select($Data['accountno'], $shDB_Data, $DBInfo['dbname'], 'contents');

// 6. Disconenct
DB_Disconnect($shDB_Data);

// 7. 돌려줄 결과 셋팅 (여기까지 오면 정상적인 결과)
$Response['result'] = $cnf_COMPLETE;
$Response['accountno'] = intval($SelectData['accountno']);
$Response['playtime'] = intval($SelectData['playtime']);
$Response['playcount'] = intval($SelectData['playcount']);
$Response['kill'] = intval($SelectData['kill']);
$Response['die'] = intval($SelectData['die']);
$Response['win'] = intval($SelectData['win']);

// 8. 결과 돌려주기
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_Clenup.php");
ResponseJSON($Response, $Data['accountno']);
?>