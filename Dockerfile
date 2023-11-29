FROM --platform=arm64 ubuntu:22.04

RUN apt update -y
RUN apt install build-essential -y
RUN apt install gcc g++ -y

WORKDIR /app
COPY . .
RUN make clean
RUN make

CMD ["./build/server", "80", "32", "./logs/serv.log"]