#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE	256

/* Default port to listen on */
#define DEFAULT_PORT	31337

int main(int argc, char **argv) {

	int socket_fd,new_socket_fd;
	struct sockaddr_in server_addr;
	struct sockaddr client_addr;
	int port=DEFAULT_PORT;
	int n;
	socklen_t client_len;
	char buffer[BUFFER_SIZE];

	/* Predefine exit string 'bye' which ends the write/read loop */
	const char exit_str[5] = {'b', 'y', 'e', '\n'};

	printf("Starting server on port %d\n",port);

	/* Open a socket to listen on */
	/* AF_INET means an IPv4 connection */
	/* SOCK_STREAM means reliable two-way connection (TCP) */
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd<0) {
		fprintf(stderr,"Error opening socket! %s\n",
			strerror(errno));
	}

	/* Set up the server address to listen on */
	/* The memset sets the address to 0.0.0.0 which means */
	/* listen on any interface. */
	memset(&server_addr,0,sizeof(struct sockaddr_in));
	server_addr.sin_family=AF_INET;
	/* Convert the port we want to network byte order */
	server_addr.sin_port=htons(port);

	/* Bind to the port */
	if (bind(socket_fd, (struct sockaddr *) &server_addr,
		sizeof(server_addr)) <0) {
		fprintf(stderr,"Error binding! %s\n", strerror(errno));
	}

	/* Tell the server we want to listen on the port */
	/* Second argument is backlog, how many pending connections can */
	/* build up */
	listen(socket_fd,5);

	/* Call accept to create a new file descriptor for an incoming */
	/* connection.  It takes the oldest one off the queue */
	/* We're blocking so it waits here until a connection happens */
	client_len=sizeof(client_addr);
	new_socket_fd = accept(socket_fd,
		(struct sockaddr *)&client_addr,&client_len);
	if (new_socket_fd<0) {
		fprintf(stderr,"Error accepting! %s\n",strerror(errno));
	}

	/* Someone connected!  Let's try to read BUFFER_SIZE-1 bytes */
	/* Keep reading the client until string 'bye' is detected */
	do {
		memset(buffer,0,BUFFER_SIZE); // clear buffer
		n = read(new_socket_fd,buffer,(BUFFER_SIZE-1));
		if (n==0) {
			fprintf(stderr,"Connection to client lost\n\n");
			break;
		}
		else if (n<0) {
			fprintf(stderr,"Error reading from socket %s\n",
			strerror(errno));
		}

		/* Check for exit string, if detected display, echo and 
		 * jump to the end
		 */
		if(strcmp(buffer, exit_str) == 0) {
			printf("Message from client: %s\n", buffer);
			n = write(new_socket_fd, buffer, strlen(buffer));
			if (n < 0) {
				fprintf(stderr, "Error writing. %s\n",
					strerror(errno));
			} 
			break;
		}

		/* Display client message (or first part if > BUFFER_SIZE) */
		printf("Message from client: ");
		for(int i = 0; i < strlen(buffer); i++) {
			buffer[i] = toupper(buffer[i]);
		}
		printf("%s", buffer);

		/* If a full buffer is sent from the client, assume the full 
		 * message could not be sent in one BUFFER_SIZE buffer.
		 * So echo the first fragment of the client message, prepare
		 * the buffer for another read and display. Repeats until 
		 * full message received. 
		 */
		while (n == (BUFFER_SIZE-2)) {
			// Echo
			n = write(new_socket_fd,buffer,(BUFFER_SIZE-1));
			if (n < 0) {
				fprintf(stderr, "Error writing. %s\n",
					strerror(errno));
			}
			// Read next
			memset(buffer,0,BUFFER_SIZE);
			n = read(new_socket_fd,buffer,(BUFFER_SIZE-1));
			if (n==0) {
				fprintf(stderr,"Connection to client lost\n\n");
				break;
			}
			else if (n<0) {
				fprintf(stderr,"Error reading from socket %s\n",
				strerror(errno));
			}
			// Make uppercase
			for(int i = 0; i < BUFFER_SIZE-1; i++) {
				buffer[i] = toupper(buffer[i]);
			}
			printf("%s", buffer);
		}
		printf("\n");

		/* Echo last message fragment */

		for(int i = 0; i < BUFFER_SIZE-1; i++) {
			buffer[i] = toupper(buffer[i]);
		}
		n = write(new_socket_fd, buffer, strlen(buffer));
		if (n < 0) {
			fprintf(stderr, "Error writing. %s\n",
				strerror(errno));
		} 
	}while(strcmp(buffer, exit_str) != 0);

	printf("Exiting server\n\n");

	/* Try to avoid TIME_WAIT */
//	sleep(1);

	/* Close the sockets */
	close(new_socket_fd);
	close(socket_fd);

	return 0;
}
