#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "macros.h"

typedef struct {
    char *username;
    char *password;
    char *host_name;
    char *file_path;
    char *file_name;
} FTP_args;

typedef struct {
    char *ip1;
    char *ip2;
    char *ip3;
    char *ip4;
    char *portMSB;
    char *portLSB;
} PassiveInfo;



int separateFileFromPath(const char *src, char **path, char **file);
int parseFTP(const char *url, FTP_args* ftp_args);
int getHostIp(char* host, char* ip);
int ftpConnect(char* server_ip, int port);
int readCodeFromSocket(int socket, char* code);
int sendToSocket(int socket, char* command, char* command_argument);
int login(int socket, FTP_args ftp_args);
int setPassiveConnection(int socket, char* receiveIP, int* receivePort);
int regexPassiveInfo(char* buffer, PassiveInfo* info);
int readPassiveInfo(int socket, char* ip, int* port);
int setRETR(int socket, FTP_args ftp_args);
int saveFile(int socket, char* filename);
int closeConnection(int socket);