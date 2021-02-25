/* f20180192@hyderabad.bits-pilani.ac.in Aviral Agarwal */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define FAIL -1
#define BUFF_LEN 8192
#define SIZE 1000

int cl = 0;
char *creds;
char s[1000];
char p[1000];
int server;

int make() {
    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(p));

    if (inet_pton(AF_INET, s, &serv_addr.sin_addr) <= 0) {
        printf("\n inet_pton error occured\n");
        return 1;
    }

    if (connect(server, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Error : Connect Failed \n");
        return 1;
    }
    return server;
}

void frame_message(char *msg, char *url) {
    sprintf(msg,
            "GET http://%s HTTP/1.1\r\nUser-Agent: Mozilla/5.0 (Windows NT "
            "10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
            "Chrome/77.0.3865.90 Safari/537.36\r\nProxy-Authorization: Basic "
            "%s\r\n\r\n",
            url, creds);
}

// Takes string to be encoded as input
// and its length and returns encoded string
char *base64Encoder(char input_str[], int len_str) {
    // Character set of base64 encoding scheme
    char char_set[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    // Resultant string
    char *res_str = (char *)malloc(SIZE * sizeof(char));
    int index, no_of_bits = 0, padding = 0, val = 0, count = 0, temp;
    int i, j, k = 0;
    // Loop takes 3 characters at a time from
    // input_str and stores it in val
    for (i = 0; i < len_str; i += 3) {
        val = 0, count = 0, no_of_bits = 0;
        for (j = i; j < len_str && j <= i + 2; j++) {
            // binary data of input_str is stored in val
            val = val << 8;
            // (A + 0 = A) stores character in val
            val = val | input_str[j];
            // calculates how many time loop
            // ran if "MEN" -> 3 otherwise "ON" -> 2
            count++;
        }
        no_of_bits = count * 8;
        // calculates how many "=" to append after res_str.
        padding = no_of_bits % 3;
        // extracts all bits from val (6 at a time)
        // and find the value of each block
        while (no_of_bits != 0) {
            // retrieve the value of each block
            if (no_of_bits >= 6) {
                temp = no_of_bits - 6;
                // binary of 63 is (111111) f
                index = (val >> temp) & 63;
                no_of_bits -= 6;
            } else {
                temp = 6 - no_of_bits;

                // append zeros to right if bits are less than 6
                index = (val << temp) & 63;
                no_of_bits = 0;
            }
            res_str[k++] = char_set[index];
        }
    }
    // padding is done here
    for (i = 1; i <= padding; i++) {
        res_str[k++] = '=';
    }
    res_str[k] = '\0';
    return res_str;
}

int check_for_redirect(char *str) {
    char *ret = strstr(str, "HTTP/1.1 30");
    if (ret != NULL) {
        return 1;
    }
    return 0;
}

int seek_till_html(char *str) {
    char temp[10];
    int j;
    for (int i = 0; str[i] != '\0'; i++) {
        if (!cl) {
            char *ret = strstr(str, "Content-Length");
            ret += 16;
            int j = 0;
            while (ret[j] != '\r') {
                cl = cl * 10 + (ret[j] - '0');
                j++;
            }
        } else {
            for (j = 0; j < 4; j++) {
                temp[j] = str[i + j];
            }
            temp[j] = '\0';
            if (!strcmp(temp, "\r\n\r\n")) {
                return i + j;
            }
        }
    }
}

void get_response(char *msg, FILE *filePointer, int type) {
    cl = 0;
    server=make();
    char buff[BUFF_LEN];
    memset(buff, '0', sizeof(buff));
    write(server, msg, strlen(msg));
    int bytes;
    int x = -1;
    int total_put = 0;
    while ((bytes = read(server, buff, sizeof(buff))) > 0) {
        if (type) {
            buff[bytes] = 0;
        }
        if (x == -1) {
            int r = check_for_redirect(buff);
            if (r) {
                close(server);
                char *ret = strstr(buff, "Location");
                ret += 17;
                char nurl[1000];
                int k = 0;
                while (ret[k] != '\r') {
                    nurl[k] = ret[k];
                    k++;
                }
                nurl[k] = '\0';
                char msg1[1000];
                frame_message(msg1, nurl);
                get_response(msg1, filePointer, 1);
                return;
            }
            x = seek_till_html(buff);
        }
        if (x != -1) {
            if (type) {
                fputs(buff + x, filePointer);
            } else {
                for (int i = x; i < bytes - x; i++) {
                    fputc(buff[i], filePointer);
                }
            }
            total_put += bytes - x;
            x = 0;
        }
        if (total_put >= cl) {
            close(server);
            return;
        }
    }
    if (bytes < 0) {
        printf("\n Read error \n");
    }
    close(server);
}

int main(int argsc, char *argsv[]) {
    if (argsc < 7) {
        printf("usage: %s <hostname> <proxyip> <proxyport> <username> "
               "<password> <htmlfile> <logofile> \n",
               argsv[0]);
        exit(0);
    }
    char ch[100];
    strcpy(ch, argsv[4]);
    strcat(ch, ":");
    strcat(ch, argsv[5]);
    creds = base64Encoder(ch, strlen(ch));
    char msg1[1000];
    frame_message(msg1, argsv[1]);

    strcpy(s, argsv[2]);
    strcpy(p, argsv[3]);
    server = make();

    char msg2[1000];
    sprintf(
        msg2,
        "GET http://%s/cc.gif HTTP/1.1\r\nUser-Agent: Mozilla/5.0 (Windows NT "
        "10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/77.0.3865.90 Safari/537.36\r\nProxy-Authorization: Basic "
        "%s\r\n\r\n",
        argsv[1], creds);

    FILE *filePointer = fopen(argsv[6], "w");
    get_response(msg1, filePointer, 1);
    fclose(filePointer);
    if (!strcmp(argsv[1], "info.in2p3.fr") && argsc == 8) {
        FILE *filePointer = fopen(argsv[7], "wb");
        get_response(msg2, filePointer, 0);
        fclose(filePointer);
    }
    return 0;
}
