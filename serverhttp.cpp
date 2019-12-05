//
//  serverhttp.cpp
//  serverhttp
//
//  Created by Andrew Tran on 10/19/19.
//  Copyright © 2019 Andrew Tran. All rights reserved.
//
//  used resources
//  https://www.geeksforgeeks.org/socket-programming-cc/
//  https://www.rosipov.com/blog/c-strtok-usage-example/
//  https://stackoverflow.com/questions/6357031/how-do-you-convert-a-byte-array-to-a-hexadecimal-string-in-c
//  https://www.geeksforgeeks.org/getopt-function-in-c-to-parse-command-line-arguments/
//  TA slides
//  asgn0 code
//  asgn1 code

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <iostream>
#include <chrono>
#define PORT "80"
#define buffer 32768

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;
bool logging;
char offset;

char *logfilename;
int globalfd = -1;
int f = -1;
int z = -1;
int new_socket;
int logfile;
int status_code;
int history_of_fd[30];
char logmessage[buffer];
char logterminate[9];
char filedata[buffer];
char* newfilename;

int check_no_loop;
ssize_t full_log_output;
ssize_t realcontentlength;
ssize_t read_length;

void handleLog(char*, char*);

void handleReq(int newSocket) {
    check_no_loop = 0;
    char data[buffer];
    char newHeader[buffer];
    char *token[80];
    char header[buffer];
    char lengthheader[buffer];
    ssize_t write_length;
    sprintf(logterminate, "========\n");
    //long contentlength = 0;
    char const *get = "GET";
    char const *put = "PUT";
    read_length = read(newSocket, newHeader, buffer);
       token[0] = strtok(newHeader, " ");
           int i = 0;
           while (token[i] != NULL) { //parses header file
               i++;
               token[i] = strtok(NULL, " ");
           }

        newfilename = token[1] + 1; //gets rid of '/' this is file name
           //----------------------------GET-----------------------------//
       if( strcmp(token[0], get) == 0 ) {        // get code
           char* name = newfilename;
           int checkfile = open(name, O_RDONLY);
           if (errno == EACCES) {
               sprintf(header,("HTTP/1.1 403 Permission\r\n"));
               status_code = 403;
               handleLog(token[0], name);
               send(newSocket, header, strlen(header), 0);
               close(newSocket);
           }
           
           if (strlen(newfilename) < 27 || strlen(newfilename) > 27) {
               sprintf(header,("HTTP/1.1 400 Bad Request\r\n"));
               send(newSocket, header, strlen(header), 0);
               close(newSocket);
           }
           
           int k = 0;
           for (k = 0; k <= 26; k++) {
               if ((name[k] >= 'a' && name[k] <= 'z')
                   || (name[k] >= 'A' && name[k] <= 'Z')
                   || (name[k] >= '0' && name[k] <= '9')
                   || (name[k] == '_')
                   || (name[k] == '-')) {
                   continue;
                   
               }

               else {
                   sprintf(header,("HTTP/1.1 400 Bad Request\r\n"));
                   status_code = 400;
                   handleLog(token[0], name);
                   send(newSocket, header, strlen(header), 0);
                   close(newSocket);
               }
           }
           
           if (checkfile == -1) {
               sprintf(header,("HTTP/1.1 404 Not Found\r\n"));
                status_code = 404;
                handleLog(token[0], name);
                send(newSocket, header, strlen(header), 0);
                close(newSocket);
           }
           else {
               struct stat stat_buf;
               fstat(checkfile, &stat_buf);
               status_code = 200;

               //sprintf(getheader, "GET /%s HTTP/1.1\r\n", newfilename);
               sprintf(header, ("HTTP/1.1 200 OK\r\n"));
               sprintf(lengthheader, "Content-Length: %ld\r\n\r\n", stat_buf.st_size);
               //send(new_socket, getheader, sizeof(getheader), 0);
               send(newSocket, header, strlen(header), 0);
               send(newSocket, lengthheader, strlen(lengthheader), 0);
               while ((read_length = read(checkfile, data, buffer)) > 0) { // reads from the file specified, filedesc opened
                   write_length = send(newSocket, data, (ssize_t) read_length, 0); // writes to client
                   handleLog(token[0], newfilename);
               }
           }
       }
           //----------------------------end GET-----------------------------//
       
       
       //----------------------------PUT-----------------------------//
       else if (strcmp(token[0], put) == 0) {      //put code
           if (strlen(newfilename) != 27) {
               sprintf(header,("HTTP/1.1 400 Bad Request\r\n"));
               send(newSocket, header, strlen(header), 0);
               close(newSocket);
           }
           int k = 0;
           for (k = 0; k <= 26; k++) {
               if ((newfilename[k] >= 'a' && newfilename[k] <= 'z')
                   || (newfilename[k] >= 'A' && newfilename[k] <= 'Z')
                   || (newfilename[k] >= '0' && newfilename[k] <= '9')
                   || (newfilename[k] == '_')
                   || (newfilename[k] == '-')) {
                   continue;
               }
               else {
                   sprintf(header,("HTTP/1.1 400 Bad Request\r\n"));
                   status_code = 400;
                   handleLog(token[0], newfilename);
                   send(newSocket, header, strlen(header), 0);
                   close(newSocket);
               }
           }
           realcontentlength = atoi(token[6]);
           char* name = newfilename;
           if (status_code == 400) {
               return;
           }
           int filedesc = open(newfilename, O_TRUNC | O_RDWR | O_CREAT, 0644);
           if (errno == EACCES) {
               sprintf(header,("HTTP/1.1 403 Permission\r\n"));
               status_code = 403;
               handleLog(token[0], newfilename);
               send(newSocket, header, strlen(header), 0);
               close(newSocket);
           }
           if (filedesc == -1){
               sprintf(header,("HTTP/1.1 404 Not Found\r\n"));
                status_code = 404;
                handleLog(token[0], newfilename);
                send(newSocket, header, strlen(header), 0);
                close(newSocket);
           }
           else {
               if(status_code == 400 || status_code == 403 || status_code == 404) {
                   return;
               }
               sprintf(header, ("HTTP/1.1 201 Created\r\n"));
               status_code = 201;
               //sscanf(data, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %ld", &contentlength);
               sprintf(lengthheader, "Content-Length: 0\r\n");
               send(newSocket, header, strlen(header), 0);
               send(newSocket, lengthheader, strlen(lengthheader), 0);
               sprintf(logmessage, "PUT %s length %ld\n", name, realcontentlength);
               while((read_length = read(newSocket, filedata, buffer)) > 0) {
                   write_length = write(filedesc, filedata, (ssize_t) read_length);
                   close(newSocket);
               }
               if (logging) {
               filedesc = open(newfilename, O_RDONLY);
                     ssize_t byte_counter = 0;
                     ssize_t twentyByte = 0;
                     ssize_t l;
                     ssize_t j;
                     char zero_padded[20];
                     char hex[buffer];
                     char line[200];
                 while (byte_counter < realcontentlength) {
                     read_length = read(filedesc, data, 20); //trouble here
                     sprintf(zero_padded, "%08zd", twentyByte); // 8 dig, prefix 0
                     if(read_length == 20) {
                         twentyByte += 20;
                     }
                     for (l = 0, j = 0; l <= read_length; l += 1, j += 3)  {
                         sprintf(hex+j, "%02x ", filedata[l]); //2 dig prefix 0, lower case hex, space between
                     }
                     hex[strlen(hex) - 3] = '\0';
                     sprintf(line, "%s %s\n", zero_padded, hex);
                     strcat(logmessage, line);
                     byte_counter += read_length;
                 }
               strcat(logmessage, logterminate);
               full_log_output = pwrite(logfile, logmessage, sizeof(logmessage), offset);
               pthread_mutex_lock(&mutex);
               offset = offset+full_log_output;
               pthread_mutex_unlock(&mutex);

                   close(filedesc);
               }
           }
           //----------------------------end PUT-----------------------------//

           //----------------------------POST-------------------------------//

       }
       else {  //anything not GET or PUT
           sprintf(header, ("HTTP/1.1 500 Internal Server Error\r\n"));
           sprintf(logmessage, "FAIL: PUT %s HTTP/1.1 --- response 500\n", newfilename);
           strcat(logmessage, logterminate);
           full_log_output = pwrite(logfile, logmessage, sizeof(logmessage), offset);
           pthread_mutex_lock(&mutex);
           offset = offset+full_log_output;
           pthread_mutex_unlock(&mutex);
           send(newSocket, header, strlen(header), 0);
       }
    //----------------------------end POST-----------------------------//

}


