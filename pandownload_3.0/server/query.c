#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>
int Query(char * query,char *retval)
{
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	const char* server="localhost";
	const char* user="root";
	const char* password="123";
	const char* database="test";//要访问的数据库名称
	unsigned int t;

	conn=mysql_init(NULL);

	if(!mysql_real_connect(conn,server,user,password,database,0,NULL,0))
	{
		printf("Error connecting to database:%s\n",mysql_error(conn));
		return -1;
	}else{
		printf("Connected...\n");
	}

	t=mysql_query(conn,query);//Zero for success. Nonzero if an error occurred.

	if(t)
	{
        printf("Error making query:%s\n",mysql_error(conn));  
        return -1;
	}else{
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res)) == NULL) {
                printf("Don't find data\n");//if query statement is wroing,this function will return NULL?
                return -1;
            } 
            while(row) {
                for (t=0; t<mysql_num_fields(res); t++) {
                    if(strlen(retval) == 0) {
                        strncpy(retval,row[t],strlen(row[t]));
                    } else {
                        sprintf(retval,"%s %s",retval,row[t]);
                    }
                }
                row=mysql_fetch_row(res);
                if(row) {
                    sprintf(retval,"%s%s",retval,"\n");
                }
            }
        }else{
            printf("Don't find data\n");
            return -1;
        }
        mysql_free_result(res);
    }
    mysql_close(conn);
    return 0;
}
