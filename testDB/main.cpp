#include <iostream>
#include "../../chatters/chatters/DBConnector.h"

using std::cin;
using std::cout;
using std::endl;

int main()
{
	DBConnector _dbc;

	if (_dbc.connect() != SQL_SUCCESS)
	{
		std::cout << "Database object creation failed." << std::endl;
		exit(1);
	}
	
	// SELECT * FROM chatters.dbo.login WHERE id='str_id' AND pw='str_pw'
	std::string stmt = "SELECT * FROM login;";

	RETCODE errmsg;

	if ((errmsg = _dbc.excute(stmt)) != SQL_SUCCESS)
	{	// excute() failed
		std::cout << "DBConnector::excute() get SQL error." << std::endl;
		exit(1);
	}

	int foundNumOfRow = 0;
	if ((errmsg = _dbc.getResultNum(foundNumOfRow)) != SQL_SUCCESS)
	{	// getResultNum(..) failed
		std::cout << "DBConnector::getResultNum() got SQL error." << std::endl;
		//exit(1);
	}

	cout << "Number of row: " << foundNumOfRow << endl;

	return 0;
}