void handleLog(char *type, char *filename) {
    if (check_no_loop == 1) {
        return;
    }
    newfilename = filename;
    if(logging) {
        if (strcmp(type, "GET") == 0 && status_code == 200) {
            sprintf(logmessage, "GET %s length 0\n", newfilename);
            strcat(logmessage, logterminate);
            full_log_output = pwrite(logfile, logmessage, strlen(logmessage), offset);
            pthread_mutex_lock(&mutex);
            offset = offset+full_log_output;
            pthread_mutex_unlock(&mutex);
            check_no_loop = 1;
        }
        if (strcmp(type, "GET") == 0 && status_code == 400) {
            sprintf(logmessage, "FAIL: GET %s HTTP/1.1 --- response 400\n", newfilename);
            strcat(logmessage, logterminate);
            full_log_output = pwrite(logfile, logmessage, sizeof(logmessage), offset);
            pthread_mutex_lock(&mutex);
            offset = offset+full_log_output;
            pthread_mutex_unlock(&mutex);
            check_no_loop = 1;
        }
        if (strcmp(type, "GET") == 0 && status_code == 403) {
            sprintf(logmessage, "FAIL: GET %s HTTP/1.1 --- response 403\n", newfilename);
            strcat(logmessage, logterminate);
            full_log_output = pwrite(logfile, logmessage, sizeof(logmessage), offset);
            pthread_mutex_lock(&mutex);
            offset = offset+full_log_output;
            pthread_mutex_unlock(&mutex);
            check_no_loop = 1;
        }
        if (strcmp(type, "GET") == 0 && status_code == 404) {
            sprintf(logmessage, "FAIL: GET %s HTTP/1.1 --- response 404\n", newfilename);
            strcat(logmessage, logterminate);
            full_log_output = pwrite(logfile, logmessage, sizeof(logmessage), offset);
            pthread_mutex_lock(&mutex);
            offset = offset+full_log_output;
            pthread_mutex_unlock(&mutex);
            check_no_loop = 1;
        }
        if (strcmp(type, "PUT") == 0 ) {
            if (status_code == 201){
                
                
            }
            if (status_code == 400) {
                sprintf(logmessage, "FAIL: PUT %s HTTP/1.1 --- response 400\n", newfilename);
                strcat(logmessage, logterminate);
                full_log_output = pwrite(logfile, logmessage, sizeof(logmessage), offset);
                pthread_mutex_lock(&mutex);
                offset = offset+full_log_output;
                pthread_mutex_unlock(&mutex);
                check_no_loop = 1;
            }
            if (status_code == 403) {
                sprintf(logmessage, "FAIL: PUT %s HTTP/1.1 --- response 430\n", newfilename);
                strcat(logmessage, logterminate);
                full_log_output = pwrite(logfile, logmessage, sizeof(logmessage), offset);
                pthread_mutex_lock(&mutex);
                offset = offset+full_log_output;
                pthread_mutex_unlock(&mutex);
                check_no_loop = 1;
            }
            if (status_code == 404) {
                sprintf(logmessage, "FAIL: PUT %s HTTP/1.1 --- response 404\n", newfilename);
                strcat(logmessage, logterminate);
                full_log_output = pwrite(logfile, logmessage, sizeof(logmessage), offset);
                pthread_mutex_lock(&mutex);
                offset = offset+full_log_output;
                pthread_mutex_unlock(&mutex);
                check_no_loop = 1;
            }
        }
    }
}

