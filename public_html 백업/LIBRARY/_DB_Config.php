<?php

// StartUP.php에서 사용하는 정보
// 시스템의 버전 정보 알아오기 등의 테이블이 있는 MYSQL 서버
$StartUP_DB_IP = '127.0.0.1';
$StartUP_DB_ID = 'root';
$StartUP_DB_Password = '034689';
$StartUP_DB_PORT = 3306;
$StartUP_DB_Name = 'Session_Test';



// 로그 저장 장소.
// 프로파일링, 시스템 로그, 게임 로그
$Log_DB_IP = '127.0.0.1';
$Log_DB_ID = 'root';
$Log_DB_Password = '034689';
$Log_DB_PORT = 3306;
$Log_DB_Name = 'log_db';



// shDB_Info의 Slave가 있는 주소
// Slave만 추가되면 코드 수정없이(설정파일만 수정) 그 Slave에도 갈 수 있어야 한다.
$Info_SlaveCount = 2;

$Info_Slave_DB_IP[0] = '127.0.0.1';
$Info_Slave_DB_ID[0] = 'root';
$Info_Slave_DB_Password[0] = '034689';
$Info_Slave_DB_PORT[0] = 3306;
$Info_Slave_DB_Name[0] = 'shdb_info';

$Info_Slave_DB_IP[1] = '127.0.0.1';
$Info_Slave_DB_ID[1] = 'root';
$Info_Slave_DB_Password[1] = '034689';
$Info_Slave_DB_PORT[1] = 3306;
$Info_Slave_DB_Name[1] = 'shdb_info';



// shDB_Index의 Master 주소
$Index_Master_DB_IP = '127.0.0.1';
$Index_Master_DB_ID = 'root';
$Index_Master_DB_Password = '034689';
$Index_Master_DB_PORT = 3306;
$Index_Master_DB_Name = 'shdb_index';


// shDB_Index의 Slave가 있는 주소
// Slave만 추가되면 코드 수정없이(설정파일만 수정) 그 Slave에도 갈 수 있어야 한다.
$Index_SlaveCount = 2;

$Index_Slave_DB_IP[0] = '127.0.0.1';
$Index_Slave_DB_ID[0] = 'root';
$Index_Slave_DB_Password[0] = '034689';
$Index_Slave_DB_PORT[0] = 3306;
$Index_Slave_DB_Name[0] = 'shdb_index';

$Index_Slave_DB_IP[1] = '127.0.0.1';
$Index_Slave_DB_ID[1] = 'root';
$Index_Slave_DB_Password[1] = '034689';
$Index_Slave_DB_PORT[1] = 3306;
$Index_Slave_DB_Name[1] = 'shdb_index';



?>