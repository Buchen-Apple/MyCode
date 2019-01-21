<?php
// 각종 인클루드
require('_Error_Handling_LIbrary.php');
require('_DB_Library.php');
require('_DB_Config.php');
require('_ErrorCode.php');

// ResponseJSON() 함수 원형
function ResponseJSON($Response, $accountNo)
{    
    // 인코딩 결과 전송
    echo json_encode($Response);
}

// 에러 발생 시, 실패패킷 전송 후 exit하는 함수
function OnError($result, $AccountNo = -1)
{
    $Response['result'] = $result;  

    // 실패 패킷 전송
    ResponseJSON($Response, $AccountNo);
    exit;
}
?>