<?php

// 2-1. 컴마로 구분 된 여러개의 숫자를 $money 로 받음. (문자열로 받음)
// 컴마 기준으로 분리하여 배열로 저장.
// 모든 총 합을 구한 뒤 1000단위 컴마를 넣어서 출력.
function Test_01()
{   
    echo '1. 컴마로 구분 된 여러개의 숫자를 $money 로 받아서 배열로 저장후 총 값 1000단위 컴마로 출력<br><br>';
    $money = @$_GET['money'];

    // money가 null인지 확인
    if($money === null)
        echo '입력받은 money 없음<br>'; 

    // null이 아니면 무언가 입력받은것
    else
    {
        echo "입력받은 money 문자열 : $money<br>";    

        // 입력받은 문자열을 다 뽑아내어 $moneyArray에 배열형태로 저장
        $moneyArray = explode(',', $money);
        
        // sum에 배열 안의 값을 모두 더한다.
        $sum = 0;
        foreach($moneyArray as $Value)
            $sum += $Value;

        // 출력하기전에 1000단위로 콤마 찍는다.
        echo number_format($sum);     
        echo '<br>';
      
    }
}

// 2-2. check.php 뒤에 어떤 변수가 들어올지 모름.
// 그냥 뭐던지 상관 없이 EXIT 라는 문자가 있다면 
// "EXIT Find" 를 출력한다.
function Test_02()
{
    echo '<br><br>2. ~.php뒤에 EXIT라는 문자가 있다면 EXIT Find 출력하기. 소문자로 exit가 적혀있어도 걸러냄<br><br>';

    // 입력한 문자열 받아오기.
    $String = $_SERVER['QUERY_STRING'];

    // EXIT가 있으면 EXIT Find 출력하기
    if(stripos($String, 'EXIT') !== false)
        echo 'EXIT Find <br>';

    else
        echo 'EXIT 문자 없음 <br>';
}

// 3-3. URL 필터링 기능
// check_url.php
// 내부에 Filter 배열 생성.
// 배열에는 필터링 대상의 문자열을 넣음.
function Test_03()
{
    echo '<br><br>3. URL 필터링 기능. 대소문자 모두 검열<br><br>';   

    $Filter = array("REMOTE_ADDR", "CHAR(", "CHR(", "EVAL(");

    // 입력한 문자열 받아오기.
    $String = $_SERVER['QUERY_STRING'];

    $boolcheck = false;

     // 배열의 값 중 하나라도 있는지 체크
     foreach($Filter as $Value)
     {
        if(stripos($String, $Value) !== false)
        {
            $boolcheck = true;
            echo "($Value) BLOCK! <br>";   
        }      
     }

     if($boolcheck == false)
        echo 'BLOCK 하나도 없음 <br>';
}

// 2-1번 과제
Test_01();

// 2-2번 과제
Test_02();

// 2-3번 과제
Test_03();

?>