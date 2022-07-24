FROM gcc:12.1.0
COPY server.c /src/server.c
RUN gcc /src/server.c -o /src/polinproxy -lpthread #--static

FROM centos:centos8
COPY --from=0 /src/polinproxy /usr/local/polinproxy/bin/polinproxy
