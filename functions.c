#include "functions.h"

int separateFileFromPath(const char *src, char **path, char **file)
{
    const char *pattern = "^(.*/)([^/]+)$";

    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Failed to compile regular expression\n");
        return 1;
    }

    regmatch_t matches[3]; // One for the whole match and one for each capturing group

    if (regexec(&regex, src, 3, matches, 0) == 0)
    {
        for (int i = 1; i <= 2 && matches[i].rm_so != -1; i++)
        {
            // The starting offset of the matched substring
            int start = matches[i].rm_so;

            // The ending offset of the matched substring
            int end = matches[i].rm_eo;
            if (i == 1)
            {
                *path = strndup(src + start, end - start);
            }
            else if (i == 2)
            {
                *file = strndup(src + start, end - start);
            }
            // printf("Group %d: %.*s\n", i, end - start, src + start);
        }
    }
    else
    {
        fprintf(stderr, "No match found\n");
    }

    regfree(&regex);
    return 0;
}

int parseFTP(const char *url, FTP_args *ftp_args)
{
    // The regular expression pattern
    const char *pattern = "ftp://([a-zA-Z0-9_-]{3,16}):([a-zA-Z0-9@!$]{1,})@([^:/]+)(/.+)?";

    // Compile the regular expression
    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
    {
        fprintf(stderr, "Failed to compile regular expression\n");
        return 1;
    }

    // Match the regular expression against the input URL
    regmatch_t matches[5]; // One for the whole match and one for each captured group
    char *pathAndFile = NULL;

    if (regexec(&regex, url, 5, matches, 0) == 0)
    {
        // Extract and print captured groups
        for (int i = 1; i <= 4 && matches[i].rm_so != -1; i++)
        {
            // The starting offset of the matched substring
            int start = matches[i].rm_so;

            // The ending offset of the matched substring
            int end = matches[i].rm_eo;
            switch (i)
            {
            case 1:
                ftp_args->username = strndup(url + start, end - start);
                break;
            case 2:
                ftp_args->password = strndup(url + start, end - start);
                break;
            case 3:
                ftp_args->host_name = strndup(url + start, end - start);
                break;
            case 4:
                pathAndFile = strndup(url + start, end - start);
                break;
            default:
                break;
            }
        }
    }
    else
    {
        fprintf(stderr, "No match found\n");
    }

    separateFileFromPath(pathAndFile, &ftp_args->file_path, &ftp_args->file_name);

    // Free resources
    regfree(&regex);

    return 0;
}

int getHostIp(char *host, char *ip)
{
    struct hostent *h;
    if ((h = gethostbyname(host)) == NULL)
    {
        herror("gethostbyname()");
        exit(-1);
    }
    strcpy(ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
    return 0;
}

int ftpConnect(char *server_ip, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip); /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);             /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        exit(-1);
    }

    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect()");
        exit(-1);
    }

    return sockfd;
}

int readCodeFromSocket(int socket, char *code) {
    printf("Starting to read from the ftp server to get the codes!\n");
    FILE *fp = fdopen(socket, "r");
    char *buffer = NULL;
    size_t bufferSize = 0;

    while (TRUE) {

        ssize_t bytesRead = getline(&buffer, &bufferSize, fp);
        if (bytesRead == -1) {
            return -1;
        }

        if ((buffer[0] >= '1' && buffer[0] < '5') || buffer[3] == ' ') {
            strncpy(code, buffer, 3);
            code[4] = '\0';
            printf("The Code is: %s\n", code);
            break;
        }
        else {
            printf("Got: '%s' from the sever\n", buffer);
        }
    }

    return 0;
}

int sendToSocket(int socket, char* command, char* command_argument) {
    printf("Sending: %s %s to FTP server!\n", command, command_argument);
    char command_buffer[BUFFSIZE];
    sprintf(command_buffer, "%s %s\n", command, command_argument);

    // Sending the command to the socket
    int bytes = write(socket, command_buffer, strlen(command_buffer));
    if (bytes != strlen(command_buffer)) {
        printf("Error Sending the command to the server!\n");
        return -1;
    }

    return 0;
}

