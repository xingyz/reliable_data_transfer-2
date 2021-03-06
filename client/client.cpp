// FTP Client over Datagram (UDP) via router
// @author Stephen Young
// @email st_youn@encs.concordia.ca
// @student_id 9736247
#pragma comment(linker,"/defaultlib:ws2_32.lib")
#include <windows.h>
#include <winsock.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;
#include "ftplib.h"
#include "protocol.h"
#include "socketlib.h"

int main(void){
	srand(time(NULL));

	//socket data types
	SOCKET client_socket;   // Client socket
	SOCKADDR_IN sa_out;      // fill with server info, IP, port

	char buffer[RAWBUF_SIZE]; // Buffer

	WSADATA wsadata;                                    // WSA connection
	char router[11];                                    // Host data
	char cusername[128], filename[128], direction[11];   // Other header data
	DWORD dwusername = sizeof(cusername);               // Retains the size of the username
	char trace_data[128];

	FILE* logfile = fopen("client.log", "w");

	try {

		if (WSAStartup(0x0202, &wsadata) != 0){
			throw "Error in starting WSAStartup";
		}
		else {

			/* Display the wsadata structure */
			cout << endl
				<< "wsadata.wVersion " << wsadata.wVersion << endl
				<< "wsadata.wHighVersion " << wsadata.wHighVersion << endl
				<< "wsadata.szDescription " << wsadata.szDescription << endl
				<< "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
				<< "wsadata.iMaxSockets " << wsadata.iMaxSockets << endl
				<< "wsadata.iMaxUdpDg " << wsadata.iMaxUdpDg << endl;
		}

		client_socket = open_port(PEER_PORT2);

		prompt("Enter the router hostname: ", router);
		sa_out = prepare_peer_connection(router, ROUTER_PORT2);

		prompt("Enter a filename: ", filename);                  // Retrieve a filename from the client
		prompt("Direction of transfer [get|put]: ", direction);  // Retrieve a transfer direction

		// Make sure the direction is one of get or put
		if (!strcmp(direction, GET) || !strcmp(direction, PUT)){

			// Retrieve the local user name
			GetUserName(cusername, &dwusername);

			int selected = rand() % 256;
			int received, verify;

			int client_num = 0; // Client packet number
			int server_num = 0; // Server packet number

			int progress = 0;
			int rcv;

			cout << "Starting packet ID negotiation..." << endl;

			while (1){

				// Send a random number to the server
				if (progress < 1){
					memset(buffer, 0, sizeof(buffer));
					sprintf(buffer, "RAND %d", selected);
					cout << "Sending " << buffer << endl;
					if ((rcv = send_safe(client_socket, sa_out, buffer, RAWBUF_SIZE, 200)) == 201){
						progress = 1;
					}
					else if (rcv != 200){
						continue;
					}

					// Finally wait for a response from the server with the number
					if (recv_safe(client_socket, sa_out, buffer, RAWBUF_SIZE, 100) == 100){
						cout << "Received " << buffer << endl;
						sscanf(buffer, "RAND %d %d", &verify, &received);
					}
					else continue;
					progress = 1;
				}

				// Send acknowledgement to the server along with our random number
				memset(buffer, 0, sizeof(buffer));
				sprintf(buffer, "RAND %d", received);
				cout << "Sending " << buffer << endl;
				if (send_safe(client_socket, sa_out, buffer, RAWBUF_SIZE, 201) != 201){
					progress = 0;
					continue;
				}
				break;
			}

			client_num = selected % WINDOW_SIZE + 1;
			server_num = received % WINDOW_SIZE + 1;

			cout << "Negotiated server start " << server_num << " and client start " << client_num << endl;

			sprintf(trace_data, "Negotiated srv %d and cli %d", server_num, client_num);
			write_log(logfile, cusername, trace_data);
			// Send client headers
			sprintf(buffer, HEADER, cusername, direction, filename);
			while ((rcv = send_safe(client_socket, sa_out, buffer, RAWBUF_SIZE, 777)) != 777){
				if (rcv == 101) break;
			}



			// Perform a get request
			if (!strcmp(direction, GET)){
				get(client_socket, sa_out, cusername, filename, client_num, server_num, logfile);

			}
			else if (!strcmp(direction, PUT)){
				put(client_socket, sa_out, cusername, filename, client_num, server_num, logfile);
			}

		}
		else{
			throw "The method you requested does not exist, use get or put";
		}

	} // try loop

	//Display any needed error response.
	catch (const char *str) {
		cerr << str << WSAGetLastError() << endl;
	}

	//close the client socket and clean up
	fclose(logfile);
	closesocket(client_socket);
	WSACleanup();
	system("pause");
	return 0;
}