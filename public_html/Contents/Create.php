<?php

// ******************************
// ID를 신규 할당한다.
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

// 파라미터가 이메일이 맞는지 체크
if(filter_var($Content_Body['email'], FILTER_VALIDATE_EMAIL) === false)
{
    // 실패 패킷 전송 후 php 종료하기 (Parameter 에러)
    global $cnf_CONTENT_ERROR_PARAMETER;
    OnError($cnf_CONTENT_ERROR_PARAMETER);  
}

// 이메일이 맞다면, escape로 안전하게 가져옴.
$shDB_Index = CshDB_Index_Contents::getInstance();
$email = mysqli_real_escape_string($shDB_Index->DB_ConnectObject(), $Content_Body['email']);


// 2. shDB_Info.available 테이블에서 가장 여유분이 많은 dbno를 알아온다.
$dbData = GetAvailableDBno();



// 3. shDB_Index.allocate 테이블에 Insert. 
// email, dbno를 날려서 새로운 유저 등록.
// 실패하면, 내부에서 알아서 실패패킷까지 보낸다.
// 성공 시, 해당 유저에게 할당된 accountNo 리턴
$AccountNo = UserInsert($email, $dbData['dbno']);



// 4. dbno를 이용해, DB 검색 후, account테이블과 contents 테이블에 정보 갱신
DataSet($dbData['dbno'], $dbData['available'], $email, $AccountNo);



// 5. 돌려줄 결과 셋팅 (여기까지 오면 정상적인 결과)
$Response['result'] = $cnf_CONTENT_COMPLETE;
$Response['accountno'] = $AccountNo; 
$Response['email'] = $email;
$Response['dbno'] = $dbData['dbno'];


// 6. 결과 돌려주기
// 해당 함수는 [인코딩, 로깅, 돌려줌] 까지 한다
ResponseJSON($Response, $AccountNo);


// ---------------------------------------
// cleanup 체크.
// 이 안에서는 [DB 연결 해제, 프로파일러 보내기, 게임로그 보내기]를 한다.
require_once('/../LIBRARY/_Clenup.php');
// --------------------------------------

echo 'LastOk'; // <<< 테스트용
?>