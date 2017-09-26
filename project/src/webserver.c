
#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<time.h>
#include<limits.h>

#define DEFAULT_PORT 80
#define BUFSIZE 512
#define MAX_PORTS 65535
#define BACKLOG 10

void parameterHandling(int nrOfArgs, int* port, char* args[]) { // Function to handle input parameters
	
	char *invalidUsage = "Invalid usage!\n";
	char *usage = "USAGE: webserver [-h|-p N]\n-h for help\n-p N for setting the port the webserver should listen to\n";

	switch(nrOfArgs) {
		case 1: // No args where defined
			*port = DEFAULT_PORT;
			break;
		case 2: // One argument where defined
			if(strcmp(args[1], "-h") == 0) {
				printf("%s", usage);
				exit(0);
			}
			else {
				printf("%s%s", invalidUsage, usage);
				exit(1);
			}
			break;
		case 3: // Two arguments where defined
			if(strcmp(args[1], "-p") == 0) {
				int temp = 0; 
				sscanf(args[2], "%d", &temp); // Convert c string to int
				if(temp > 0 && temp <= MAX_PORTS)
					*port = temp;
				else {
					printf("%s%s", invalidUsage, usage);
					exit(1);
				}
			}
			else {
				printf("%s%s", invalidUsage, usage);
				exit(1);
			}
			break;
		default: // Error
			printf("%s%s", invalidUsage, usage);
			exit(1);
			break;
	}
}

void setUp(int *sd, const int port, struct sockaddr_in *sin) { // Function to set up sockets
	// Fix socket
	// sd = socket descriptor
	// socket(DOMAIN, TYPE, PROTOCOL)
	if((*sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(-1);
	}
	
	// Information about our port and ip
	memset(&(*sin), 0, sizeof(*sin)); // Fill sin memory area with the constant byte 0
	(*sin).sin_family = AF_INET; // IPv4
	(*sin).sin_addr.s_addr = INADDR_ANY; // Socket accepts connections to all the IPs of the machine
	(*sin).sin_port = htons(port); 
	
	// Bind socket
	// Bind the connection information to the socket file descriptor
	if(bind(*sd, (struct sockaddr*)&(*sin), sizeof(*sin)) == -1) {
		perror("bind");
		exit(-1);
	}
	
	// Listen to socket
	// sd = socket file descriptor of socket system call
	// BACKLOG = Number of connections allowed on incoming queue
	if(listen(*sd, BACKLOG) == -1) {
		perror("listen");
		exit(-1);
	}
}

void acceptAndRecData(int *sd, int *sd_current, struct sockaddr_in *pin, int *addrlen, char buf[BUFSIZE]) { // Function to accept and recieve data
	// Wait for client to send us an request
	// Get new socket file descriptor to use for this single connection
	// pin = space where information about where the connection is comming from
	*addrlen = sizeof(*pin);
	if((*sd_current = accept(*sd, (struct sockaddr*)&(*pin), &(*addrlen))) == -1) {
		perror("accept");
		exit(-1);
	}

	// Get a message from the client
	// sd_current = current socket file descriptor
	// buf = location to store data recived
	// sizeof(buf) = size of buffer
	// 0 = flag
	if(recv(*sd_current, buf, BUFSIZE * sizeof(char), 0) == -1) {
		perror("recv");
		exit(-1);
	}

	// Show info about the client
	printf("Request from %s:%i\n", inet_ntoa((*pin).sin_addr), ntohs((*pin).sin_port));
	printf("Request: %s", buf);
}

// Return NULL if there was an error else return current date as a string
char* getDate() { // Function to return date
	time_t currentTime;
	char *timeStr = NULL;
	char *response;
	char *dateText = "Date: ";
	
	currentTime = time(NULL); // Set time to NULL, in case of time not found
	
	if(currentTime != ((time_t)-1))
		timeStr = ctime(&currentTime); // ***
	
	response = calloc(strlen(dateText) + strlen(timeStr) + 1, sizeof(char)); // Make space in memory for response
	strcpy(response, dateText); // Set date and time in response string
	strcat(response, timeStr);	

	return response;
}

char* getFileLastModified(char *path) { // Function to return last date and time for file modified
	char *response;
	char *lastResponseText = "Last-Modified: ";
	char *temp = "File not found!\n";
	struct stat attr;
	if(stat(path, &attr) == 0) // ***
		temp = ctime(&attr.st_mtime);
	
	response = calloc(strlen(lastResponseText) + strlen(temp) + 1, sizeof(char)); // Make space in memory for response
	strcpy(response, lastResponseText); // Set two char arrays into one char array.
	strcat(response, temp);	
	
	return response;
}

char* handleGet(char* file) { // Function to handle GET command.
	
	// Variables used in the response
	char *protocol = "HTTP/1.0 ";
	char *responseCode = "200 OK\n";
	char *time = getDate();
	char *server = "Server: runs something\n"; //!!!!
	char *lastModified = "TEMP"; //!!!!
	char *eTag = "ETag: rand12345num\n"; //!!!!
	char *acceptRanges = "Accept-Ranges: bytes\n";
	char *contentLengthStart = "Content-Length: %d\n";
	char *contentLength;
	char *connection = "Connection: close\n";
	char *contentType = "Content-Type: text/html\n";
        char *response;
	
	char *page;
	char *pathToWWW = "../www/";

	FILE *fp = NULL;
	size_t fileSize = 0;
	char *pathToRequestedFile;
	char *fileName = calloc(strlen(file), sizeof(char));
	
	realpath(file, fileName); // Check for .. or .

	if(strcmp(fileName, "/")==0)
		fileName = "index.html";
	else if(fileName[0] = '/')
		fileName++;


	pathToRequestedFile = calloc(strlen(pathToWWW) + strlen(fileName) + 1, sizeof(char)); // Make space in memory for file path string
	strcpy(pathToRequestedFile, pathToWWW); // Set two char arrays into one, filepath+filename
	strcat(pathToRequestedFile, fileName);
	
	printf("open file in path: %s\n", pathToRequestedFile);
	fp = fopen(pathToRequestedFile, "r"); // Open file from recieved path and filename string
	
	if(fp == NULL) { 
		fp = fopen("../www/error404.html", "r");	
		responseCode = "404 Not Found\n";
	}

	if(fp == NULL) { 
		fp = fopen("../www/error500.html", "r");	
		responseCode = "500 Internal Server Error\n";
	}

	if(fp != NULL) {
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp); // Set filesize to size of opened file
		
		rewind(fp);
		page = calloc(fileSize + 1, sizeof(char)); // Allocate space in memory for file
		
		fread(page, fileSize, 1, fp); // Set variable page to opened files content data
		page[fileSize] = '\0';

		fclose(fp); // Close opened file
	}
	else {
		page = "<html><head>Internal server error 500</head><body><h1>Internal server error 500</h1></body></html>";
		responseCode = "500 Internal Server Error\n";
	}

	lastModified = getFileLastModified(pathToRequestedFile); // Set value for last modified with function
	int pageSize = strlen(page);
	contentLength = calloc(strlen(contentLengthStart) + sizeof(pageSize) + 1, sizeof(char));
	//strcpy(contentLength, contentLengthStart);
	//strcat(contentLength, 
	sprintf(contentLength, contentLengthStart, pageSize);

	response = calloc(	strlen(protocol) +
				strlen(responseCode) + 
				strlen(time) +
				strlen(server) +
				strlen(lastModified) +
				strlen(eTag) +
				strlen(acceptRanges) +
				strlen(contentLength) +
				strlen(connection) + 
				strlen(contentType) +
				strlen("\n") +
				strlen(page) +
				1, 
				sizeof(char)); // Allocate space in memory for a response to send back

	strcpy(response, protocol);
	strcat(response, responseCode);
	strcat(response, time);
	strcat(response, server);
	strcat(response, lastModified);
	strcat(response, eTag);
	strcat(response, acceptRanges);
	strcat(response, contentLength);
	strcat(response, connection);
	strcat(response, contentType);
	strcat(response, "\n");
	strcat(response, page); // Set all different parts of the response to one char array
		
	return response;
}

