#ifndef SERVER_H
#define SERVER_H

#endif // SERVER_H
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <wordexp.h>
#include <sys/wait.h>

//constants
#define HOST_NAME_MAX 256
#define MESSAGE_SIZE 1024
const char* VALIDUSERNAME = "john";
const char* VALIDPASSWORD = "1234";

using namespace std;

struct globalArgs_t
{
    int usage;          //-h
    char* port;         //-p
    char* dir;          //-d

} args;

//prototypes
string getLineStart(string username);                             //generate user@host
void launchProgram(string programName, char** argv);              //runs the command
void shell(int clientfd);                                         //handles non login in userinput and calls launchProgram
int validateLogin(string username, string password, string salt); //compare encrypted password with what is stored locally
string generateSalt();                                            //for generates a salt using pseudo random number generation
int login(int clientfd);                                          //handles logging in
int getSocket(string port_number);                                //acquires a listening socket
void child(int clientfd, int listen);                             //forks and calls shell if login successful
void usage(char* current_directory);                              //displays help message and exits
void setupArgs(int argc, char** argv, char* current_directory);   //calls getopt and sets args
int main(int argc, char** argv);
