#define _XOPEN_SOURCE
#include "server.h"

string getLineStart(string username) //generate user@host
{
    string linestart;
    char hostname[HOST_NAME_MAX+1];
    gethostname(hostname, HOST_NAME_MAX);
    linestart = username + "@";
    linestart += hostname;
    linestart += ":~$ ";
    return linestart;
}

void launchProgram(string programName, char** argv) //runs the command
{
    int status;
    pid_t pid = fork();
    if(pid == 0)
    {
        if(execvp(programName.c_str(), argv) == -1)
        {
            perror("execvp");
        }        
    }
    else if(pid > 0)
    {
        //parent process
        while (wait(&status) != pid)
            ;
    }
    else
    {
        cerr << "Cannot create child process.";
        perror("fork");
    }
}

void shell(int clientfd) //handles non login in userinput and calls launchProgram
{
    string programName, line;
    wordexp_t words;
    char** argv;    
    int bytes = 0;
    char message[MESSAGE_SIZE];
    memset(&message, 0, sizeof message); //start with no message
    string linestart = getLineStart("john");
    string welcomeMessage = "Welcome to Distributed Shell\r\n";
    string fullMessage = welcomeMessage + linestart;

    //redirect stdout and stderr
    int stdout_dupdfd;
    stdout_dupdfd = dup(1);
    dup2(clientfd, STDOUT_FILENO);
    dup2(clientfd, STDERR_FILENO);

    //send initial info packet
    cout << fullMessage;
    fflush(stdout);

    //unredirect stout
    dup2(stdout_dupdfd, 1);
    close(stdout_dupdfd);


    if((bytes = read(clientfd, message, MESSAGE_SIZE))> 0) //wait for command
    {
        //parse command
        message[bytes] = '\0';
        line = message;
        line = line.substr(0, line.find_first_of("\r\n"));
        programName = line.substr(0, line.find_first_of(' '));
        wordexp(line.c_str(), &words, 0);
        argv = words.we_wordv;


        cout << "command: " << line << endl;
        cout << "executing command..." << endl << endl;

        //reredirect stdout
        dup2(clientfd, STDOUT_FILENO);
        close(clientfd);

        launchProgram(programName, argv); //call exec

        //clean
        wordfree(&words);
    }


}

//compare encrypted password with what is stored locally
int validateLogin(string username, string password, string salt)
{
    if(strcmp(VALIDUSERNAME, username.c_str())== 0)
    {
        string encryptedPassword = crypt(VALIDPASSWORD, salt.c_str());
        if(strcmp(encryptedPassword.c_str(), password.c_str()) == 0 )
        {
            cout << "password ok" << endl;
            return 0;
        }
    }
    return -1;
}

string generateSalt() //for encryption
{
    srand (time(NULL)); // initialize seed
    int i = rand();
    char fullSalt[65];  //ready for 64bit int if neccessary
    sprintf(fullSalt, "%d", i);
    string salt = fullSalt;
    return salt;
}

int login(int clientfd)
{
    char message[MESSAGE_SIZE];
    memset(&message, 0, sizeof message);               //start with no message
    string username, password, salt;

    int bytes = read(clientfd, message, MESSAGE_SIZE); //read username
    if(bytes > -1)
    {
        message[bytes] = '\0';
        cout << "received: " << message;
        fflush(stdout);
        username = message;

        salt = generateSalt();
        send(clientfd, salt.c_str(), 2, 0);            //send secret number
        bytes = read(clientfd, message, MESSAGE_SIZE); //read password
        if(bytes > -1)
        {
            message[bytes] = '\0';
            password = message;

            //validate login details
            username = username.substr(0, username.find_first_of("\r\n"));
            password = password.substr(0, password.find_first_of("\r\n"));

            if(validateLogin(username, password, salt) == 0)
            {
                return 0;
            }
        }
    }
    return -1;
}

