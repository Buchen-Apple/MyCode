<?php
// -------------------
// 신규 회원가입
// -------------------

// ---------------------------------------
// startUp 체크.
// 이 안에서는 [클라가 보낸 데이터 받기, DB 연결, 버전처리, 프로파일러 생성, 게임로그 생성]를 한다.
require_once('/../LIBRARY/_StartUp.php');

// --------------------------------------

// 1. 컨텐츠 부분 decoding
$Content_Body = json_decode($Body[3], true);

// 2. 컨텐츠용으로 DB 접속
$ContentDB;
if(DB_Connect($ContentDB, "127.0.0.1", 'root', '034689', 'Session_Test', 3306) == false)
{
    echo "Connect Error";
    exit;
}

// 3. accounttbl에서 id 중복체크
$IDQuery = "SELECT COUNT(*) FROM `accounttbl` WHERE `id` = '{$Content_Body['id']}'";
$IDResult = DB_Query($IDQuery, $ContentDB);

$Count = mysqli_fetch_Assoc($IDResult);

// 중복되는 id가 하나이상 있으면 중복이라고 알려줌
if($Count['COUNT(*)'] > 0)
{
    $Response['resultCode'] = 9001;
    $Response['resultMsg'] = 'ID DUPLICATION!!'; // 회원가입 중, 아이디 중복
    $Response['accountNo'] = $Body[2];

    // 해당 함수는 [인코딩, 로깅, 돌려줌] 까지 한다
    ResponseJSON($Response, $Response['accountNo']);
    exit;
}




// 4. accounttbl에서 nick 중복 체크
$NickQuery = "SELECT COUNT(*) FROM `accounttbl` WHERE `nickname` = '{$Content_Body['nick']}'";
$NickResult = DB_Query($NickQuery, $ContentDB);

$Count = mysqli_fetch_Assoc($NickResult);

// 중복되는 nock가 하나이상 있으면 중복이라고 알려줌
if($Count['COUNT(*)'] > 0)
{
    $Response['resultCode'] = 9002;
    $Response['resultMsg'] = 'NICKNAME DUPLICATION!!'; // 회원가입 중, 닉네임 중복
    $Response['accountNo'] = $Body[2];

    // 해당 함수는 [인코딩, 로깅, 돌려줌] 까지 한다
    ResponseJSON($Response, $Response['accountNo']);
    exit;
}




// 5. 이번 유저의 accountno를 알아온다.
$AccountNoQuery = "SELECT * FROM `accountchecktbl`";
$AccountNoResult = DB_Query($AccountNoQuery, $ContentDB);

$accountID = mysqli_fetch_row($AccountNoResult);




// 6. accountTBL,  SessionTBL, ScoreTBL, loginTBL에 insert 하기
// id와 nickname에 유니크가 걸려있어서 여기서도 중복 걸러내긴 한다. (id는 PK이기때문에 자동으로 중복불가능, nickname에는 유니크 걸어둠.)
// SessionTBL과 ScoreTBL, loginTBl에는 0으로 데이터 추가.
$Query1 = "INSERT INTO `accounttbl` (`accountno`, `id`, `password`, `nickname`) VALUES ($accountID[0], '{$Content_Body['id']}', '{$Content_Body['pass']}', '{$Content_Body['nick']}')";

$Query2 = "INSERT INTO `sessiontbl` (`accountno`, `SessionKey`, `StartTime`) VALUES($accountID[0], '0', 0)";
$Query3 = "INSERT INTO `scoretbl` (`accountno`, `score`) VALUES($accountID[0], 0)";
$Query4 = "INSERT INTO `loginTBl` (`accountno`, `time`, `ip`, `count`) VALUES($accountID[0], '0',  '0', 0)";

$queryArray = array($Query1, $Query2, $Query3, $Query4);

// 트랜잭션 걸고 한다.
DB_TransactionQuery($queryArray, $ContentDB);




// 7. `accountchecktbl`의 값을 1 증가시킨다.
$accountID[0]++;
$AccountAddQuery = "UPDATE `accountchecktbl` SET `NowAccount` = $accountID[0]";

$accountID[0]--;
DB_Query($AccountAddQuery, $ContentDB, $accountID[0]);



// 8. 돌려줄 결과 셋팅
$Response['resultCode'] = 1;
$Response['resultMsg'] = 'SUCCESS'; // 성공
$Response['accountNo'] = $accountID[0];



// 9. 리소스 반환
// 순서대로 3, 4, 5번에서 사용한 것.
// ID 중복 체크 리소스 해제
mysqli_free_result($IDResult);

// Nick 중복 체크 리소스 해제
mysqli_free_result($NickResult);

// AccountNo 알아오기 쿼리 리소스 해제
mysqli_free_result($AccountNoResult);


// ---------------------------------------
// cleanup 체크.
// 이 안에서는 [DB 연결 해제, 프로파일러 보내기, 게임로그 보내기]를 한다.
require_once('/../LIBRARY/_Clenup.php');

// --------------------------------------


// 10. 결과 돌려주기
// 해당 함수는 [인코딩, 로깅, 돌려줌] 까지 한다
ResponseJSON($Response, $accountID[0]);



?>