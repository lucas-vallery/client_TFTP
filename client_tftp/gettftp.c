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

int main(int argc, char *argv[]) {

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


	//Construction de la requête RRQ
	char* request;
	int requestLength;

	requestLength = OPCODE_LENGTH + strlen(file) + ASCII_ZERO_LENGTH  + strlen("netascii") + ASCII_ZERO_LENGTH ;
	request = malloc(requestLength);

	//Opcode
	request[0] = 0;
	request[1] = 1;

	//Filename
	char* fileOffset = request + OPCODE_LENGTH;
	strcpy(fileOffset, file);					//\0 inclu

	//Mode
	char* modeOffset = fileOffset + strlen(file) + ASCII_ZERO_LENGTH ;
	strcpy(modeOffset, "netascii");				//\0 inclu


	//Envoie de la requête RRQ
	if(sendto(socket_fd, request, requestLength, NO_FLAG_SPECIFIED, result->ai_addr, result->ai_addrlen) == -1){
		printf("La requête RRQ a échouée. Code d'erreur : %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}


	//Construction de la requête ACK
	char ack[ACK_LENGTH];
	ack[0] = 0;
	ack[1] = 4;


	//RECEPTION D'UN PAQUET
	/*
	//Reception du fichier demandé
	char* receivedBuffer = malloc(BUFFER_SIZE*sizeof(char));
	struct sockaddr address;						//Permet de stocket l'adresse de la source du paquet
	socklen_t addressLen = sizeof(address);

	int receivedBytes = recvfrom(socket_fd, receivedBuffer, BUFFER_SIZE,NO_FLAG_SPECIFIED, &address, &addressLen);
	*/


	//RECEPTION DE PLUSIEURS PAQUETS
	char* receivedBuffer = malloc(BUFFER_SIZE*sizeof(char));
	struct sockaddr address;						//Permet de stocket l'adresse de la source du paquet
	socklen_t addressLen = sizeof(address);

	int receivedBytes = BUFFER_SIZE;

	while(IS_FULL_SOCKET(receivedBytes)) {
		receivedBytes = recvfrom(socket_fd, receivedBuffer, BUFFER_SIZE, NO_FLAG_SPECIFIED, &address, &addressLen);

		if (receivedBytes == -1) {
			printf("Erreur dans la réception");
			close(socket_fd);
			close(local_fd);
			freeaddrinfo(result);
			free(request);
			exit(EXIT_FAILURE);
		}

		printf("Reception de %d octets venant du serveur\n", receivedBytes);

		//On complete l'ACK
		ack[2] = receivedBuffer[2];
		ack[3] = receivedBuffer[3];

		//Envoie de l'ACK
		if(sendto(socket_fd, ack, ACK_LENGTH, NO_FLAG_SPECIFIED, &address, addressLen) == -1){
			printf("La requête ACK a échouée. Code d'erreur : %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		//Ecriture dans le fichier
		write(local_fd, receivedBuffer + BLOCKID_LENGTH + OPCODE_LENGTH, receivedBytes - BLOCKID_LENGTH - OPCODE_LENGTH);
	}

	close(socket_fd);
	close(local_fd);
	freeaddrinfo(result);
	free(request);
	exit(EXIT_SUCCESS);
}