char* handleBuf(char buf[BUFSIZE]) { // Function to handle recieved data
	// Variables used 
	char *token;
	
	//char *temp = strdup(buf);
	char *temp = calloc(strlen(buf) + 1, sizeof(char));
	strcpy(temp, buf);
	
	const char *delim = " \0";
	token = strtok(temp, delim);
	
	char *response;
	
	if(token != NULL && *token != '\r') {
		// WIP
		// Implement function handling head and return response code
		if(strcmp(token, "HEAD") == 0) {
			response = "HEAD\n"; 
		}
		// WIP
		// Implement function handling get and return response code
		else if(strcmp(token, "GET") == 0) {
			token = strtok(NULL, delim); // Set token to next part of char array, filename
			//token[strlen(token) - 2] = '\0'; // Remove junk from char array
			
			response = handleGet(token); // Call handleget and set the response to var response
		}
		else {
			response = "ERROR 501\n"; // If command not implemented, return error 501
		}
	}
	else{
		response = "ERROR 400\n"; // If something else went wrong, return error 400
	}
	
	printf("Response message:\n%s\n", response);

	return response;
}

int main(int argc, char* argv[]) {
	// Variables used
	int port = 0;
	struct sockaddr_in sin, pin;
	int sd, sd_current;
	int addrlen;
	char buf[BUFSIZE];
	char *response = "No response";
	
	parameterHandling(argc, &port, argv); // Call function to handle parameters entered when starting the webserver

	printf("Listening to port: %d\n", port); // Showing what port has been set to listen to
	
	setUp(&sd, port, &sin);	// Set up the socket to listen for requests
	
	// Figure out where to fork what, suggestion, fork in start of while
	while(1) {
		for(int i = 0; i < BUFSIZE; i++) { // Remove junk from buffer
			buf[i] = '\0';
		}				
		acceptAndRecData(&sd, &sd_current, &pin, &addrlen, buf); // Call function to handle recieved data
		response = handleBuf(buf); // Call function to handle a response
	
		// Send a response to the client
		if(send(sd_current, response, strlen(response) + 1, 0) == -1) { // Send a response to client, if it does not work, show error.
			perror("send");
			exit(-1);
		}
	
		// Close current socket
		close(sd_current); // Close current socket
	}	
	
	// Close socket
	close(sd);
	
	return 0;
}
