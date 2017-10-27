
// DV1467 Lab assignment 2
// Authors: Peter Aoun & Per Sandgren

#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
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

// Function that handles the parameters
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

// Function that sets up the socket
void setUp(int *sd, const int port, struct sockaddr_in *sin) { 
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

// Function to accept and recieve data
int acceptAndRecData(int *sd, int *sd_current, struct sockaddr_in *pin, int *addrlen, char buf[BUFSIZE]) { 
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
	//printf("Request from %s:%i\n", inet_ntoa((*pin).sin_addr), ntohs((*pin).sin_port));
	//printf("Request: %s", buf);

	return 1;
}

// Return NULL if there was an error else return current date as a string
char* getDate() { // Function to return date
	time_t currentTime;
	char *timeStr = NULL;
	char *response;
	char *dateText = "Date: ";
	
	currentTime = time(NULL); // Set time to NULL, in case of time not found it will return -1
	

	if(currentTime != ((time_t)-1)) {
		timeStr = ctime(&currentTime); // Convert time to string
	}
	
	response = calloc(strlen(dateText) + strlen(timeStr) + 1, sizeof(char)); // Make space in memory for response

	strcpy(response, dateText); // Set date and time in response string
	strcat(response, timeStr);

	// Add \r\n to the end
	response[strlen(response) - 1] = '\r';
	response[strlen(response)] = '\n';
	
	return response;
}

char* getFileLastModified(char *path) { // Function to return last date and time for file modified
	char *response;
	char *lastResponseText = "Last-Modified: ";
	char *temp = "File not found!";
	struct stat attr;

	if(stat(path, &attr) == 0) // Check that the file was found
		temp = ctime(&attr.st_mtime); // Set the last modified date and time to temp
	
	response = calloc(strlen(lastResponseText) + strlen(temp) + 1, sizeof(char)); // Make space in memory for response
	strcpy(response, lastResponseText);
	strcat(response, temp);	
	
	// Add \r\n to the end
	response[strlen(response) - 1] = '\r';
	response[strlen(response)] = '\n';
	
	return response;
}

char* handleRequest(char *requestType, char* file) { // Function to handle GET command.
	
	// Variables used in the response
	char *protocol = "HTTP/1.0 ";
	char *responseCode = "400 Bad Request\r\n";
	char *time = getDate();
	char *server = "Server: Ubuntu 16.04\r\n"; //!!!!
	char *lastModified = "TEMP"; //!!!!
	char *eTag = "ETag: rand12345num\r\n"; //!!!!
	char *acceptRanges = "Accept-Ranges: bytes\r\n";
	char *contentLengthStart = "Content-Length: %d\r\n";
	char *contentLength;
	char *connection = "Connection: close\r\n";
	char *contentType = "Content-Type: text/html\r\n";
        char *response;
	
	char *page;
	char *pathToWWW = "../www/";

	FILE *fp = NULL;
	size_t fileSize = 0;
	char *pathToRequestedFile;
	char *fileName;

	// Check if the request type is too long
	if(strlen(requestType) > 5) {
		fp = fopen("../www/error400.html", "r");
		lastModified = getFileLastModified("../www/error400.html"); // Set value for last modified with function	
	} // Check if the requested file name is too long 
	else if(strlen(file) > 50) {
		fp = fopen("../www/error403.html", "r");
		responseCode = "403 Forbidden\n";
		lastModified = getFileLastModified("../www/error403.html"); // Set value for last modified with function	
	} // Check if the request type is HEAD or GET
	else if(strcmp(requestType, "HEAD") == 0 || strcmp(requestType, "GET") == 0) {
		
		fileName = calloc(strlen(file), sizeof(char));

		realpath(file, fileName); // Remove .. and ./ in the filename
	
		// If file name is only a /, set it to index.html
		if(strcmp(fileName, "/")==0)
			fileName = "index.html";
		else if(fileName[0] = '/') // If the file name starts with a /, move the pointer forward
			fileName++;

		pathToRequestedFile = calloc(strlen(pathToWWW) + strlen(fileName) + 1, sizeof(char)); // Make space in memory for file path string
		strcpy(pathToRequestedFile, pathToWWW); // Set two char arrays into one, filepath+filename
		strcat(pathToRequestedFile, fileName);

		fp = fopen(pathToRequestedFile, "r"); // Open file from recieved path and filename string	
		responseCode = "200 OK\n";
		lastModified = getFileLastModified(pathToRequestedFile); // Set value for last modified with function	
		
		// Check if file not found
		if(fp == NULL) { 
			fp = fopen("../www/error404.html", "r");	
			responseCode = "404 Not Found\n";
			lastModified = getFileLastModified("../www/error404.html"); // Set value for last modified with function	
		}
	} // Request type not implemented
	else { 
		fp = fopen("../www/error501.html", "r");
		responseCode = "501 Not Implemented\n";
		lastModified = getFileLastModified("../www/error501.html"); // Set value for last modified with function	
	}	

	// If file could not be open
	if(fp == NULL) { 
		fp = fopen("../www/error500.html", "r");	
		responseCode = "500 Internal Server Error\n";
		lastModified = getFileLastModified("../www/error500.html"); // Set value for last modified with function
	}

	// Get file content if a file was abled to be opened
	if(fp != NULL) {
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp); // Set filesize to size of opened file
		
		rewind(fp);
		page = calloc(fileSize + 1, sizeof(char)); // Allocate space in memory for file
		
		fread(page, fileSize, 1, fp); // Set variable page to opened files content data
		page[fileSize] = '\0';

		fclose(fp); // Close opened file
	}
	else { // No file could be opened
		page = "<html><head>Internal server error 500</head><body><h1>Internal server error 500</h1></body></html>";
		responseCode = "500 Internal Server Error\n";
	}
	
	// Only get HEAD part of html file 
	if(strcmp(requestType, "HEAD") == 0) {	
		// Get p1 to the start of head part and p2 to the end of head part
		char *head = NULL;
		char *p1 = strstr(page, "<head>");
		if(p1 == NULL)
			p1 = strstr(page, "<HEAD>");
	
		char *p2 = strstr(page, "</head>");
		if(p2 == NULL)
			p2 = strstr(page, "</HEAD>");
		p2 += 7;

		if(p1 == NULL || p2 == NULL) { // No head found
			page = calloc(strlen("<head>No head found</head>"), sizeof(char));
			page = "<head>No head found</head>";
		}
		else {
			// Get the content in head and save it in page
			size_t headLength = p2 - p1;

			head = calloc(headLength + 1, sizeof(char));
			strncpy(head, p1, headLength);
			head[headLength] = '\0';
			
			page = calloc(strlen(head), sizeof(char));
			page = head;
		}
	}
	
	// Calculate content length
	int pageSize = strlen(page);
	contentLength = calloc(strlen(contentLengthStart) + sizeof(pageSize) + 1, sizeof(char));
	sprintf(contentLength, contentLengthStart, pageSize);

	// Allocate memory for the response memory
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
				strlen("\r\n") +
				strlen(page) +
				1, 
				sizeof(char));

	// Copy and concatenate all the variables into response
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
	strcat(response, "\r\n");
	strcat(response, page); // Set all different parts of the response to one char array
		
	return response;
}

