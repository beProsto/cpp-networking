@echo off

clang++ -std=c++17 server.cpp -o server.exe -lws2_32 -lmswsock -ladvapi32 && start server.exe
clang++ -std=c++17 client.cpp -o client.exe -lws2_32 -lmswsock -ladvapi32 && start client.exe