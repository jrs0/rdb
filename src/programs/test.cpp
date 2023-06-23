#define OTL_ODBC_MSSQL_2008 // Compile OTL 4/ODBC, MS SQL 2008
#include "otlv4.h"

int main() {

    otl_connect::otl_initialize();

    otl_connect db;
    db.rlogon("DSN=xsw");


    db.logoff();
    std::cout << "Got to the end" << std::endl;

}