void * dispatchThread(void*) {
    while(true) {
        int fd = -1;
        pthread_mutex_lock(&mutex);
        if (globalfd == -1) {
            pthread_cond_wait(&condition_var, &mutex);
            fd = globalfd;
            globalfd = -1;
        }
        z++;
        fd = history_of_fd[z];
        pthread_mutex_unlock(&mutex);

        handleReq(fd);
    }
}



int main(int argc, char *argv[])
{
    int server_fd;
    char *host;
    char *specifiedport = (char*)PORT;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt;
    int number_of_threads = 4;
    int optval = 1;
    
    while ((opt = getopt(argc, argv, "N:l:")) != -1) {
        switch(opt) {
            case 'N':
                number_of_threads = atoi(optarg);
                break;
            case 'l':
                logging = true;
                logfilename = optarg;
                logfile = open(logfilename, O_TRUNC | O_RDWR | O_CREAT, 0644);
                break;
            case '?':
                break;
        }
    }
    
    if (optind != -1) {
    host = argv[optind];
        if (optind + 1 != -1) {
            specifiedport = argv[optind+1];
        }
    }
   
    struct addrinfo *addrs, hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(host, specifiedport, &hints, &addrs);
    
    if ((server_fd = socket(addrs->ai_family, addrs->ai_socktype, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(optval))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, addrs->ai_addr,
                                 addrs->ai_addrlen)<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    pthread_t *my_thread;
    my_thread = (pthread_t*)malloc(sizeof(pthread_t)*number_of_threads);
    int i;
    for (i = 0; i < number_of_threads; i++) {
        int newThread = pthread_create(&my_thread[i], NULL, dispatchThread, NULL);
        if (newThread != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    while (true) {
        
        if ((new_socket = accept(server_fd, addrs->ai_addr, (socklen_t*)&addrlen))<0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        f++;
    
        history_of_fd[f] = new_socket;

        // lock a mutex
        pthread_mutex_lock(&mutex);
        // set globalfd to accepted fd
        globalfd = new_socket;
        // signal
        pthread_cond_signal(&condition_var);
        // unlock
        pthread_mutex_unlock(&mutex);

        //close(server_fd);
    }
}

