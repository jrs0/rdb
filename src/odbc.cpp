#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdlib.h>
#include <mbstring.h>

#define MAX_DATA 100
#define MYSQLSUCCESS(rc) ((rc==SQL_SUCCESS)||(rc==SQL_SUCCESS_WITH_INFO))

class direxec
{
    RETCODE rc; ///< ODBC return code
    HENV henv; ///< Environment  
    HDBC hdbc; ///< Connection handle
    HSTMT hstmt; ///< Statement handle
    unsigned char szData[MAX_DATA]; ///< Returned data storage
    SQLLEN cbData; ///< Output length of data
    char chr_ds_name[SQL_MAX_DSN_LENGTH]; ///< Data source name

public:
    direxec();
    void sqlconn();     // Allocate env, stat, and conn
    void sqlexec(unsigned char *);  // Execute SQL statement
    void sqldisconn();  // Free pointers to env, stat, conn,
    // and disconnect
    void error_out();   // Displays errors
};

// [[Rcpp::export]]
int connect()
{
    // Declare an instance of the direxec object.
    direxec x;
  
    // Allocate handles, and connect.
    x.sqlconn();

    // Execute SQL command "SELECT first name, last_name FROM employee".
    x.sqlexec((UCHAR FAR *)"SELECT TOP 10 * from ABI.dbo.vw_APC_SEM_001");

    // Free handles, and disconnect.
    x.sqldisconn();
    
    // Return success code; example executed successfully.
    return (TRUE);
}


// Constructor initializes the string chr_ds_name with the
// data source name.
direxec::direxec()
{
    strcpy(chr_ds_name, "xsw");
}

// Allocate environment handle, allocate connection handle,
// connect to data source, and allocate statement handle.
void direxec::sqlconn(void)
{
    SQLAllocEnv(&henv);
    SQLAllocConnect(henv,&hdbc);
    rc=SQLConnect(hdbc, // handle
		  (SQLCHAR*)chr_ds_name, // server name (DSN)
		  SQL_NTS, // Length of server name
		  NULL,
		  0,
		  NULL,
		  0);
  
    // Deallocate handles, display error message, and exit.
    if (!MYSQLSUCCESS(rc))
	{
	    SQLFreeEnv(henv);
	    SQLFreeConnect(hdbc);
	    error_out();
	    exit(-1);
	}

    rc=SQLAllocStmt(hdbc,&hstmt);

}

// Execute SQL command with SQLExecDirect() ODBC API.
void direxec::sqlexec(unsigned char * cmdstr)
{
    rc=SQLExecDirect(hstmt,cmdstr,SQL_NTS);
    if (!MYSQLSUCCESS(rc)) //Error
	{
	    error_out();
	    // Deallocate handles and disconnect.
	    SQLFreeStmt(hstmt,SQL_DROP);
	    SQLDisconnect(hdbc);
	    SQLFreeConnect(hdbc);
	    SQLFreeEnv(henv);
	    exit(-1);
	}
    else
	{
	    for (rc=SQLFetch(hstmt); rc == SQL_SUCCESS; rc=SQLFetch(hstmt))
		{
		    SQLGetData(hstmt,1,SQL_C_CHAR,szData,sizeof(szData),&cbData);
		    // In this example, the data is returned in a messagebox
		    // for simplicity. However, normally the SQLBindCol() ODBC API
		    // could be called to bind individual rows of data and assign
		    // for a rowset.
		    MessageBox(NULL,(const char *)szData,"ODBC",MB_OK);
		}
	}
}

// Free the statement handle, disconnect, free the connection handle, and
// free the environment handle.
void direxec::sqldisconn(void)
{
    SQLFreeStmt(hstmt,SQL_DROP);
    SQLDisconnect(hdbc);
    SQLFreeConnect(hdbc);
    SQLFreeEnv(henv);
}

// Display error message in a message box that has an OK button.
void direxec::error_out(void)
{
    unsigned char szSQLSTATE[10];
    SDWORD nErr;
    unsigned char msg[SQL_MAX_MESSAGE_LENGTH+1];
    SWORD cbmsg;

    while(SQLError(0,0,hstmt,szSQLSTATE,&nErr,msg,sizeof(msg),&cbmsg)== SQL_SUCCESS)
	{
	    wsprintf((char *)szData,"Error:\nSQLSTATE=%s, Native error=%ld,msg='%s'",szSQLSTATE,nErr,msg);
	    MessageBox(NULL,(const char *)szData,"ODBC Error",MB_OK);
	}
}
