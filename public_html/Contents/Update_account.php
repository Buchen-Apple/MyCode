<?php


// ******************************
// AccountTBL에 정보를 Update 한다.
// ******************************

// ---------------------------------------
// startUp 체크.
// 이 안에서는 [클라가 보낸 데이터 받기, DB 연결, 버전처리, 프로파일러 생성, 게임로그 생성]를 한다.
require_once('/../LIBRARY/_StartUp.php');
require_once('/../LIBRARY/_Content_Library.php');
// ---------------------------------------

// 1. 컨텐츠 부분 decoding
// 컨텐츠쪽 파라미터가 안왔을 경우, 실패패킷 보냄
if(isset($Body[3]) === false)
{
     // 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
     global $cnf_CONTENT_ERROR_PARAMETER;
     OnError($cnf_CONTENT_ERROR_PARAMETER);  
}

$Content_Body = json_decode($Body[3], true);

// 가장 상단에는 Accountno 혹은 email이와야한다.
// email이 왔다면, email 형태가 맞는지 체크한다.
$DataKey = key($Content_Body);
if($DataKey  == 'email')
{
    // 파라미터가 이메일이 맞는지 체크
    if(filter_var(current($Content_Body), FILTER_VALIDATE_EMAIL) === false)
    {
        // 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
        global $cnf_CONTENT_ERROR_PARAMETER;
        OnError($cnf_CONTENT_ERROR_PARAMETER);  
    }
}


// 2. 해당 유저가 저장되어있는 dbno와 해당 유저의 accountNo를 알아온다.
// Value가 Email일 수도 있기 때문에 AccountNo도 같이 리턴 받는다.
// Key, Value를 인자로 던진다.
$Data = SearchUser($DataKey , current($Content_Body));

// 배열을 다음칸으로 이동
// 만약, accountNo외에 파라미터가 하나도 없으면 파라미터 에러
if(next($Content_Body) === false)
{
    // 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
    global $cnf_CONTENT_ERROR_PARAMETER;
    OnError($cnf_CONTENT_ERROR_PARAMETER, $Data['accountno']);  
}


// 3. 접속할 shDB_Data 알아온 후 Conenct
$DBInfo = shDB_Data_ConnectInfo($Data);
$shDB_Data = shDB_Data_Conenct($DBInfo);


// 4. 해당 DB의 TBL에, 인자로 던진 AccountNo가 존재하는지 확인
// 이미 존재한다면 내부에서 롤백 후 실패 응답까지 보낸 후 exit
shDB_Data_AccountNoCheck($Data['accountno'], $shDB_Data, $DBInfo['dbname'], 'account');


// 5. 해당 DB의 TBL에, Update
shDB_Data_Update($Data['accountno'], $shDB_Data, $DBInfo['dbname'], 'account', $Content_Body);


// 6. Disconenct
DB_Disconnect($shDB_Data);


// 7. 돌려줄 결과 셋팅 (여기까지 오면 정상적인 결과)
$Response['result'] = $cnf_CONTENT_COMPLETE;


// 8. 결과 돌려주기
// 해당 함수는 [인코딩, 로깅, 돌려줌] 까지 한다
ResponseJSON($Response, $Data['accountno']);


// ---------------------------------------
// cleanup 체크.
// 이 안에서는 [DB 연결 해제, 프로파일러 보내기, 게임로그 보내기]를 한다.
require_once('/../LIBRARY/_Clenup.php');
// --------------------------------------

echo 'LastOk'; // <<< 테스트용  

?>