#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

void *client_proc(void *);
double solve(double a, double b, const char *cmd, char *result_str);

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 10)) {
        perror("listen() failed");
        return 1;
    }

    while (1) {
        printf("Waiting for new client\n");
        int client = accept(listener, NULL, NULL);
        printf("New client accepted, client = %d\n", client);
        
        pthread_t tid;
        pthread_create(&tid, NULL, client_proc, &client);
        pthread_detach(tid);
    }

    return 0;
}

void *client_proc(void *arg) {
    int client = *(int *)arg;
    char buf[2048];

    int ret = recv(client, buf, sizeof(buf), 0);
    if (ret <= 0) {
        close(client);
        pthread_exit(NULL);
    }

    buf[ret] = 0;
    printf("Received from %d: %s\n", client, buf);
    
    if (strncmp(buf, "GET /", 5) == 0 || strncmp(buf, "POST /", 6) == 0) {
        char method[8], url[1024], version[16];
        sscanf(buf, "%s %s %s", method, url, version);

        if (strncmp(method, "GET", 3) == 0) {
            char *a_str = strstr(url, "a=");
            char *b_str = strstr(url, "b=");
            char *cmd_str = strstr(url, "cmd=");

            if (a_str && b_str && cmd_str) {
                double a = atof(a_str + 2);
                double b = atof(b_str + 2);
                char cmd[4];
                sscanf(cmd_str, "cmd=%3s", cmd);

                char result[256];
                solve(a, b, cmd, result);

                char response[1024];
                snprintf(response, sizeof(response), 
                         "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                         "<html><body><h1>Result: %s</h1></body></html>", result);
                send(client, response, strlen(response), 0);
            } else {
                const char *response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n"
                                       "<html><body><h1>Missing parameters</h1></body></html>";
                send(client, response, strlen(response), 0);
            }
        } else if (strncmp(method, "POST", 4) == 0) {
            char *content = strstr(buf, "\r\n\r\n") + 4;
            char *a_str = strstr(content, "a=");
            char *b_str = strstr(content, "b=");
            char *cmd_str = strstr(content, "cmd=");

            if (a_str && b_str && cmd_str) {
                double a = atof(a_str + 2);
                double b = atof(b_str + 2);
                char cmd[4];
                sscanf(cmd_str, "cmd=%3s", cmd);

                char result[256];
                solve(a, b, cmd, result);

                char response[1024];
                snprintf(response, sizeof(response), 
                         "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                         "<html><body><h1>Result: %s</h1></body></html>", result);
                send(client, response, strlen(response), 0);
            } else {
                const char *response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n"
                                       "<html><body><h1>Missing parameters</h1></body></html>";
                send(client, response, strlen(response), 0);
            }
        }
    }

    close(client);
    pthread_exit(NULL);
}

double solve(double a, double b, const char *cmd, char *result_str) {
    double result;

    if (strcmp(cmd, "add") == 0) {
        result = a + b;
        snprintf(result_str, 256, "%f + %f = %f", a, b, result);
    } else if (strcmp(cmd, "sub") == 0) {
        result = a - b;
        snprintf(result_str, 256, "%f - %f = %f", a, b, result);
    } else if (strcmp(cmd, "mul") == 0) {
        result = a * b;
        snprintf(result_str, 256, "%f * %f = %f", a, b, result);
    } else if (strcmp(cmd, "div") == 0) {
        if (b == 0) {
            snprintf(result_str, 256, "Error !!! ");
            return -1;
        }
        result = a / b;
        snprintf(result_str, 256, "%f / %f = %f", a, b, result);
    } else {
        snprintf(result_str, 256, "Error! ");
        return -1;
    }

    return result;
}
