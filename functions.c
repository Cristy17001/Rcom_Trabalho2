#include "functions.h"

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