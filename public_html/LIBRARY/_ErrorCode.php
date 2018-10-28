<?php

// ****************************
// 컨텐츠 파트의 Error Code
// ****************************


// ------------------------
// 공통
// ------------------------
// 성공
$cnf_COMPLETE = 1;

// Request 파라미터 에러
$cnf_ERROR_PARAMETER = -100;

// shDB API 서버 HTTP 에러(shDB API HTTP CODE가 200이 아닌 경우)
$cnf_SHDB_API_HTTP_ERROR    =   -110;

// shDB API 서버 기타 에러
$cnf_SHDB_API_ERROR    =   -111;


// ------------------------
// get_matchmaking.php에서 사용
// ------------------------
// 재시도 요망(shdb에서 -11 에러난 경우)
$cnf_LOBBY_RETRY    =   -1;

// 가입정보 없음
$cnf_LOBBY_NOT_FIND_JOIN_INFO   =   -2;

// 인증 오류(토큰키 오류)
$cnf_LOBBY_TOKEN_ERROR  =   -3;

// 접속 가능한 서버 없음 (메치메이킹 서버가 없음)
$cnf_LOBBY_MATCHMAKING_SERVER_NOT_FIND  =   -4;


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
$cnf_CONTENT_ERROR_NOT_FIND_ACCOUNTNO_ACCOUNTTBL  =   -11;

// Index.allocate에는 있으나, data.contents에 없음. 롤백 후 해당 에러 리턴
$cnf_CONTENT_ERROR_NOT_FIND_ACCOUNTNO_CONTENTSTBL  =   -12;



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
$cnf_DB_NOT_FIND_COLUMN    =   -61;

// Update/Select 테이블 오류 (존재하지 않는 테이블)
$cnf_DB_NOT_FOUNT_TABLE =   -62;






?>