char* handleBuf(char buf[BUFSIZE]) { // Function to handle recieved data

	//printf("Request: %s\n", buf);
	
	int found = 0;
	for(int i = 0; i < BUFSIZE && found == 0; i++){
		if(buf[i] == '\r'){
			buf[i] = ' ';
			found = 1;
		}
	}

	char *temp = calloc(strlen(buf) + 1, sizeof(char));
	strcpy(temp, buf);
	
	// Get request type and requested file name
	const char *delim = " \0";
	char *requestType = strtok(temp, delim);	
	char *fileName = strtok(NULL, delim);
	char *response = handleRequest(requestType, fileName);

	//printf("Response message:\n%s\n", response);

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
	int pid = getpid();	
	int status;


	parameterHandling(argc, &port, argv); // Call function to handle parameters entered when starting the webserver

	printf("Listening to port: %d\n", port); // Showing what port has been set to listen to
	
	setUp(&sd, port, &sin);	// Set up the socket to listen for requests
	
	for(int i = 0; i < BUFSIZE; i++) { // Remove junk from buffer
		buf[i] = '\0';
	}

	// Main server loop
	while(pid != 0) {
		
		pid = fork();
		switch(pid) {
		case -1: // Error forking
			perror("fork");
			exit(-1);
			break;

		case 0: // Child that will create another child
			if(acceptAndRecData(&sd, &sd_current, &pin, &addrlen, buf)) {
				if(!fork()) { // Child that handles the response
					sleep(1); // Sleep to be adopted by init when parent die
					
					response = handleBuf(buf); // Get response message
					
					// Send response message
					if(send(sd_current, response, strlen(response) + 1, 0) == -1){
						perror("send");
						close(sd_current);
						exit(-1);
					}

					close(sd_current); // Close socket
				}
				exit(0);
			}
			break;
		default: // Parent waiting for the child to create another child and die
			waitpid(pid, &status, 0);
		}
	}

	// Close socket and exit if the parent escapes somehow
	close(sd);	
	exit(0);	
}
