gcc project_sql_client.c -o sql_client -D_REENTRANT -pthread -lmysqlclient
gcc project_bluetooth_client.c -o bluetooth_client -lbluetooth -pthread
gcc project_server.c -o server -D_REENTRANT -pthread
