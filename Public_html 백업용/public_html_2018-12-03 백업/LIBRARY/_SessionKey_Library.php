<?php
// -------------------------
// 세션키 만드는 라이브러리
// -------------------------

function Create_SessionKey()
{
    // 총 31개의 키 (인덱스 0~30)
    $keyArray = array('a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G',
                      '1', '2', '3', '4', '5', '6', '{', '~', 'H', '?', '|', '[', ']', '*',
                      'j', '_', ';');

    // 현재, $key에는 31이 들어간다.
    $key = count($keyArray);

    // rand()를 시드키로 한번 만들어낸 후, 다시 rand()와 $Key를 mod연산 한다.
    // 총 62개의 키 만들어냄.
    srand(rand());

    $SessionKey = array();
    for($i = 0; $i < 62; ++$i)
    {
        $Count = rand() % $key;       
        $SessionKey[$i] = $keyArray[$Count];
        //echo $Count . '=' . $keyArray[$Count] . '<br>';
    }

    $returnKey = implode($SessionKey);

    return $returnKey;
}

?>