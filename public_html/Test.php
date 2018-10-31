<?php
$Test = "에이비씨"; 

//mb_convert_encoding($Test, "UTF-8", "EUC-KR");
iconv("UTF-8", "EUC-KR", $Test);

echo mb_detect_encoding($Test,"ASCII, EUC-KR, UTF-8", true);  

/*
// 파일로 남긴다.
$myfile = fopen("MYErrorfileERROR_Handler.txt", "w") or die("Unable to open file!");
$txt = "ERROR_Handler --> $Temp1 . $Temp2 . $Test \n";
fwrite($myfile, $txt);
fclose($myfile);
*/

//phpinfo();

?>