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

    int sockfd = 0;

    // Get the actual Ip address
    char ip[BUFFSIZE];
    if (getHostIp(ftp_args.host_name, ip) != 0) {
        printf("Error getting the Host Ip!\n");
        return -1;
    }
    printf("IP address: %s\n", ip);

    // Connect to the ftp server via socket
    if ((sockfd = ftpConnect(ip, FTP_PORT)) < 0) {
        printf("Error establishing connection via FTP!");
        return -1;
    }

    char code[4]; 
    if (readCodeFromSocket(sockfd, code) != 0) {
        return -1;
    }
    
    if (code[0] == '2') {
        printf("Waiting for the username!\n");
    }
    else {
        printf("Error with connection, didn't receive code 220 for confirmation!\n");
        return -1;
    }
    

    // Login in the ftp server
    if (login(sockfd, ftp_args) != 0) {
        printf("Failed during the login process!\n");
        return -1;
    }

    char receiveIP[BUFFSIZE];
    int receivePort = 0;
    // Set the server to passive mode
    if (setPassiveConnection(sockfd, receiveIP, &receivePort) != 0) {
        printf("Passive mode configuration failed!\n");
        return -1;
    }

    printf("The IP to connect to is %s\n", receiveIP);
    printf("The PORT to connect to is %d\n", receivePort);

    // Open another connection to receive the data
    int sockfdReceive = 0;
    if ((sockfdReceive = ftpConnect(receiveIP, receivePort)) < 0) {
        printf("Error establishing connection via FTP!");
        return -1;
    }

    // Send the retr to the ftp server
    if (setRETR(sockfd, ftp_args) != 0) {
        printf("Setting retr failed!\n");
        return -1;
    }   

    if (saveFile(sockfdReceive, ftp_args.file_name) != 0) {
        printf("Error saving the file!\n");
        return -1;
    }

    if (closeConnection(sockfd) != 0) {
        printf("Error closing connection!\n");
        return -1;
    }

    return 0;
}