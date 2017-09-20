
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


#define DEFAULT_PORT 80
#define BUFSIZE 512
#define MAX_PORTS 65535
#define BACKLOG 10

void parameterHandling(int nrOfArgs, int* port, char* args[]) {
	
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

void acceptAndRecData(int *sd, int *sd_current, struct sockaddr_in *pin, int *addrlen, char buf[BUFSIZE]) {
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
	printf("Accept and Recive data: %s", buf);
}

// Return NULL if there was an error else return current date as a string
char* getDate() {
	printf("getDate\n");	

	time_t currentTime;
	char *timeStr = NULL;
	char *response;
	char *dateText = "Date: ";
	
	currentTime = time(NULL);
	
	if(currentTime != ((time_t)-1))
		timeStr = ctime(&currentTime);
	
	response = calloc(strlen(dateText) + strlen(timeStr) + 1, sizeof(char));
	strcpy(response, dateText);
	strcat(response, timeStr);	

	return response;
}

char* getFileLastModified(char *path) {
	printf("getFileLastModified param: %s\n", path);	

	char *response;
	char *lastResponseText = "Last-Modified: ";
	char *temp = "File not found!\n";
	struct stat attr;
	if(stat(path, &attr) == 0)
		temp = ctime(&attr.st_mtime);
	
	response = calloc(strlen(lastResponseText) + strlen(temp) + 1, sizeof(char));
	strcpy(response, lastResponseText);
	strcat(response, temp);	
	
	return response;
}

char* handleGet(char* file) {
	printf("HandleGet\n");	
	printf("file: %s", file);
	
	// Variables used in the response
	char *protocol = "HTTP/1.0 ";
	char *responseCode = "400 Bad Request\n";
	char *time = getDate();
	char *server = "Server: runs something\n"; //!!!!
	char *lastModified = "TEMP"; //!!!!
	char *eTag = "ETag: ?\n"; //!!!!
	char *acceptRanges = "Accept-Ranges: bytes\n";
	char *contentLength = "Content-Length: ";
	char *connection = "Connection: close\n";
	char *contentType = "Content-Type: text/html\n\n";
        char *response;
	
	char *page;
	char *pathToWWW = "../../www/";

	FILE *fp;
	size_t fileSize = 0;
	char *pathToRequestedFile;

	pathToRequestedFile = calloc(strlen(pathToWWW) + strlen(file) + 1, sizeof(char));
	strcpy(pathToRequestedFile, pathToWWW);
	strcat(pathToRequestedFile, file);	
	lastModified = getFileLastModified(pathToRequestedFile);
	printf("lastMod: %s\n", lastModified);
	
	printf("open file in path: %s\n", pathToRequestedFile);
	fp = fopen(pathToRequestedFile, "r");
	
	
	if(fp != NULL) {
		printf("File open\n");
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);
		
		rewind(fp);
		page = calloc(fileSize + 1, sizeof(char));
		
		fread(page, fileSize, 1, fp);
		page[fileSize] = '\0';

		printf("Page: %s", page);
		
		fclose(fp);
	}
	

	printf("Put together response\n");	
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
				1, 
				sizeof(char));

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
		
	return response;
}

char* handleBuf(char buf[BUFSIZE]) {
	
	printf("Handle buf: %s", buf);

	char *token;
	char *temp = strdup(buf);
	const char *delim = " \0";
	token = strtok(temp, delim);
	
	//printf("TokeN: %d\n", *token);

	char *response;
	
	if(token != NULL && *token != '\r') {
		// WIP
		// Implement function handling head and return response code
		if(strcmp(token, "HEAD") == 0) {
			printf("Head\n");
			response = "HEAD\n"; 
		}
		// WIP
		// Implement function handling get and return response code
		else if(strcmp(token, "GET") == 0) {
			printf("GET\n");
			token = strtok(NULL, delim); 
			response = handleGet(token);
			printf("%s\n", response);
		}
		else {
			printf("ERROR 501\n");
			response = "ERROR 501\n";
		}
	}
	else{
		printf("ERROR 400 %s\n", token);
		response = "ERROR 400\n";
	}

	return response;
}

int main(int argc, char* argv[]) {
	
	int port = 0;
	struct sockaddr_in sin, pin;
	int sd, sd_current;
	int addrlen;
	char buf[BUFSIZE];
	char *response = "No response";
	
	parameterHandling(argc, &port, argv);

	printf("Listening to port: %d\n", port);
	
	setUp(&sd, port, &sin);		
	
	// Figure out where to fork what, suggestion, fork in start of while
	while(1) {
		for(int i = 0; i < BUFSIZE; i++) {
			buf[i] = '\0';
		}				
		acceptAndRecData(&sd, &sd_current, &pin, &addrlen, buf);
		response = handleBuf(buf);
	
		// Send a response to the client
		if(send(sd_current, response, strlen(response) + 1, 0) == -1) {
			perror("send");
			exit(-1);
		}
	
		// Close current socket
		close(sd_current);
	}	
	
	// Close socket
	close(sd);
	
	return 0;
}