int login(int socket, FTP_args ftp_args) {
    // Send the Username to the ftp server
    char code[4]; 

    while (!(code[0] == '3' && code[1] == '3' && code[2] == '1')) {
        printf("Sending username to the ftp server!\n");
        if (sendToSocket(socket, "USER", ftp_args.username) != 0) {
            printf("Something went wrong sending the username to the server!\n");
            return -1;
        }

        // Wait for the username confirm code 331 to enter the password
        if (readCodeFromSocket(socket, code) != 0) {
            printf("Something went wrong reading from the socket!\n");
            return -1;
        }
    }
    printf("Username correctly sent to the ftp server!\n");
    

    // Send the Password to the ftp server
    while (!(code[0] == '2' && code[1] == '3' && code[2] == '0')) {
        printf("Sending password to the ftp server!\n");
        if (sendToSocket(socket, "PASS", ftp_args.password) != 0) {
            printf("Something went wrong sending the password to the server!\n");
            return -1;
        }

        // Wait for the password confirm code 230 to enter the password
        if (readCodeFromSocket(socket, code) != 0) {
            printf("Something went wrong reading from the socket!\n");
            return -1;
        }
    }

    printf("Password correctly sent to the ftp server!\n");

    return 0;
}

int regexPassiveInfo(char* buffer, PassiveInfo* info) {
    // Use regex to find the ip numbers:
    regex_t regex;
    regmatch_t matches[7];

    // Compile the regular expression
    if (regcomp(&regex, "([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)", REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile regex\n");
        return 1;
    }

    if (regexec(&regex, buffer, 7, matches, 0) == 0)
    {
        for (int i = 1; i <= 7 && matches[i].rm_so != -1; i++)
        {
            // The starting offset of the matched substring
            int start = matches[i].rm_so;

            // The ending offset of the matched substring
            int end = matches[i].rm_eo;
            switch (i)
            {
            case 1:
                info->ip1 = strndup(buffer + start, end - start);
                break;
            case 2:
                info->ip2 = strndup(buffer + start, end - start);
                break;
            case 3:
                info->ip3 = strndup(buffer + start, end - start);
                break;
            case 4:
                info->ip4 = strndup(buffer + start, end - start);
                break;
            case 5:
                info->portMSB = strndup(buffer + start, end - start);
                break;
            case 6:
                info->portLSB = strndup(buffer + start, end - start);
                break;
            default:
                break;
            }
        }
    }
    else {
        fprintf(stderr, "No match found\n");
        return -1;
    }

    regfree(&regex);
    return 0;
}

int readPassiveInfo(int socket, char* ip, int* port) {
    FILE *fp = fdopen(socket, "r");
    char* buffer;
    size_t bytes;
    PassiveInfo info = {NULL, NULL, NULL, NULL, NULL, NULL};

    while (TRUE)
    {
        getline(&buffer, &bytes, fp);
        printf("Read: %s", buffer);
        if (buffer[3] == ' ') break;
    }    
    if (regexPassiveInfo(buffer, &info) != 0) {
        return -1;
    }

    sprintf(ip, "%s.%s.%s.%s", info.ip1, info.ip2, info.ip3, info.ip4);
    *port = atoi(info.portMSB)*256 + atoi(info.portLSB);

    return 0;
}

int setPassiveConnection(int socket, char* receiveIP, int* receivePort) {
    if (sendToSocket(socket, "pasv", "") != 0) {
        printf("Failed to send the pasv command!\n");
        return -1;
    }

    char ip[BUFFSIZE];
    int port = 0;

    if (readPassiveInfo(socket, ip, &port) != 0) {
        printf("Failed to read the passive information!\n");
        return -1;
    }


    strcpy(receiveIP, ip);
    *receivePort = port;

    return 0;
}

int setRETR(int socket, FTP_args ftp_args) {
    char fullpath[BUFFSIZE];
    sprintf(fullpath, "%s%s", ftp_args.file_path, ftp_args.file_name);
    if (sendToSocket(socket, "retr", fullpath) != 0) {
        printf("Error sending the retr command to the socket!\n");
        return -1;
    }
    return 0;
}

int saveFile(int socket, char* filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL){
        printf("Error opening or creating file\n");
        return -1;
    } 
    int bytes; 
    char buffer[BUFFSIZE*2];

    do {
        bytes = read(socket, buffer, sizeof(buffer));
        if (bytes == 0) {
            break;
        }
        printf("Received: %s\n", buffer);
        if (bytes < 0) {
            printf("Error reading from data socket\n");
            return -1;
        }

        if (bytes > 0) {
            if (fwrite(buffer, bytes, 1, fp) < 0) {
                printf("Error writing data to file\n");
                return -1;
            }
        }
    } while (bytes > 0);

    fclose(fp);
    close(socket);
    return 0;
}

int closeConnection(int socket) {
    char code[4];
    while (code[0] != '2') {
        if (sendToSocket(socket, "QUIT", "") != 0) {
            printf("Error ocurred when finishing the connection!\n");
            return -1;
        }
        if (readCodeFromSocket(socket, code) != 0) {
            printf("Something went wrong reading from the socket!\n");
            return -1;
        }
    }
    printf("Quitting from the ftp server!\n");
    return 0;
}