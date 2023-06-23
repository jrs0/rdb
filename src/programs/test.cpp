#define OTL_ODBC_MSSQL_2008  // Compile OTL 4/ODBC, MS SQL 2008
#define OTL_STL              // Turn on STL features
#define OTL_ANSI_CPP         // Turn on ANSI C++ typecasts

#if (defined(__clang__) || defined(__GNUC__)) && \
    (defined(OTL_CPP_14_ON) || defined(OTL_CPP_17_ON))
#include <experimental/string_view>
#define OTL_STD_STRING_VIEW_CLASS std::experimental::string_view
#define OTL_STD_STRING_VIEW_CLASS std::experimental::string_view
#elif defined(_MSC_VER) && (_MSC_VER >= 1910) && defined(OTL_CPP_17_ON)
// VC++ 2017 or higher when /std=c++latest is used
#include <string_view>
#define OTL_STD_STRING_VIEW_CLASS std::string_view
#endif

#include "otlv4.h"

void select(otl_connect& db) {
    std::string nhs_number;

    otl_stream i(1, "select top 10 aimtc_pseudo_nhs from abi.dbo.vw_apc_sem_001", db);

    while (!i.eof()) {  // while not end-of-data
        i >> nhs_number;
        std::cout << nhs_number << std::endl;
    }
}

int main() {
    try {
        otl_connect::otl_initialize();

        otl_connect db;
        db.rlogon("DSN=xsw");

        select(db);

        db.logoff();
        std::cout << "Got to the end" << std::endl;
    } catch (otl_exception& p) {
        std::cerr << p.msg << std::endl;
        std::cerr << p.stm_text << std::endl;
        std::cerr << p.var_info << std::endl;
    }
}