<meta http-equiv="refresh" content="60">

<?php
// --------------------
// OTP 형태로 키 생성
// --------------------

// ----------- DB와 연결 파트
$IP = "127.0.0.1";
$ID = 'root';
$Password = '034689';
$DBName = 'test_02';


// DB 커넥트
$DB = mysqli_connect($IP, $ID, $Password, $DBName);

if(!$DB)
{
    echo "Error: Unable to connect to MySQL.";
    echo "<br>Debugging errno: " . mysqli_connect_errno($DB);
    echo "<br>Debugging error: " . mysqli_connect_error($DB);
    exit;
}

// ------------------ 세션 만들기 파트
// 타임스템프 시간과, 임의의 값 무언가를 구한다.
// 총 3개. 1분 전 / 현재 / 1분 후

// 1. 유저의 ID 길이를 알아온다. 일단 무조건 ddd2026 이다.
$UserID = 'ddd2026';
$len = strlen($UserID);

// 2. 1글자씩 분리한다.
// $Text = array();
// for($i = 0; $i < $len; ++$i)
// {
//     $Text[$i] = substr($UserID, 0, 1);
//     $UserID = substr($UserID, 1);
// }

$Text = array();
for($i = 0; $i < $len; ++$i)
    $Text[$i] = $UserID[$i];

// 3. 분리한 글자를 Key로, otpTBL테이블에서 매칭되는 Value를 찾아서 XORKey 배열에 저장
$imprtKey1 = array();
$imprtKey2 = array();
$imprtKey3 = array();
$XORKey = array();
for($i = 0; $i < $len; $i++)
{
    $Select_Query = "SELECT `Value` FROM otpTBL WHERE `Key` = '$Text[$i]'";
    $retval = mysqli_query($DB, $Select_Query);
    if($retval == false)
    {
        echo 'Select Query Fail...<br>';
        echo mysqli_errno($DB) . '<br>';
        echo mysqli_error($DB) . '<br>';
        mysqli_close($D);
        exit;
    }

    $fetchValue = mysqli_fetch_array($retval, MYSQLI_BOTH);   
    $XORKey[$i] = $fetchValue['Value'];

     // 쿼리의 결과 리소스 삭제
     mysqli_free_result($retval);
}

// ------------------ 세션 출력 파트
 $timestamp = time();
    
 $ModValue = 10148;
 for($i = 0; $i < $len; ++$i)
 {
     // 알아온 테이블의 Value와, 현재 시간 30148으로 모듈러 해서 나머지 값으로 문자로 해서 XOR 한다
     $imprtKey1[$i] = $XORKey[$i] ^ (string)(($timestamp-60) % $ModValue);
     $imprtKey2[$i] = $XORKey[$i] ^ (string)($timestamp % $ModValue);
     $imprtKey3[$i] = $XORKey[$i] ^ (string)(($timestamp+60) % $ModValue);    
 }

 // 4. imprtKey 안의 배열을 1개로 합친다.
 $SessionKey1 = implode($imprtKey1);
 $SessionKey2 = implode($imprtKey2);
 $SessionKey3 = implode($imprtKey3);

 echo date("Y-m-d H:i:s") . ' (1분 단위로 갱신)<br>';
 echo "세션 키 이전 : $SessionKey1 <br>";
 echo "세션 키 현재 : $SessionKey2 <br>";
 echo "세션 키 이후 : $SessionKey3 <br><br>";

// -------------------- 해제 파트
// 연결된 DB와 연결 해제
mysqli_close($DB);

?>