<?php
// -----------------------
// 세션키 재발급
// -----------------------
// 해당 php에서 사용할 DB 이름 설정
$DB_Name =  "session_test";

// ---------------------------------------
// startUp 체크.
// 이 안에서는 [클라가 보낸 데이터 받기, DB 연결, 버전처리, 프로파일러 생성, 게임로그 생성]를 한다.
require_once('/../LIBRARY/_StartUp.php');
require_once('/../LIBRARY/_SessionKey_Library.php');
// --------------------------------------


// 1. 컨텐츠 부분 decoding
$Content_Body = json_decode($Body[3], true);




// 2. sessiontbl에서 클라에서 입력받은 accountno와 SessionKey가 완전히 일치하는 유저를 찾아온다.
$Query = "SELECT * FROM `sessiontbl` WHERE `accountno` = {$Content_Body['no']} AND `SessionKey` = '{$Content_Body['key']}'";
$result = DB_Query($Query);

$user = mysqli_fetch_Assoc($result);
mysqli_free_result($result);




// 3. 상황에 따라 쿼리 날림여부 체크
// 일치하는 유저가 없다면
if($user === NULL)
{
    // 리턴 코드를 9004으로 만든다.
    $ReturnCode = 9004;
    $resultMsg = "AccountNo OR SessionKey Not Found";
    $SessionKey = "";
}

// 그게 아니라면, 있는것이니 로직 처리
else
{
    // 리턴코드를 1로 넣음
    $ReturnCode = 1;

    // 메시지 입력
    $resultMsg = 'SUCCESS';

    // 세션키 유효시간 체크
    $timestamp = time();

    // 시간이 만료되었으면 세션키 발급 안해줌
    if($timestamp > ($user['StartTime'] + 300))
    {
        echo '세션키 유효기간 만료. <br>';

        // DB와 연결 해제
        DB_Disconnect();
        exit;
    }

    // 만료 안됐으면 다시 만든다
    $SessionKey = Create_SessionKey();

    // 만든 키를 테이블에 반영한다.
    $Query = "UPDATE `sessiontbl` SET `SessionKey` = '$SessionKey', `StartTime` = $timestamp WHERE `accountno` = {$Content_Body['no']}";
    DB_Query($Query);
}




// 4. 결과 돌려주기
$Response['resultCode'] = $ReturnCode;
$Response['resultMsg'] = $resultMsg; 
$Response['accountNo'] = $user['accountno'];
$Response['sessionKey'] = "$SessionKey";

echo json_encode($Response);




// ---------------------------------------
// cleanup 체크.
// 이 안에서는 [DB 연결 해제, 프로파일러 보내기, 게임로그 보내기]를 한다.
require_once('/../LIBRARY/_Clenup.php');

// --------------------------------------

?>