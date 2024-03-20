#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <mysql/mysql.h>

#define BUF_SIZE	100
#define NAME_SIZE	20
#define ARR_CNT		7

#define DEBUG

void* send_msg(void* arg);
void* recv_msg(void* arg);
void error_handling(char* msg);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

void finish_with_error(MYSQL* con)
{
	fprintf(stderr, "%s\n", mysql_error(con));
	mysql_close(con);

	exit(1);
}

int main(int argc, char* argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread, mysql_thread;
	void* thread_return;

	if(argc != 4)
	{
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);

		exit(1);
	}

	sprintf(name, "%s", argv[3]);

	sock = socket(PF_INET, SOCK_STREAM, 0);

	if(sock < 0)
	{
		error_handling("socket() error!");
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
	{
		error_handling("connect() error");
	}

	sprintf(msg, "[%s:PASSWD]", name);

	write(sock, msg, strlen(msg));

	pthread_create(&rcv_thread, NULL, recv_msg, (void*) &sock);
	pthread_create(&snd_thread, NULL, send_msg, (void*) &sock);

	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);

	close(sock);

	return 0;
}

void* send_msg(void* arg)
{
	int* sock = (int*) arg;
	int str_len;
	int ret;
	fd_set initset, newset;
	struct timeval tv;
	char name_msg[NAME_SIZE + BUF_SIZE + 2];

	FD_ZERO(&initset);
	FD_SET(STDIN_FILENO, &initset);

	fputs("Input a message![ID]msg (Default ID:ALLMSG)\n", stdout);

	while(1)
	{
		memset(msg, 0, sizeof(msg));
		name_msg[0] = '\0';
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		newset = initset;

		ret = select(STDIN_FILENO + 1, &newset, NULL, NULL, &tv);

		if(FD_ISSET(STDIN_FILENO, &newset))
		{
			fgets(msg, BUF_SIZE, stdin);
			if(!strncmp(msg, "quit\n", 5))
			{
				*sock = -1;
				return NULL;
			}
			else if(msg[0] != '[')
			{
				strcat(name_msg, "[ALLMSG]");
				strcat(name_msg, msg);
			}
			else
			{
				strcpy(name_msg, msg);
			}

			if(write(*sock, name_msg, strlen(name_msg)) <= 0)
			{
				*sock = -1;
				return NULL;
			}
		}

		if(ret == 0)
		{
			if(*sock == -1)
			{
				return NULL;
			}
		}
	}
}

void* recv_msg(void* arg)
{
	MYSQL* conn;
	MYSQL_ROW sqlrow;
	int res;
	char sql_cmd[200] = { 0 };

	// mariaDB 접속
	char* host = "localhost";
	char* user = "iot";
	char* pass = "pwiot";
	char* dbname = "projectDB";

	int* sock = (int*) arg;
	int i;
	char* pToken;
	char* pArray[ARR_CNT] = { 0 };

	char name_msg[NAME_SIZE + BUF_SIZE + 1];
	int str_len;

	char Product_Name[10];
	int Count = 0;
	int Goal = 0;
	float Rate;
	
	conn = mysql_init(NULL);

	puts("MYSQL startup");

	if(!(mysql_real_connect(conn, host, user, pass, dbname, 0, NULL, 0)))
	{
		fprintf(stderr, "ERROR : %s[%d]\n", mysql_error(conn), mysql_errno(conn));

		exit(1);
	}
	else
	{
		printf("Connection Successful!\n\n");
	}

	while(1)
	{
		memset(name_msg, 0, sizeof(name_msg));
		str_len = read(*sock, name_msg, NAME_SIZE + BUF_SIZE);
		
		if(str_len <= 0)
		{
			*sock = -1;
			return NULL;
		}
		
		fputs(name_msg, stdout);

		name_msg[str_len - 1] = 0;

		pToken = strtok(name_msg, "[:@]");

		i = 0;

		while(pToken != NULL)
		{
			pArray[i] = pToken;

			if(++i >= ARR_CNT)
			{
				break;
			}

			pToken = strtok(NULL, "[:@]");
		}

		if(!strcmp(pArray[1], "MYID"))
		{
			printf("id : %s\n", pArray[2]);
			
			sprintf(sql_cmd, "SELECT * FROM person_list WHERE cardID = \'%s\'", pArray[2]);

			if(mysql_query(conn, sql_cmd))
			{
				finish_with_error(conn);			
			}

			MYSQL_RES* result = mysql_store_result(conn);

			if(result == NULL)
			{
				finish_with_error(conn);
			}
			

#ifdef DEBUG
			int num_fields = mysql_num_fields(result);
			printf("fields num : %d\n", num_fields);
#endif
			sqlrow = mysql_fetch_row(result);
						
			if(sqlrow == NULL)
			{
				sprintf(sql_cmd, "[PRJ_BT]ACCESS_DENIED\n");
				
				write(*sock, sql_cmd, strlen(sql_cmd));
				
				// memset(sql_cmd, 0, sizeof(sql_cmd));
			}
			
			else if(!strcmp(sqlrow[9], "0"))
			{
				sprintf(sql_cmd, "UPDATE person_list SET Attendance = '1' WHERE cardID = \'%s\'", pArray[2]);

				mysql_query(conn, sql_cmd);

				memset(sql_cmd, 0, sizeof(sql_cmd));

				sprintf(sql_cmd, "[%s]%s@%s\n", "PRJ_BT", "HELLO", sqlrow[2]);

				write(*sock, sql_cmd, strlen(sql_cmd));

				memset(sql_cmd, 0, sizeof(sql_cmd));

				// [PRJ_WIFI1]DEVICE@255@255@255@1@255
				sprintf(sql_cmd, "[%s]%s@%s@%s@%s@%s@%s\n", sqlrow[3], "DEVICE", sqlrow[4], sqlrow[5], sqlrow[6], sqlrow[7], sqlrow[8]);

#ifdef DEBUG
				printf("%s\n", sql_cmd);
#endif

				write(*sock, sql_cmd, strlen(sql_cmd));

				memset(sql_cmd, 0, sizeof(sql_cmd));

				sprintf(sql_cmd, "[PRJ_AND]%s@IN\n", sqlrow[2]);
#ifdef DEBUG
				printf(sql_cmd);
#endif
				
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
			else
			{
				sprintf(sql_cmd, "UPDATE person_list SET Attendance = '0' WHERE cardID = \'%s\'", pArray[2]);

				mysql_query(conn, sql_cmd);

				memset(sql_cmd, 0, sizeof(sql_cmd));

				sprintf(sql_cmd, "[PRJ_BT]GOODBYE@%s\n", sqlrow[2]);

				write(*sock, sql_cmd, strlen(sql_cmd));

				memset(sql_cmd, 0, sizeof(sql_cmd));

				sprintf(sql_cmd, "[%s]DEVICE@0@0@0@0@0\n", sqlrow[3]);

				write(*sock, sql_cmd, strlen(sql_cmd));

				memset(sql_cmd, 0, sizeof(sql_cmd));

				sprintf(sql_cmd, "[PRJ_AND]%s@OUT\n", sqlrow[2]);
#ifdef DEBUG
				printf(sql_cmd);
#endif
				
				write(*sock, sql_cmd, strlen(sql_cmd));
			}
		}
	}
	
	mysql_close(conn);
}
		
void error_handling(char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);

	exit(1);
}


