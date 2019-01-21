<?php
require($_SERVER['DOCUMENT_ROOT']. "/LIBRARY/_StartUp.php");
require($_SERVER['DOCUMENT_ROOT']. "/LIBRARY/_Select_Library.php");

// 1. 클라이언트에서 받은 RAW 데이터 받음
$Body = file_get_contents('php://input');

// 2. 파라미터가 안왔을 경우, 실패패킷 보냄
if(isset($Body) === false)
{
     // 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
     global $cnf_ERROR_PARAMETER;
     OnError($cnf_ERROR_PARAMETER);  
}

$Content_Body = json_decode($Body, true);

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

// 4. shDB_Data에서 유저 정보 가져오기(Select)
$SelectData = shDB_Data_Select($Data['dbno'], $Data['accountno'], 'account');

// // 5. 돌려줄 결과 셋팅 (여기까지 오면 정상적인 결과)
$Response['result'] = $cnf_COMPLETE;

$Key = key($SelectData);
$Response["$Key"] = intval(current($SelectData));

while(next($SelectData) !== false)
{
    $Key = key($SelectData);
    $Value = current($SelectData);
    $Response["$Key"] = current($SelectData);
}

// 6. 결과 돌려주기
ResponseJSON($Response, $Data['accountno']);
?>