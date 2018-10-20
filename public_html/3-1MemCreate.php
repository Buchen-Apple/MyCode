<?php
// -------------------
// 신규 회원가입
// -------------------

require '_DB_Library.php';


// DB 이름 변경
$DB_Name = "session_test";

// 1. DB와 접속
DB_Connect();

// 2. id, password, nickname 입력받음. 무조건 다 입력했어야 함.
if(isset($_GET['id']) === false || isset($_GET['password']) === false || isset($_GET['nickname']) === false)
{
    echo 'parameter err <br>';

    // DB와 연결 해제
    DB_Disconnect();
    exit;
}

// 3. id, password, nickname 가져옴
$id         = mysqli_real_escape_string($g_DB, $_GET['id']);
$password   = mysqli_real_escape_string($g_DB, $_GET['password']);
$nickname   = mysqli_real_escape_string($g_DB, $_GET['nickname']);

//4.  accounttbl에서 id 중복체크
$Query = "SELECT 0 FROM `accounttbl` WHERE `id` = '$id'";
$result = DB_Query($Query);

// 데이터를 빼왔는데, 무언가가 있다면 중복된 데이터가 있다는 뜻.
if(mysqli_num_rows($result))
{     
    echo '중복 ID <br>';

    // DB와 연결 해제
    DB_Disconnect();
    exit;
}

// 리소스 해제
mysqli_free_result($result);

// 5. accounttbl에서 nickname 중복체크
$Query = "SELECT 0 FROM `accounttbl` WHERE `nickname` = '$nickname'";
$result = DB_Query($Query);

// 데이터를 빼왔는데, 무언가가 있다면 중복된 데이터가 있다는 뜻.
if(mysqli_num_rows($result))
{
    echo '중복 닉네임 <br>';

    // DB와 연결 해제
    DB_Disconnect();
    exit;
}

// 리소스 해제
mysqli_free_result($result);

// 6. 이번 유저의 accountno를 알아온다.
$Query = "SELECT * FROM `accountchecktbl`";
$result = DB_Query($Query);

$accountID = mysqli_fetch_row($result);

// 리소스 해제
mysqli_free_result($result);

// 7. accountTBL,  SessionTBL, ScoreTBL, loginTBL에 insert 하기
// id와 nickname에 유니크가 걸려있어서 여기서도 중복 걸러내긴 한다.
// SessionTBL과 ScoreTBL, loginTBl에는 0으로 데이터 추가.
$Query1 = "INSERT INTO `accounttbl` (`accountno`, `id`, `password`, `nickname`) VALUES ($accountID[0], '$id', '$password', '$nickname')";

$Query2 = "INSERT INTO `sessiontbl` (`accountno`, `SessionKey`, `StartTime`) VALUES($accountID[0], '0', 0)";
$Query3 = "INSERT INTO `scoretbl` (`accountno`, `score`) VALUES($accountID[0], 0)";
$Query4 = "INSERT INTO `loginTBl` (`accountno`, `time`, `ip`, `count`) VALUES($accountID[0], '0',  '0', 0)";

$queryArray = array($Query1, $Query2, $Query3, $Query4);

// 트랜잭션 걸고 한다.
DB_TransactionQuery($queryArray);

// 8. `accountchecktbl`의 값을 1 증가시킨다.
$accountID[0]++;
$Query = "UPDATE `accountchecktbl` SET `NowAccount` = $accountID[0]";
DB_Query($Query);


// 9. 최종 완료 메시지 보여줌
$accountID[0]--;
echo "<br><br>Success!! AccountID = $accountID[0] <br>";


// DB와 연결 해제
DB_Disconnect();


?>