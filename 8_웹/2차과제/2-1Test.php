<?php
// ------------------------------
// DB에 접속하기 연습 및 과제
// ------------------------------

// IP, ID, password, DBName이 설정되어 있는 config를 require한다.
require('2-1Config.php');

// _GET 방식으로, 인자 넘겨받기.
$Name = $_GET['name'];
$Tel = $_GET['tel'];

// DB 커넥트
$DB = mysqli_connect($IP, $ID, $Password, $DBName);

if(!$DB)
{
    echo "Error: Unable to connect to MySQL.";
    echo "<br>Debugging errno: " . mysqli_connect_errno($DB);
    echo "<br>Debugging error: " . mysqli_connect_error($DB);
    exit;
}

// insert를 날릴지 말지 결정. 유저가 name과 tel 둘 다 입력해야 insert 쿼리 날림
if($Name === null || $Tel === null)
    echo '입력받은 name 혹은 tel이 없음. insert쿼리 안날림 <br><br>';

 else
 {
    // insert 쿼리 날리기
    $Insert_Query = "INSERT INTO testTBL(Name, Tel) VALUES('$Name', '$Tel')";
    $retval = mysqli_query($DB, $Insert_Query);
    if($retval == false)
    {
        echo 'Insert Query Fail...<br>';
        echo mysqli_errno($DB) . '<br>';
        echo mysqli_error($DB) . '<br>';
        mysqli_close($DB);
        exit;
    }
 }

// select 쿼리 날리기
$Select_Query = "SELECT * FROM testTBL";
$retval = mysqli_query($DB, $Select_Query);
if($retval === false)
{
    echo 'Select Query Fail...<br>';
    mysqli_close($D);
    exit;
}

// select로 받은 쿼리 분리시켜서 보여주기
// $row_data가 null이 될 때 까지 뺀다. 
// mysqli_fetch함수는, 인자로 받은 변수에 값이 없으면(?) null을 반환한다.
// 근데, 내부 안에 데이터가 null이면 내부 데이터는 null을 반환할 수도 있다. 이건, while문 조건이랑은 다른것.
// 예를들어, 아래 코드 중, Tel이 Not Null이라면, $row_data['Tel'] 안에 널이 들어올 수도 있다.
echo 'No / Name / Tel <br>';
while($row_data = mysqli_fetch_Assoc($retval))
    echo $row_data['No'] . ' / ' . $row_data['Name'] . ' / ' . $row_data['Tel'] . '<br>';

if($row_data === null)
    echo 'aaab <br>';

// 쿼리의 결과 리소스 삭제
mysqli_free_result($retval);

// 연결된 DB와 연결 해제
mysqli_close($DB);

?>