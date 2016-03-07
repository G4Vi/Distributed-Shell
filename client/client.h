#ifndef CLIENT_H
#define CLIENT_H

#endif // CLIENT_H
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#define MESSAGE_SIZE 1024
using namespace std;
string lineStart;

struct globalArgs_t
{
    int usage;          //-h
    char* port;         //-p
    char* host;         //-s
    char* command;

} args;

int getSocket(string host,  string port_number, struct addrinfo **results);  //gets socket for connecting
string encryptPassword(string password, string salt);                        //uses salt to protect password over network
int login(int tcp_socket, string username, string password);                 //handles logging in
void shell(int tcp_socket);                                                  //after logging in it uses the command supplied or prompts for command
void usage();                                                                //displays help message and exits
void setupArgs(int argc, char** argv);                                       //calls getopt and sets args
int main(int argc, char** argv);
