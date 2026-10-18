#include "blend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __wasi__
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

static int send_all(int fd, const char *data, size_t size) {
    while (size) {
        ssize_t n = send(fd, data, size, MSG_NOSIGNAL);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return 0;
        data += n; size -= (size_t)n;
    }
    return 1;
}

static int serve(void) {
    int server = socket(AF_INET, SOCK_STREAM, 0), one = 1;
    if (server < 0) return 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(8290);
    if (bind(server, (struct sockaddr *)&address, sizeof address) || listen(server, 32)) {
        perror("blend_lab bind/listen"); close(server); return 1;
    }
    fprintf(stderr, "blend_lab local listener 127.0.0.1:8290\n");
    for (;;) {
        int client = accept(server, NULL, NULL);
        if (client < 0) { if (errno == EINTR) continue; break; }
        struct timeval timeout = {.tv_sec = 5, .tv_usec = 0};
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
        char line[2048] = {0}, body[2048], header[256]; size_t used = 0;
        while (used + 1 < sizeof line) {
            ssize_t n = recv(client, line + used, sizeof line - used - 1, 0);
            if (n <= 0) break;
            used += (size_t)n; line[used] = '\0';
            if (strchr(line, '\n')) break;
        }
        blend_answer(line, body, sizeof body);
        int n = snprintf(header, sizeof header, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", strlen(body));
        if (n > 0 && send_all(client, header, (size_t)n)) send_all(client, body, strlen(body));
        close(client);
    }
    close(server); return 1;
}
#endif

int main(int argc, char **argv) {
#ifndef __wasi__
    if (argc == 2 && !strcmp(argv[1], "--serve")) return serve();
#else
    (void)argc; (void)argv;
#endif
    char line[2048] = {0}, output[2048];
    if (!fgets(line, sizeof line, stdin)) line[0] = '\0';
    blend_answer(line, output, sizeof output);
    puts(output);
    return 0;
}
