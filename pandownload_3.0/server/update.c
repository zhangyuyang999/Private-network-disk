#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>

int update(char *query)
{
	MYSQL *conn;
	const char* server="localhost";
	const char* user="root";
	const char* password="123";
	const char* database="test";
	int t;
	conn=mysql_init(NULL);
	if(!mysql_real_connect(conn,server,user,password,database,0,NULL,0))
	{
		printf("Error connecting to database:%s\n",mysql_error(conn));
	}else{
		printf("Connected...\n");
	}
	t=mysql_query(conn,query);
	if(t)
	{
		printf("Error making query:%s\n",mysql_error(conn));
	}else{
		printf("update success\n");
	}
	mysql_close(conn);
	return 0;
}
