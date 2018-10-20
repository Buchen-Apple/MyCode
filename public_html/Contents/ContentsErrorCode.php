<?php

// ****************************
// 컨텐츠 파트의 Error Code
// ****************************


// ------------------------
// 공통
// ------------------------
// 성공
$cnf_CONTENT_COMPLETE = 1;

// Request 파라미터 에러
$cnf_CONTENT_ERROR_PARAMETER = -100;



// ------------------------
// Create.php에서 사용
// ------------------------
// 이미 가입된 이메일
$cnf_CONTENT_ERROR_CREATE_EMAIL_DUPLICATE = -1;

// DB Index 할당 에러
$cnf_CONTENT_ERROR_CREATE_DB_INDEX_ALLOCATE = -2;

// DB Data 삽입 에러
$cnf_CONTENT_ERROR_CREATE_DB_DATA_INSERT = -3;




// ------------------------
// Select_Account.php에서 사용
// ------------------------
// 회원가입 자체가 안된 상태
$cnf_CONTENT_ERROR_SELECT_NOT_CREATE_USER = -10;

// Index.allocate에는 있으나, data.account에 없음. 롤백 후 해당 에러 리턴
$cnf_CONTENT_ERROR_NOT_FOUND_ACCOUNTNO  =   -11;



// ------------------------
// DataBase 관련 에러
// ------------------------
// DBIndex Master 연결 에러
$cnf_DB_INDEX_MASETER_CONNECT_ERROR   =   -50;

// DBIndex Slave 연결 에러
$cnf_DB_INDEX_SLAVE_CONNECT_ERROR   =   -51;

// DBData 연결 에러
$cnf_DB_DATA_CONNECT_ERROR   =   -52;

// DB 쿼리 에러
$cnf_DB_QUERY_ERROR   =   -60;

// Update 컬럼 오류 (존재하지 않는 컬럼)
$cnf_DB_NOT_FOUND_COLUMN    =   -61;



?>