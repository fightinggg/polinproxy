FROM gcc:12.1.0
COPY server.c /src/server.c
RUN gcc /src/server.c -o /src/polinproxy

FROM alpine:3.14
COPY --from=0 /src/polinproxy /usr/local/polinproxy/bin/polinproxy
