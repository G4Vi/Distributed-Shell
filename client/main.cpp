#include "client.h"

int getSocket(string host,  string port_number, struct addrinfo **results)
{
    int tcp_socket;
    struct addrinfo socket_info;
    memset(&socket_info, 0, sizeof socket_info); //clear struct to be safe
    socket_info.ai_family = AF_UNSPEC; //compatible with IPV4 and 6
    socket_info.ai_socktype = SOCK_STREAM; //TCP
    socket_info.ai_flags  = AI_PASSIVE;

    //setup the socket
    int status = getaddrinfo(host.c_str(),port_number.c_str(),&socket_info, results);

    if(status != 0) //Check getaddrinfo worked before proceeding with bad information
    {
        printf("getaddrinfo failed!\n error: %s\n", gai_strerror(status));
        exit(1);
    }

    if ((tcp_socket = socket((*results)->ai_family, (*results)->ai_socktype, (*results)->ai_protocol)) == -1)
    {
        printf("Socket opening error\n");
        exit(1);
    }

    return tcp_socket;
}

string encryptPassword(string password, string salt) //prepare password for sending by encryption
{
    password = crypt(password.c_str(), salt.c_str());
    return password;
}

int login(int tcp_socket, string username, string password)
{
    int bytes = 0;
    char* salt;
    char message[MESSAGE_SIZE];
    memset(&message, 0, sizeof message);
    if (send(tcp_socket, username.c_str(), username.length(), 0)!= -1) //Send username
    {
        if((bytes = read(tcp_socket, message, MESSAGE_SIZE))> -1)
        {
            message[bytes] = '\0';
            salt = message;
            password = encryptPassword(password, salt);
            password += "\r\n";
            if (send(tcp_socket, password.c_str(), password.length(), 0)!= -1) //Send password
            {
                memset(&message, 0, sizeof message);
                if((bytes = read(tcp_socket, message, MESSAGE_SIZE))> -1)
                {
                    message[bytes] = '\0';
                    cout << message;
                    fflush(stdout);
                    if(strcmp(message, "Invalid username or password") != 0)
                    {
                        memset(&message, 0, sizeof message);
                        if((bytes = read(tcp_socket, message, MESSAGE_SIZE))> -1)
                        {
                            message[bytes] = '\0';
                            lineStart = message;
                        }

                        return 0;
                    }

                }
            }

        }
    }
    return -1;
}

void shell(int tcp_socket)
{
    string line;
    int bytes = 0;
    char message[MESSAGE_SIZE];
    memset(&message, 0, sizeof message);

    if(args.command == NULL)
    {
        cout << lineStart; //print user@hostname
        fflush(stdout);
        getline(cin, line); //recieve command from cin
    }
    else
    {
        //user supplied in argv
        line = args.command;
    }

    line += "\r\n"; //for compatablity with telnet
    if (send(tcp_socket, line.c_str(), line.length(), 0)!= -1) //Send input
    {
        if((bytes = read(tcp_socket, message, MESSAGE_SIZE))> 0) //print output
        {
            message[bytes] = '\0';
            cout << message;
            fflush(stdout);
        }

        cout << endl;
    }
}

void usage()
{
    cout << "Usage: client [flags], where flags are:" << endl;
    cout << "  -p #       port server is on (default is 53000)" << endl;
    cout << "  -s host    host server is on (default is 127.0.0.1)" << endl;
    cout << "  -c command command to set to server, else will be prompted." << endl;
    cout << "  -h         display this help and exit" << endl << endl;
    cout << endl<< "Program by Gavin Hayes <gahayes@wpi.edu> for CS4513 C16" << endl;

    exit( EXIT_FAILURE );
}

void setupArgs(int argc, char** argv)
{
    //getopt
    int opt = 0;
    args.usage = 0;
    args.host = NULL;
    args.port = NULL;
    args.command = NULL;

    while((opt = getopt(argc, argv, "hs:p:c:")) != -1)
    {
        switch(opt){
        case 'h':
            args.usage = 1;
            usage();
            break;
        case 's':
            args.host = optarg;
            break;
        case 'p':
            args.port = optarg;
            break;
        case 'c':
            args.command = optarg;
            break;

        default:
            //won't go here
            break;
        }
    }
}

int main(int argc, char** argv)
{
    //initialization with defaults
    string host = "127.0.0.1";
    string port_number = "53000";
    string username = "john\r\n";
    string password = "1234";
    struct addrinfo *results;

    //update from args
    setupArgs(argc, argv);
    if(args.port != NULL)
        port_number = args.port;
    if(args.host != NULL)
        host = args.host;

    int tcp_socket = getSocket(host, port_number, &results);

    if (connect(tcp_socket, results->ai_addr, results->ai_addrlen) != -1) //Check if connect succeeds
    {
        if(login(tcp_socket, username, password)==0) //try login
        {
            shell(tcp_socket);
        }
    }
    else
    {
        cerr << "Connect to '" << host << "'failed!"<<endl;
        perror("connect");
    }

    close(tcp_socket);
    freeaddrinfo(results); //Free linked list as its no longer used;

    return 0;
}

