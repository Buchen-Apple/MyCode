<?php
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_Content_Library.php");

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

// 파라미터가 이메일이 맞는지 체크 (무조건 이메일이 와야한다.)
if(filter_var($Content_Body['email'], FILTER_VALIDATE_EMAIL) === false)
{
    // 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
    global $cnf_ERROR_PARAMETER;
    OnError($cnf_ERROR_PARAMETER);  
}

// 3. shDB_Info.available 테이블에서 가장 여유분이 많은 dbno와 available을 알아온다.
$dbData = GetAvailableDBno();

// 4. shDB_Index.allocate 테이블에 Insert. 
$safeEmail;
$AccountNo = UserInsert($Content_Body['email'], $dbData['dbno'], $safeEmail);

// 5. dbno를 이용해, shDB_Info.dbconnect에서 db의 정보를 가져온다.
$dbInfoData = shDB_Data_ConnectInfo($dbData['dbno'], $AccountNo);

// 6. shDB_Data에 Connect
$shDB_Data = shDB_Data_Conenct($dbInfoData);

// 7. 인자로 던진 shDB_Data의 account테이블과 contents 테이블에 정보 갱신
shDB_Data_CreateInsert($shDB_Data, $dbInfoData['dbname'], $safeEmail, $AccountNo);

// 8. 연결했던 shDB_Data에 Disconenct
DB_Disconnect($shDB_Data);

// 9. 이번에 유저를 접속시킨 dbno의 available을 1 감소.  
MinusAvailable($dbData['dbno']);

// 10. 돌려줄 결과 셋팅 (여기까지 오면 정상적인 결과)
$Response['result'] = $cnf_COMPLETE;
$Response['accountno'] = intval($AccountNo); 
$Response['email'] = $safeEmail;
$Response['dbno'] = intval($dbData['dbno']);

// 11. 결과 돌려주기
ResponseJSON($Response, $AccountNo);

$ClearAccountNo = $AccountNo;
require_once($_SERVER['DOCUMENT_ROOT'] . "/LIBRARY/_Clenup.php");
?>