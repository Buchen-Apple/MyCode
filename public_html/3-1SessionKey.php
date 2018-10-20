<?php
// -----------------------
// 세션키 재발급
// -----------------------

require '_DB_Library.php';
require '_SessionKey_Library.php';

// DB 이름 변경
$DB_Name = "session_test";

// 1. DB와 접속
DB_Connect();

// 2. accountno랑 세션키 입력받음. 무조건 다 입력했어야 함.
if(isset($_GET['no']) === false || isset($_GET['key']) === false)
{
    echo 'parameter err <br>';

    // DB와 연결 해제
    DB_Disconnect();
    exit;
}

// 3. accountno, 세션키 가져옴
$accountno    = mysqli_real_escape_string($g_DB, $_GET['no']);
$SessionKey   = mysqli_real_escape_string($g_DB, $_GET['key']);

// 4. sessiontbl에서 클라에서 입력받은 accountno와 SessionKey가 완전히 일치하는 유저를 찾아온다.
$Query = "SELECT * FROM `sessiontbl` WHERE `accountno` = $accountno AND `SessionKey` = '$SessionKey'";
$result = DB_Query($Query);

$user = mysqli_fetch_Assoc($result);
mysqli_free_result($result);

// 5. 상황에 따라 쿼리 날림여부 체크
// 없는 accountNo라면, returncode를 0으로 넣음.
if($user === NULL)
{
    $ReturnCode = 0;
    $SessionKey = "";
}

// 그게 아니라면, 있는것이니 로직 처리
else
{
    // 리턴코드를 1로 넣음
    $ReturnCode = 1;

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
    $Query = "UPDATE `sessiontbl` SET `SessionKey` = '$SessionKey', `StartTime` = $timestamp WHERE `accountno` = $accountno";
    DB_Query($Query);
}


// 6. DB와 연결 해제
DB_Disconnect();

// 7. 결과 출력하기
echo '<br>ResultCode : ' . $ReturnCode . '/ AccountNo : ' . $user['accountno'] . '/ SessionKey : ' . $SessionKey . '<br>';

?>