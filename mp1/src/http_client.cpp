/*
** client.c -- a stream socket client demo
*/

#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <cstring>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fstream>
#include <arpa/inet.h>

using namespace std;
//#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)//acquire IP address. And If Protocol family is AF_NET,it would be IPV4.Else,it would be IPV6
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  //sockfd is socket
	char buf[MAXDATASIZE]; //store the recived data from server
	struct addrinfo hints, *servinfo, *p;//hints is a struct and it store all parameters which could build socket,servinfo save all information about server.
	int rv;
	char s[INET6_ADDRSTRLEN];
	string host_URL;//the host_URL, such as 12.34.56.78::88898.(In this string,12.34.56.78 is hostName, 8888 is port)
	string hostName;
	string port;
	string path;//the documents path in the server

	if (argc != 2) {//if argc equal 2,then error,exit.
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	string getWholeInformation = argv[1]; //get all of information.(run as "./http_client http://hostname[:port]/path/to/file ")
	int findProtocolPosition = getWholeInformation.find("//", 0); 
	/*cout << findProtocolPosition << endl;*/
	string getInformation = getWholeInformation.substr(findProtocolPosition + 2, getWholeInformation.length());//extract required information,such as (hostname[:port]/path/to/file)
	/*cout << getInformation << endl;*/
	int URL_position = getInformation.find("/", 0);
	host_URL = getInformation.substr(0, URL_position);//get host_URL,such as (/hostname[:port])
	path = getInformation.substr(URL_position, getInformation.length());
	cout << host_URL << endl;
	cout << path << endl;
	int portPosition;
	if (host_URL.find(":") != -1)
	{
		portPosition = host_URL.find(":");
		hostName = host_URL.substr(0, portPosition);//get hostName,such as (hostname)
		port = host_URL.substr(portPosition + 1, host_URL.length());//get port,such as ([:port])
	}
	else
	{
		hostName = host_URL.substr(0, host_URL.length());
		port = "80";//if not find ":", it means no port shown on the sentence, so we should set port as 80
	}
	cout << hostName << endl;
	cout << port << endl;


	memset(&hints, 0, sizeof hints);//through library memset,Clear all memory space pointed to by hints
	hints.ai_family = AF_UNSPEC;//Specify the local (Server) protocol family
	hints.ai_socktype = SOCK_STREAM;//Specify the transmission method

	if ((rv = getaddrinfo(hostName.c_str(), port.c_str(), &hints, &servinfo)) != 0) {//Store all the information that the server should have in servinfo
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,   //Create socket, return socket
				p->ai_protocol)) == -1) {//if it fails, return error
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {//try to connect server
			close(sockfd);//if fail, close socket
			perror("client: connect");
			continue;
		}

		break;//The first time the socket is established and the connection to the server is successful, the loop is exited.
	}

	if (p == NULL) { //If p=null, it means that the server cannot be found through the address. That is, it does not exist. .
		fprintf(stderr, "client: failed to connect\n");//connection fail 
		return 2;
	}
	//The connection is successful p->ai_family points to the protocol family. get_In_addr is to get the server IP address
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	//Get the server IP address, this function converts the 32-bit IPv4 or 128-bit IPv6 address (if PHP was built with IPv6 support enabled) into the appropriate string representation of the address
	printf("client: connecting to %s\n", s);//Print IP address

	freeaddrinfo(servinfo); // all done with this structure
	string request = "GET " + path + " HTTP/1.1\r\n";

	string host_content = "Host: " + hostName + ":" + port + "\r\n";

	string userAgent = "User-Agent: Wget/1.12 (linux-gnu)\r\n";

	string connectionStatus = "Connection: Keep-Alive\r\n\r\n";



	string theRequestContent = request + userAgent + host_content + connectionStatus;

	send(sockfd, theRequestContent.c_str(), strlen(theRequestContent.c_str()), 0);//send request to server.





	/*if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {//Receive information from the server, if ==-1, it means failure.

		perror("recv");

		exit(1);

	}*/



	//printf("client: received '%s'\n",buf);//not failure



	ofstream outputFile;//output file stream

	outputFile.open("output", ios::out | ios::binary);//open output document

	bool flag = true;//if numbytes of receving data is smaller than 0,then error,need to close the output.file right now. 



	while (flag) {
		numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0);//receving data from server
		buf[numbytes] = '\0';//The terminator is '\0', indicating that the information has been received here.
		if (numbytes > 0) {
			char* newbuf = strstr(buf, "\r\n\r\n");
			if (newbuf != NULL) //if has content
			{

				newbuf += 4;//delete the first two lines in documents.(because it contains some header information)

				outputFile.write(newbuf, strlen(newbuf));//write buffer(data from server) to file
			}
			else

			{

				outputFile.write(buf, numbytes);//write buffer(data from server) to file

			}

		}
		else {

			flag = false;

		}


	}
	outputFile.close();//after receving all data and wrtie,then close output file stream

	close(sockfd);//after reciving all data from server, close socket
	return 0;
}

