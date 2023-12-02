#include <stdio.h>
#include <string.h>
#include "functions.h"
#include "macros.h"

int main(int argc, char *argv[]) {
    // If diferent numbers of arguments where provided
    if (argc != 2) {
        printf("The correct way to use this is '%s %s' \n", argv[0], "ftp://[<user>:<password>@]<host>/<url-path>");
        return -1;
    }

    // Get the command line arguments and fill the arguments struct
    FTP_args ftp_args = {NULL, NULL, NULL, NULL, NULL};
    if (parseFTP(argv[1], &ftp_args) != 0) {
        printf("Error during parsing!\n");
        return -1;
    }

    printf("Username: %s\n", ftp_args.username);
    printf("Password: %s\n", ftp_args.password);
    printf("Host: %s\n", ftp_args.host_name);
    printf("Path: %s\n", ftp_args.file_path);
    printf("FileName: %s\n", ftp_args.file_name);

    char commands_buffer[BUFFSIZE];
    char responses_buffer[BUFFSIZE];
    int sockfd = 0;



    // Get the actual Ip address
    char ip[BUFFSIZE];
    if (getHostIp(ftp_args.host_name, ip) != 0) {
        printf("Error getting the Host Ip!\n");
        return -1;
    }
    printf("IP address: %s\n", ip);

    // Connect to the ftp server via socket
    if ((sockfd = ftpConnect(ip)) < 0) {
        printf("Error establishing connection via FTP!");
        return -1;
    }

    



    return 0;
}