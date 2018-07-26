<?php
// -------------------
// 로그인
// -------------------
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



// 2. ID가 존재하고, password가 정확한 유저를 찾아서 가져옴.
$Query = "SELECT * FROM `accounttbl` WHERE `id` = '{$Content_Body['id']}' AND `password` = '{$Content_Body['pass']}'";
$result = DB_Query($Query);

$LoginUser = mysqli_fetch_Assoc($result);

// 리소스 해제
mysqli_free_result($result);



// 3. 로그인 여부 확인
// id,password가 일치하는 유저가 없으면

if($LoginUser === NULL)
{
    // 리턴 코드를 9003으로 만든다.
    $ReturnCode = 9003;
    $resultMsg = "id OR Pass Discordance";
    $SessionKey = "";
}


// 일치하는 유저가 있으면
else
{
    // 로그인 성공한 것. 리턴코드 1로 만든다.
    $ReturnCode = 1;

    // 메시지도 성공으로 만듬
    $resultMsg = 'SUCCESS';

    // 세션키 만든다.
    $SessionKey = Create_SessionKey();

    // SessionTBL과 LoginTBl 갱신. 이미 만들어뒀으니 갱신만 시키면 된다.
    // 쿼리를 만들어만 둔다.
    $accountno = $LoginUser['accountno'];
    $time = date("Y-m-d H:i:s");
    $timestamp = time();
    $Array = array();

    array_push($Array, "UPDATE `sessiontbl` SET `SessionKey` = '$SessionKey', `StartTime` = $timestamp WHERE `accountno` = $accountno");
    array_push($Array, "UPDATE `logintbl` SET `time` = '$time', `ip` = '{$_SERVER['REMOTE_ADDR']}', `count` = count+1 WHERE `accountno` = $accountno");

    // 트랜잭션 걸고 쿼리 날림
    DB_TransactionQuery($Array);
}


// 4. 결과 돌려주기
$Response['resultCode'] = $ReturnCode;
$Response['resultMsg'] = $resultMsg; 
$Response['accountNo'] = $LoginUser['accountno'];
$Response['sessionKey'] = "$SessionKey";

echo json_encode($Response);



// ---------------------------------------
// cleanup 체크.
// 이 안에서는 [DB 연결 해제, 프로파일러 보내기, 게임로그 보내기]를 한다.
require_once('/../LIBRARY/_Clenup.php');

// --------------------------------------

?>