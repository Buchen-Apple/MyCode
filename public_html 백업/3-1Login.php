<?php
// -------------------
// 로그인
// -------------------

require '_DB_Library.php';
require '_SessionKey_Library.php';

// DB 이름 변경
$DB_Name = "session_test";

// 1. DB와 접속
DB_Connect();

// 2. id, password입력받음. 무조건 다 입력했어야 함.
if(isset($_GET['id']) === false || isset($_GET['password']) === false)
{
    echo 'parameter err <br>';

    // DB와 연결 해제
    DB_Disconnect();
    exit;
}

// 3. id, password 가져옴
$id         = mysqli_real_escape_string($g_DB, $_GET['id']);
$password   = mysqli_real_escape_string($g_DB, $_GET['password']);

// 4. ID가 존재하고, password가 정확한 유저를 찾아서 가져옴.
$Query = "SELECT * FROM `accounttbl` WHERE `id` = '$id' AND `password` = '$password'";
$result = DB_Query($Query);

$LoginUser = mysqli_fetch_Assoc($result);

// 리소스 해제
mysqli_free_result($result);

// 5. 로그인 여부 확인
// id,password가 일치하는 유저가 없으면

if($LoginUser === NULL)
{
    // 리턴 코드를 0으로 만든다.
    $ReturnCode = 0;
    $SessionKey = "";
}

// 일치하는 유저가 있으면
else
{
    // 로그인 성공한 것. 리턴코드 1로 만든다.
    $ReturnCode = 1;

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

// 6. DB와 연결 해제
DB_Disconnect();

// 7. 결과 출력하기
echo '<br>ResultCode : ' . $ReturnCode . '/ AccountNo : ' . $LoginUser['accountno'] . '/ SessionKey : ' . $SessionKey . '<br>';

?>