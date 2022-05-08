@echo off

clang++ server.cpp -o server.exe -lws2_32 -lmswsock -ladvapi32 && start server.exe
clang++ client.cpp -o client.exe -lws2_32 -lmswsock -ladvapi32 && start client.exe