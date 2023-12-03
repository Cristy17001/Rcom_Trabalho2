#include "functions.h"


bool isConnectionSuccessful(int sockfd) {
    // Check if the socket file descriptor is valid
    return (sockfd >= 0);
}

int readReply(FILE * readSockect){
    long code;
    char * aux;
    
    char *buf;
    size_t bufsize = 256;
    size_t characters;


    while(1){
        buf = (char *)malloc(bufsize * sizeof(char));
        characters = getline(&buf,&bufsize,readSockect);
        printf("> %s", buf);
        
        if((characters > 0) && (buf[3] == ' ')){
            code = strtol(buf, &aux, 10); 
            
            if(code >= 500 && code <= 559){
                printf("Error\n");
                exit(1);
            }
            //printf("Code: %li\n", code);
            break;
        }
        free(buf);
    }  

    return 0;
}

int separateFileFromPath(const char *src, char **path, char **file) {
    const char *pattern = "^(.*/)([^/]+)$";

    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile regular expression\n");
        return 1;
    }

    regmatch_t matches[3]; // One for the whole match and one for each capturing group

    if (regexec(&regex, src, 3, matches, 0) == 0) {
        for (int i = 1; i <= 2 && matches[i].rm_so != -1; i++) {
            // The starting offset of the matched substring
            int start = matches[i].rm_so;

            // The ending offset of the matched substring
            int end = matches[i].rm_eo;
            if (i == 1) {
                *path = strndup(src + start, end - start);
            }
            else if (i == 2) {
                *file = strndup(src + start, end - start);
            }
            //printf("Group %d: %.*s\n", i, end - start, src + start);
        }
    }
    else {
        fprintf(stderr, "No match found\n");
    }

    regfree(&regex);
    return 0;
}

int sendCommand(int sockfd, char * command){
    int bytesSent;
    bytesSent = write(sockfd, command, strlen(command));

    if(bytesSent == 0){
        printf("sendCommand: connection closed\n");
        return 1;
    }
    if(bytesSent == -1){
        printf("error sending command\n");
        return 1;
    }
    printf("%s\n",command);
    return 0;
}

int parseFTP(const char *url, FTP_args* ftp_args) {
    // The regular expression pattern
    const char *pattern = "ftp://([a-zA-Z0-9_-]{3,16}):([a-zA-Z0-9@!$]{1,})@([^:/]+)(/.+)?";

    // Compile the regular expression
    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile regular expression\n");
        return 1;
    }

    // Match the regular expression against the input URL
    regmatch_t matches[5]; // One for the whole match and one for each captured group
    char* pathAndFile = NULL;

    if (regexec(&regex, url, 5, matches, 0) == 0) {
        // Extract and print captured groups
        for (int i = 1; i <= 4 && matches[i].rm_so != -1; i++) {
            // The starting offset of the matched substring
            int start = matches[i].rm_so;

            // The ending offset of the matched substring
            int end = matches[i].rm_eo;
            switch (i) {
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
    } else {
        fprintf(stderr, "No match found\n");
    }

    separateFileFromPath(pathAndFile, &ftp_args->file_path, &ftp_args->file_name);
    
    // Free resources
    regfree(&regex);

    return 0;
}

int getHostIp(char* host, char* ip) {
    struct hostent *h;
    if ((h = gethostbyname(host)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }
    strcpy(ip, inet_ntoa(*((struct in_addr *) h->h_addr)));
    return 0;
}

int ftpConnect(char* server_ip) {
    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(FTP_PORT);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }

    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    return sockfd;
}

int ftpLogin(int sockfd, const char *username, const char *password, FILE *readSocket) {
    char command[256];

    // Send USER command
    sprintf(command, "user %s\n", username);
    if (sendCommand(sockfd, command) != 0) {
        return -1;  // Error in sending command
    }
    readReply(readSocket);

    // Send PASS command
    sprintf(command, "pass %s\n", password);
    if (sendCommand(sockfd, command) != 0) {
        return -1;  // Error in sending command
    }
    readReply(readSocket);

    return 0;  // Login successful
}

int ftpChangeDirectory(int sockfd, const char *directory, FILE *readSocket) {
    char command[256];

    // Send CWD command
    sprintf(command, "cwd %s\n", directory);
    if (sendCommand(sockfd, command) != 0) {
        return -1;  // Error in sending command
    }
    readReply(readSocket);

    return 0;  // Directory change successful
}

int ftpSetPassiveMode(int sockfd, int *port, char *ip, FILE *readSocket) {
    char command[256];

    // Send PASV command
    sprintf(command, "pasv \n");
    if (sendCommand(sockfd, command) != 0) {
        return -1;  // Error in sending command
    }

    // Read IP and port from the server's response
    readIpPort(ip, port, readSocket);
    
    // Open connection for passive mode
    if (startConnection(ip, *port, &sockfd) != 0) {
        printf("Error starting passive connection\n");
        return -1;
    }

    return 0;  // Passive mode setup successful
}

int startConnection(const char *ip, int port, int *sockfd) {
    struct sockaddr_in server_addr;

    // Initialize server address structure
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Open a new TCP socket for passive connection
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    // Connect to the server
    if (connect(*sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        close(*sockfd);  // Close the socket in case of failure
        return -1;
    }

    return 0;  // Connection successful
}