int getSocket(string port_number)
{
    int tcp_socket;
    struct addrinfo socket_info, *results;
    memset(&socket_info, 0, sizeof socket_info); //clear struct to be safe
    socket_info.ai_family = AF_UNSPEC; //compatible with IPV4 and 6
    socket_info.ai_socktype = SOCK_STREAM; //TCP
    socket_info.ai_flags  = AI_PASSIVE;

    //setup the socket
    int status = getaddrinfo(NULL,port_number.c_str(),&socket_info, &results);

    if(status != 0) //Check getaddrinfo worked before proceeding with bad information
    {
        printf("getaddrinfo failed!\n error: %s\n", gai_strerror(status));
        exit(1);
    }

    if ((tcp_socket = socket(results->ai_family, results->ai_socktype, results->ai_protocol)) == -1)
    {
        printf("Socket opening error\n");
        perror("socket");
        exit(1);
    }   

    if(bind(tcp_socket, results->ai_addr, results->ai_addrlen) < 0)
    {
        printf("Socket binding error\n");
        perror("bind");
        exit(1);
    }

    if (listen(tcp_socket, 20) != 0)
    {
        printf("Socket listen error\n%s\n", strerror(errno));
        exit(1);
    }

    freeaddrinfo(results); //Free linked list as its no longer used;
    return tcp_socket;
}

void child(int clientfd, int listen)
{
    pid_t pid = fork();
    if(pid == 0)
    {
        //child
        close(listen);
        cout << "forked child" << endl;
        if(login(clientfd) == 0)
        {
            shell(clientfd);
        }       
    }
    else if(pid > 0)
    {
        //parent process
    }
    else
    {
        cerr << "Cannot create child process.";
        perror("fork");
    }
}

void usage(char* current_directory)
{
    cout << "Usage: server [flags], where flags are:" << endl;
    cout << "  -p #      port to serve on (default is 53000)" << endl;
    cout << "  -d dir    directory to serve out of (default is " << current_directory << ")" << endl;
    cout << "  -h        display this help and exit" << endl << endl;
    cout << endl<< "Program by Gavin Hayes <gahayes@wpi.edu> for CS4513 C16" << endl;

    free(current_directory);
    exit( EXIT_FAILURE );
}

void setupArgs(int argc, char** argv, char* current_directory)
{
    //getopt
    int opt = 0;
    args.usage = 0;
    args.port = NULL;
    args.dir = NULL;

    while((opt = getopt(argc, argv, "hd:p:")) != -1)
    {
        switch(opt){
        case 'h':
            args.usage = 1;
            usage(current_directory);
            break;
        case 'd':
            args.dir = optarg;
            break;
        case 'p':
            args.port = optarg;
            break;

        default:
            //won't go here
            break;
        }
    }
}


int main(int argc, char** argv)
{
    //initialization
    string port_number = "53000";
    char* current_directory = get_current_dir_name();
    struct sockaddr_in client_addr;
    socklen_t addrlen =sizeof(client_addr);

    //adjust operation from command line arguments
    setupArgs(argc, argv, current_directory);
    if(args.port != NULL)
        port_number = args.port;

    if(args.dir != NULL)
    {
        if(chdir(args.dir)== -1)
        {
            perror("chdir");
            free(current_directory);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        args.dir = current_directory;
    }

    int* clientfd = (int *)malloc(sizeof(int));
    *clientfd = -1;

    //Print initial info
    cout << argv[0] << " activating." << endl;
    cout << "    directory: " << args.dir << endl;
    free(current_directory);
    cout << "    port: " << port_number << endl;
    int tcp_socket = getSocket(port_number);
    cout << "Socket created! Accepting connections." << endl << endl;
    cout.flush();

    //Loop forever
    for(;;)
    {
        *clientfd = accept(tcp_socket, (struct sockaddr*)&client_addr, &addrlen); //accept connection
        if(*clientfd != -1)
        {
            cout << "Connection request recieved." << endl;
            child(*clientfd, tcp_socket);           
            close(*clientfd); //parent doesn't need this anymore           
        }
        waitpid(-1, NULL, WNOHANG); //collect zombies
    }
    free(clientfd);

    return 0;
}

