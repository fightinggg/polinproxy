//服务器端
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#define bool int
#define debug

bool isvisiable(char c) {
    if ('0' <= c && c <= '9') {
        return 1;
    }
    if ('a' <= c && c <= 'z') {
        return 1;
    }
    if ('A' <= c && c <= 'Z') {
        return 1;
    }
    return 0;
}

void showBinary(char *prefix, char *s, unsigned long len) {
#ifdef debug
    printf("%s size=%d\n", prefix, len);
    int rowsize = 24;
    for (int row = 0; row * rowsize < len; row++) {
        printf("%s ", prefix);
        for (int i = 0; i < rowsize; i++) {
            if (row * rowsize + i < len) {
                printf("0x%03d ", s[row * rowsize + i]);
            } else {
                printf("      ");
            }
        }

        for (int i = 0; i < rowsize; i++) {
            if (row * rowsize + i < len) {
                printf("%c", isvisiable(s[row * rowsize + i]) ? s[row * rowsize + i] : '.');
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    printf("\n");
    fflush(stdout);
#endif
}

void readMore(int fd) {
    char buff[128] = {0};
    long size = read(fd, buff, 128);
    showBinary(">", buff, size);
}

int dns(const char *domain, char *ipaddr) {

    char **pptr;
    char IP[32];
    struct hostent *hptr;

    if ((hptr = gethostbyname(domain)) == NULL) {
        printf("gethostbyname error for host:%s\n", domain);
        return 0;
    }

//    printf("official hostname:%s\n", hptr->h_name);
//    for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
//        printf("alias:%s\n", *pptr);

    switch (hptr->h_addrtype) {
        case AF_INET:
        case AF_INET6:
            pptr = hptr->h_addr_list;
            for (; *pptr != NULL; pptr++) {
                sprintf(ipaddr, "%s", inet_ntop(hptr->h_addrtype, *pptr, IP, sizeof(IP)));
                return 0;
            }
            break;
        default:
            printf("unknown address type\n");
            break;
    }

    return 0;
}

void init(int fd) {
    /*
     *  The client connects to the server, and sends a version
     *    identifier/method selection message:
     *
     *                    +----+----------+----------+
     *                    |VER | NMETHODS | METHODS  |
     *                    +----+----------+----------+
     *                    | 1  |    1     | 1 to 255 |
     *                    +----+----------+----------+
     *
     *    The VER field is set to X'05' for this version of the protocol.  The
     *    NMETHODS field contains the number of method identifier octets that
     *    appear in the METHODS field.
     *    客户端发起请求，第一个字节一定是0x5,第二个字节 nmethods 代表支持的 method 的数量
     *    跟着的是 nmethods 个method，每个method一个字节
     */
    char buff[128] = {0};
    long size = read(fd, buff, 128);
    showBinary(">", buff, size);

    /*
     *  The server selects from one of the methods given in METHODS, and
     *    sends a METHOD selection message:
     *
     *                          +----+--------+
     *                          |VER | METHOD |
     *                          +----+--------+
     *                          | 1  |   1    |
     *                          +----+--------+
     *
     *    If the selected METHOD is X'FF', none of the methods listed by the
     *    client are acceptable, and the client MUST close the connection.
     *
     *    The values currently defined for METHOD are:
     *
     *           o  X'00' NO AUTHENTICATION REQUIRED
     *           o  X'01' GSSAPI
     *           o  X'02' USERNAME/PASSWORD
     *           o  X'03' to X'7F' IANA ASSIGNED
     *           o  X'80' to X'FE' RESERVED FOR PRIVATE METHODS
     *           o  X'FF' NO ACCEPTABLE METHODS
     *
     *   这里直接回复 0x05 0x00 不进行鉴权
     */
    char wbuf[2] = {5, 0};
    write(fd, wbuf, 2);
    showBinary("<", wbuf, 2);
}

int parseTarget(int fd, char *host, int *port) {
    /*
     *  The SOCKS request is formed as follows:
     *
     *         +----+-----+-------+------+----------+----------+
     *         |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
     *         +----+-----+-------+------+----------+----------+
     *         | 1  |  1  | X'00' |  1   | Variable |    2     |
     *         +----+-----+-------+------+----------+----------+
     *
     *      Where:
     *
     *           o  VER    protocol version: X'05'
     *           o  CMD
     *              o  CONNECT X'01'
     *              o  BIND X'02'
     *              o  UDP ASSOCIATE X'03'
     *           o  RSV    RESERVED
     *           o  ATYP   address type of following address
     *              o  IP V4 address: X'01'
     *              o  DOMAINNAME: X'03'
     *              o  IP V6 address: X'04'
     *           o  DST.ADDR       desired destination address
     *           o  DST.PORT desired destination port in network octet
     *              order
     *
     *    In an address field (DST.ADDR, BND.ADDR), the ATYP field specifies
     *    the type of address contained within the field:
     *
     *           o  X'01'
     *
     *    the address is a version-4 IP address, with a length of 4 octets
     *
     *           o  X'03'
     *
     *    the address field contains a fully-qualified domain name.  The first
     *    octet of the address field contains the number of octets of name that
     *    follow, there is no terminating NUL octet.
     *
     *           o  X'04'
     *
     */
    char buff[128] = {0};
    long size = read(fd, buff, 128);
    showBinary(">", buff, size);


    if (buff[3] == 1) { // IP

    } else if (buff[3] == 3) { // domain
        int hostlen = buff[4] & 0xff;
        for (int i = 0; i < hostlen; i++) {
            host[i] = buff[5 + i];
        }
        host[hostlen] = 0;
        *port = (buff[5 + hostlen] & 0xff) * 256 + (buff[5 + hostlen + 1] & 0xff);
    } else {
        return -1;
    }

//    printf("proxy to [%s:%d]\n", host, *port);
//    fflush(stdout);

    /*
     * The SOCKS request information is sent by the client as soon as it has
     *    established a connection to the SOCKS server, and completed the
     *    authentication negotiations.  The server evaluates the request, and
     *    returns a reply formed as follows:
     *
     *         +----+-----+-------+------+----------+----------+
     *         |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
     *         +----+-----+-------+------+----------+----------+
     *         | 1  |  1  | X'00' |  1   | Variable |    2     |
     *         +----+-----+-------+------+----------+----------+
     *
     *      Where:
     *
     *           o  VER    protocol version: X'05'
     *           o  REP    Reply field:
     *              o  X'00' succeeded
     *              o  X'01' general SOCKS server failure
     *              o  X'02' connection not allowed by ruleset
     *              o  X'03' Network unreachable
     *              o  X'04' Host unreachable
     *              o  X'05' Connection refused
     *              o  X'06' TTL expired
     *              o  X'07' Command not supported
     *              o  X'08' Address type not supported
     *              o  X'09' to X'FF' unassigned
     *           o  RSV    RESERVED
     *           o  ATYP   address type of following address
     *              o  IP V4 address: X'01'
     *              o  DOMAINNAME: X'03'
     *              o  IP V6 address: X'04'
     *           o  BND.ADDR       server bound address
     *           o  BND.PORT       server bound port in network octet order
     *
     *    Fields marked RESERVED (RSV) must be set to X'00'.
     *
     *    If the chosen method includes encapsulation for purposes of
     *    authentication, integrity and/or confidentiality, the replies are
     *    encapsulated in the method-dependent encapsulation.
     *
     */

    char connectResult[] = {5, 0, 0, 1, 192, 168, 1, 1, 0, 80};
    write(fd, connectResult, 10);
    showBinary("<", connectResult, 10);

}

bool proxy(int sourceFd, char *host, int port) {

    // connect to target
    int targetFd = 0;
    struct sockaddr_in Data_buf;
    int len = sizeof(Data_buf);
    //建立监听套接字
    targetFd = socket(AF_INET, SOCK_STREAM, 0);

    //绑定IP地址和端口号
    Data_buf.sin_family = AF_INET;
    Data_buf.sin_port = htons(port);
    Data_buf.sin_addr.s_addr = inet_addr(host);
    //建立链接
    int ret = connect(targetFd, (struct sockaddr *) &Data_buf, len);
    if (ret == -1) {
        perror("connect error");
        return 0;
    }

    fcntl(sourceFd, F_SETFL, fcntl(sourceFd, F_GETFL) | O_NONBLOCK);
    fcntl(targetFd, F_SETFL, fcntl(targetFd, F_GETFL) | O_NONBLOCK);

    int sopen = 1, copen = 1;

    while (sopen || copen) {
        static const int bufsize = 4096;
        char buff[4096] = {0};
        long size = 0;

        size = read(sourceFd, buff, bufsize);
        if (size > 0) {
            showBinary("client>", buff, size);
            write(targetFd, buff, size);
            showBinary("target<", buff, size);
        } else if (size == 0) {
            close(sourceFd);
            close(targetFd);
            sopen = 0;
            copen = 0;
        }
//        printf("read from source size=%d\n", size);

        size = read(targetFd, buff, bufsize);
        if (size > 0) {
            showBinary("target>", buff, size);
            write(sourceFd, buff, size);
            showBinary("client<", buff, size);
        } else if (size == 0) {
            close(targetFd);
            close(sourceFd);
            copen = 0;
            sopen = 0;
        }
//        printf("read from target size=%d\n", size);

        usleep(10 * 1000); // 10 ms

    }


    return 1;
}

void *processConnect(void *args) {
    int *fdp = args;

    int fd = *fdp;
    init(fd);
    // 解析target
    char host[256];
    int port;
    parseTarget(fd, host, &port);

    dns(host, host);
//    printf("proxy to [%s:%d]\n", host, port);

    proxy(fd, host, port);

    printf("close connect with fd=%d\n", fd);
}

bool strPrefixEq(char *a, char *b, int len) {
    if (strlen(a) < len || strlen(b) < len) {
        return 0;
    }
    for (int i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

char usage[] = "--help        : show this usage\n--port=<port> : set proxy listening port\n";

int main(int args, char **argc) {
    int listenPort = 0;
    for (int i = 1; i < args; i++) {
        if (strPrefixEq("--port=", argc[i], 7)) {
            listenPort = 0;
            for (int j = 7; j < strlen(argc[i]); j++) {
                if (argc[i][j] < '0' || argc[i][j] > '9') {
                    printf("could not parse: [%s]", argc[i]);
                    return -1;
                }
                listenPort = listenPort * 10 + argc[i][j] - '0';
            }
        } else if (strPrefixEq("--help", argc[i], 6)) {
            printf("%s", usage);
            return 0;
        } else {
            printf("could not parse: [%s] ,please use flat: --help ", argc[i]);
            return -1;
        }
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd) {
        perror("socket error");
        exit(-1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (listenPort == 0) {
        listenPort = 1080;
    }
    server_addr.sin_port = htons(listenPort);
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    socklen_t addrlen = sizeof(server_addr);

    if (-1 == bind(sockfd, (struct sockaddr *) &server_addr, addrlen)) {
        perror("bind error");
        exit(-1);
    }

    if (-1 == listen(sockfd, 5)) {
        perror("listen error");
        exit(-1);
    }
    printf("listening on 0.0.0.0:%d\n", listenPort);


    while (1) {
        //定义一个结构体，保存客户端的信息
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));//清空
        socklen_t clientaddrlen = sizeof(client_addr);

        //5.阻塞等待客户端连接
        int acceptfd = accept(sockfd, (struct sockaddr *) &client_addr, &clientaddrlen);
        if (-1 == acceptfd) {
            perror("accept error");
        }

        printf("connect with %s:%d \n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t th1;
        pthread_create(&th1, NULL, processConnect, &acceptfd);
    }

    printf("server close...");

    //7.关闭套接字
    close(sockfd);

    return 0;
}