<?php

// 1-1. 서버, 클라이언트의 IP, Port 출력
function Test_01()
{       
    $clientaddr = $_SERVER['REMOTE_ADDR'];
    $clientport = $_SERVER['REMOTE_PORT'];

    $servertaddr = $_SERVER['SERVER_ADDR'];
    $serverport = $_SERVER['SERVER_PORT'];

    echo '1. 서버와 클라 IP/Port출력하기<br><br>';
    echo "서버 - [$servertaddr : $serverport]<br>";
    echo "클라이언트 - [$clientaddr : $clientport]";
}

// 1-2. URL GET 방식으로 구구단 단수 입력 받아서 출력하기.
function Test_02()
{        
    echo'<br><br>2. URL GET 방식으로 구구단 단수 입력 받아서 출력하기<br><br>';
    $num = @$_GET['num'];

    // num이 null인지 체크
    if($num === null)
        echo '입력받은 단수 없음<br>'; 

    // null이 아니라면, 숫자를 입력했는지 체크
    else if(is_numeric($num))
    {
        echo "입력받은 단수 : $num<br>";
        for($a=1; $a <10; ++$a)
        {
            $Value = $num*$a;
            echo "$num * $a = $Value <br>";    
        }    
    }

    else
        echo '구구단, 문자를 입력했습니다<br>';
}

// 1-3. URL GET 방식으로 문자열을 입력 받으면 이를 . 로 잘라서 표시하기.
function Test_03()
{            
    echo '<br><br>3. URL GET 방식으로 문자열을 입력 받으면 이를 . 로 잘라서 표시하기<br><br>';
    $str = @$_GET['str'];

    // str이 null인지 체크
    if($str === null)
        echo '입력받은 문자 없음<br>'; 

    // null이 아니라면 무언가 입력받은것.
    else
    {
        echo "입력받은 문자열 : $str<br>";    

        while(true)
        {
            // $str에서, '.'이 시작되는 위치를 0번째부터 알아오기. $pos에 저장됨.
            $pos = stripos($str, '.', 0);

            // 만약 $pos가 false면 마지막 .까지 찾은것이니, 마지막 문자를 출력하고 break;
            if($pos === false)
            {
                $ShowText = substr($str, 0);
                echo "$ShowText  <br>";  
                break;
            }           

            // $pos가 0이 아닐때만 아래 로직 진행.
            // $pos가 0이라는 것은, 가장 앞에(0번째 위치) '.'이 있다는 것이고 로직상 이 때는 아무것도 출력할게 없다.
            if($pos !==0)
            {
                 // $str에서 0번째부터 $pos번째까지 문자열을 $ShowText에 저장.
                $ShowText = substr($str, 0, $pos);
                echo "$ShowText <br>";  
            }

            // $str의, 맨 앞의 $pos+1부터 복사.
            // 가장 앞에는 .이 있으니 한칸 >>로 이동
            $str = substr($str, $pos+1);
        }
    } 
}


// 1-11번 과제
Test_01();

// 1-2번 과제
Test_02();

// 1-3번 과제
Test_03();


?>