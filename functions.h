#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include "macros.h"

typedef struct {
    char *username;
    char *password;
    char *host_name;
    char *file_path;
    char *file_name;
} FTP_args;

int separateFileFromPath(const char *src, char **path, char **file);
int parseFTP(const char *url, FTP_args* ftp_args);
int getHostIp(char* host, char* ip);
int ftpConnect(char* server_ip);
