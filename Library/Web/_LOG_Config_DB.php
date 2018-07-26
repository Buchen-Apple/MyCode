<?php

/////////////////////////////////////////////////////////////////////
// 로그 서버 DB 정보 / SystemLog, GameLog, ProfilingLog 통합.
//
/////////////////////////////////////////////////////////////////////
$LOG_DB_IP = "127.0.0.1";
$LOG_DB_ID = 'root';
$LOG_DB_PORT = 3306;
$LOG_DB_Password = '034689';
$LOG_DB_Name = 'log_db';

$g_DB;


/*-----------------------------------

* SystemLog 테이블 템플릿,

CREATE TABLE `SystemLog_template` (
  `no`			int(11) NOT NULL AUTO_INCREMENT,
  `date`		datetime NOT NULL,
  `AccountNo`	varchar(50) NOT NULL,
  `Action`		varchar(128) DEFAULT NULL,
  `Message`		varchar(1024) DEFAULT NULL,

  PRIMARY KEY (`no`,`date`,`AccountNo`)
) ENGINE=MyISAM;

* GameLog 테이블 템플릿,
CREATE TABLE `GameLog_template` (
  `no`			bigint(20) NOT NULL AUTO_INCREMENT,
  `date`		datetime NOT NULL,
  `AccountNo`	bigint(20) NOT NULL,
  `LogType`		int(11) NOT NULL,
  `LogCode`		int(11) NOT NULL,
  `Param1`		int(11) DEFAULT '0',
  `Param2`		int(11) DEFAULT '0',
  `Param3`		int(11) DEFAULT '0',
  `Param4`		int(11) DEFAULT '0',
  `ParamString`	varchar(256) DEFAULT '',
  
  PRIMARY KEY (`no`, `AccountNo`)
) ENGINE=MyISAM;

*/

?>