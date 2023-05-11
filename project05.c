#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

//predefined sizes 
#define SIZE 1024
#define MAX_CLIENTS 10
#define BUFSIZE 8096
#include <fcntl.h>

// search and replace  all occurrences of a word with another word ... .
int exists(const char *fname)
{
    FILE *file;
    if ((file = fopen(fname, "r"))) //open and read file
    {
        fclose(file);
        return 1;
    }
    return 0;
}

const char *get_filename_ext(const char *filename) { //get file name
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}


char * get_failure_msg() { //error message 

    char *final = (char *) malloc( 1024 * sizeof (char)) ;

    char *failure_msg = "\
<!DOCTYPE html>\n\
<html>\n\
<body>\n\
Not Found\n\
</body>\n\
</html>\n";

    char *return_msg = "\
HTTP/1.1 404 Not Found\n\
Content-Type: text/html\n\
Content-Length: ";

    int length = strlen(failure_msg); //length of error msg
    char *str ;
    asprintf(&str, "%i", length);
    strcat(final,return_msg);
    strcat(final, str);
    strcat(final, "\n\n");
    strcat(final, failure_msg); //add failure_msg to final var 
    return final ;
}

int fsize(FILE *fp){ //filesize in int val
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END); 
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); 
    return sz;
}

int create_and_bind_socket(int port) //call by val port 
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr; //struct created 

    addr.sin_family = AF_INET; //struct vals 
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bindResult = bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (bindResult == -1)
    {
        perror("Error in binding \n"); //error message after port fails 
    }

    int listenResult = listen(server_socket, 5);
    if (listenResult == -1) //-1 error message 
    {
        perror("Error in listen");
    }
     return server_socket;
}


int main(int argc, char **argv)
{
    int port = 8064 ; //current port val

    FILE *fp;
    fp = fopen("port.txt", "r"); //read file containing port num
    if (fp != NULL) { //if file is not empty ...  
        fscanf(fp, "%d", &port);
        fclose(fp); //unbounded error check 
    }

    for(int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) { //'-p' command in command line is used to enter given port value
            port = atoi(argv[i+1]);
        }
    }

    printf("Starting http server in port:%d\n", port); //error check message to see if port connected  

    int server_socket = create_and_bind_socket(port); 
    // listener socket is added to poll with POLLIN...

    struct pollfd pollfds[MAX_CLIENTS + 1];   // monitors for POLLIN events on all sockets in polls()
    
    pollfds[0].fd = server_socket;
    pollfds[0].events = POLLIN ;    
    int useClient = 0; //initializing to 0 
    int client_socket = 0; //initializing to 0 


    while (1)
    {
        for (int i = 1; i < MAX_CLIENTS; i++) {

            int pollResult = poll(pollfds, useClient + 1, 10000);
            if (pollResult > 0) //

                    if (pollfds[0].revents & POLLIN) {
                        struct sockaddr_in cliaddr;
                        int addrlen = sizeof(cliaddr); //passing sizeof  cliaddr struct to addrlen 
                        client_socket = accept(server_socket, (struct sockaddr *) &cliaddr, &addrlen); //accept function runs 
						
                        printf("creating new client socket\n");
                        pollfds[i].fd = client_socket;
                        pollfds[i].events = POLLIN;
                        useClient++;
                    }
                    
					//handle the incoming connection when there is POLLIN event on the listener socket
					
					//handle the HTTP request  when there is POLLIN event on the accepted socket...
                    if (pollfds[i].fd > 0 && pollfds[i].revents & POLLIN) {
                        char buf[SIZE];
                        int bufSize = read(pollfds[i].fd, buf, 1024);
						
                        if (bufSize == -1 || bufSize == 0) { //bufsize error check code 
                            pollfds[i].fd = 0;
                            pollfds[i].events = 0;
                            pollfds[i].revents = 0;
                            useClient--;
                        } else {
                            char sep = ' ';
                            char *request_path = "";
                            char *dhup = strdup(buf);
                            request_path = strsep(&dhup, &sep);
                            request_path = strsep(&dhup, &sep);
                            printf("-------------------------\n");
                            printf("Request path :%s:\n", request_path);
                            char *filename = (char *) malloc(10000 * sizeof(char));

                            if (strcmp(request_path, "/") == 0) {
                                strcat(filename, "./www/");
                                strcat(filename, "index.html");
                            } else {
                                strcat(filename, "./www");
                                strcat(filename, request_path);
                            }

                            if ( !exists(filename) ) { //error check if the given filename is not found ...
                                printf("URL not found\n");
                                char * reply_bufffer = get_failure_msg();
                                (void) write(pollfds[i].fd, reply_bufffer, strlen(reply_bufffer));
                                free(reply_bufffer); //free malloc 
                                close(client_socket); //unbounded error check 
                                continue;
                            }
                            printf("URL found\n");
                            FILE *fp = fopen(filename, "r");
                            printf("File name :%s:\n", filename);
                            char reply_bufffer[BUFSIZE + 1];

                            if (strcmp(filename, "./www/slides/network-2.html") == 0) {
                                close(client_socket);
                                continue;
                            }


                            char *file_type = NULL;
                            printf("File type %s\n", get_filename_ext(filename));
                            if (strcmp(get_filename_ext(filename), "html") == 0) {  //for .html file
                                file_type = "text/html";
                            }
                            if (strcmp(get_filename_ext(filename), "css") == 0) { //for .css file
                                file_type = "text/css";
                            }
                            if (strcmp(get_filename_ext(filename), "ico") == 0) { //for .ico file
                                file_type = "image/icon";
                            }
                            if (strcmp(get_filename_ext(filename), "png") == 0) { //for .png file
                                file_type = "image/png";
                            }
                            if (strcmp(get_filename_ext(filename), "jpg") == 0) { //for .jpg file
                                file_type = "image/jpg";
                            }
                            if (strcmp(get_filename_ext(filename), "js") == 0) { //for .js file
                                file_type = "application/javascript";
                            }
                            if (file_type != NULL) {

                                long file_size = fsize(fp);
                                printf("File size %d\n", file_size);

                                (void) sprintf(reply_bufffer, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", file_type);
                                (void) write(pollfds[i].fd, reply_bufffer, strlen(reply_bufffer));
                                int file_fd;

                                if ((file_fd = open(filename, O_RDONLY)) == -1) /* open the file for reading */
                                    printf("Error\n");

                                long ret;
                                while ((ret = read(file_fd, reply_bufffer, BUFSIZE)) > 0) {
                                    (void) write(pollfds[i].fd, reply_bufffer, ret);
                                }

                                (void) write(pollfds[i].fd,"\n", 1);
                                printf("Response sent for %s\n", filename);

                            } else {
                                printf("File type is empty:%s", filename);
                                printf("Response Not sent for %s\n", filename);
                            }
                            close(client_socket); //unbounded error checking 
                        }

                    }
            }


    }

    close(server_socket);
    return 0;
}
