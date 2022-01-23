#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define DEFAULT_TFTP_PORT "69"
#define BUFFER_SIZE 512
#define OPCODE_LENGTH 2
#define BLOCKID_LENGTH 2
#define ASCII_ZERO_LENGTH 1
#define ACK_LENGTH 4
#define NO_FLAG_SPECIFIED 0
#define DATA_SIZE (BUFFER_SIZE - (OPCODE_LENGTH + BLOCKID_LENGTH))


#define IS_VALID_SOCKET(s) ((s) >= 0)
#define IS_FULL_SOCKET(s) ((s) == BUFFER_SIZE)

char* host;
char* port;
char* file;

void parseCmd(char** cmd) {
	host = strtok(cmd[1], ":");
	char* portString = strtok(NULL, ":");

	if(portString == NULL){
		port = DEFAULT_TFTP_PORT;
	}else{
		port = portString;
	}

	file = cmd[2];
}

int main(int argc, char *argv[]){
	//Commande sous la forme prgm host:port file
	if(argc != 3) {
		printf("Usage : ./gettftp [host:port] [file]\n");
		exit(EXIT_FAILURE);
	}
	parseCmd(argv);

	//Structure pour filtrer la réponse de getaddrinfo()
	struct addrinfo hints;

	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    	// Autorise les IPV4 et IPV6
	hints.ai_socktype = SOCK_DGRAM; 	// Datagram socket
	hints.ai_protocol = IPPROTO_UDP;	// Protocole UDP


	//Structure pour stocker la réponse de getaddrinfo()
	struct addrinfo* result;
	//printf("J'essaye de joinde %s, sur le port %s, le fichier %s...\n", host, port, file);

	if(getaddrinfo(host, port, &hints, &result)) {
		printf("getaddrinfo() a échoué\n");
		exit(EXIT_FAILURE);
	}


	//Socket
	int local_fd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	int socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);


	if(!IS_VALID_SOCKET(socket_fd)){
		printf("socket() a échoué\n");
		exit(EXIT_FAILURE);
	}


	//Construction de la requête WRQ
	char* request;
	int requestLength;

	requestLength = OPCODE_LENGTH + strlen(file) + ASCII_ZERO_LENGTH  + strlen("netascii") + ASCII_ZERO_LENGTH ;
	request = malloc(requestLength);

	//Opcode
	request[0] = 0;
	request[1] = 2;

	//Filename
	char* fileOffset = request + OPCODE_LENGTH;
	strcpy(fileOffset, file);					//\0 inclu

	//Mode
	char* modeOffset = fileOffset + strlen(file) + ASCII_ZERO_LENGTH ;
	strcpy(modeOffset, "netascii");				//\0 inclu

	//Envoie de la requête WRQ
	if(sendto(socket_fd, request, requestLength, NO_FLAG_SPECIFIED, result->ai_addr, result->ai_addrlen) < 0){
		printf("La requête WRQ a échouée. Code d'erreur : %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}


	//Reception de l'ACK de la requête
	struct sockaddr address;						//Permet de stocket l'adresse de la source du paquet
	socklen_t addressLen = sizeof(address);
	char* receivedBuffer = malloc(BUFFER_SIZE * sizeof(char));
	char* sendingBuffer = malloc(BUFFER_SIZE * sizeof(char));


	if(recvfrom(socket_fd, receivedBuffer, BUFFER_SIZE, NO_FLAG_SPECIFIED, &address, &addressLen) < 0) {
		printf("La requête WRQ a échouée, pas d'ACK reçue. Code d'erreur : %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("Connecté, prêt à l'envoie des données\n");

	//Opcode
	sendingBuffer[0] = 0;
	sendingBuffer[1] = 3;

	//Blockid init
	sendingBuffer[3] = 1;

	int sentBytes = BUFFER_SIZE - OPCODE_LENGTH - BLOCKID_LENGTH ;

	while(sentBytes == BUFFER_SIZE - OPCODE_LENGTH - BLOCKID_LENGTH) {
		printf("DATA %d\n", sendingBuffer[3]);
		sentBytes = read(local_fd, sendingBuffer + OPCODE_LENGTH + BLOCKID_LENGTH, DATA_SIZE);
		printf("%s\n", sendingBuffer + OPCODE_LENGTH + BLOCKID_LENGTH);

		if(sentBytes < 0){
			printf("read() à échoué");
			exit(EXIT_FAILURE);
		}

		if(sendto(socket_fd, sendingBuffer, BUFFER_SIZE, NO_FLAG_SPECIFIED, &address, addressLen) < 0) {
			printf("sendto() à échoué");
			exit(EXIT_FAILURE);
		}

		if(recvfrom(socket_fd, receivedBuffer, BUFFER_SIZE, NO_FLAG_SPECIFIED, &address, &addressLen) < 0) {
			printf("La requête WRQ a échouée, pas d'ACK reçue. Code d'erreur : %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		sendingBuffer[3]++;
	}
	close(socket_fd);
	close(local_fd);
	freeaddrinfo(result);
	free(request);
	exit(EXIT_SUCCESS);
}
