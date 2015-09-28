#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

#define DOC1PORT "41273"    // the port users will be connecting to
#define DOC2PORT "42273"

#define MAXBUFLEN 100

//-------------------------------------split------------------------------------
vector<string> split(string str) {
	vector<string> items;
	int start = 0;
	for (int i = 0; i < str.length() ;) {
		while (i < str.length() && str[i] == ' ') {
			start = ++i;
		}
		while (i < str.length() && str[i] != ' ') {
			i++;
		}
		string part = str.substr(start, i - start);
		items.push_back(part);
	}
	return items;
}

//-------------------get all insurance info---------------------
struct insureInfo 
{
	string insure;
	string price;
	insureInfo(string i, string p){
		insure = i;
		price = p;
	}
};

vector<insureInfo> insureInfos;

//-------------------------read insurance information------------------------------
void getInsureInfo_doc1 ()
{
	ifstream fin("doc1.txt");
	string insurePrice;
	if(fin == NULL)
	{
		perror("Error opening doc1.txt");
		exit(1);
	}
	
	while (getline(fin, insurePrice) != NULL) {
		vector<string> insurePriceInfo = split(insurePrice);
		insureInfo i(insurePriceInfo[0], insurePriceInfo[1]);
		insureInfos.push_back(i);
		
	}
	fin.close();
}

void getInsureInfo_doc2 ()
{
	ifstream fin("doc2.txt");
	string insurePrice;
	if(fin == NULL)
	{
		perror("Error opening doc2.txt");
		exit(1);
	}
	
	while (getline(fin, insurePrice) != NULL) {
		vector<string> insurePriceInfo = split(insurePrice);
		insureInfo i(insurePriceInfo[0], insurePriceInfo[1]);
		insureInfos.push_back(i);
		
	}
	fin.close();
}

// get sockaddr, IPv4 or IPv6:
//FROM Beej's Guide to Network Programming
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	//FROM Beej's Guide to Network Programming
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes, sendPrice;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    
//-------use for gethostby name---------
	struct hostent *he;
	struct in_addr **addr_list;
	
	pid_t pid = fork();
	
	if(pid > 0) {
//---------------get nunki.usc.edu IP addr----------------------------------------------
	he = gethostbyname("nunki.usc.edu");
	addr_list = (struct in_addr **)he->h_addr_list;
	cout<<"Phase 3: Doctor 1 has a static UDP port "<<DOC1PORT<<" and IP address "<<inet_ntoa(*addr_list[0])<<"."<<endl;
	
	getInsureInfo_doc1();

for(int i = 0; i < 2; i++) {
	//FROM Beej's Guide to Network Programming
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, DOC1PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';
    string insur_port = buf;
    vector<string> insurance = split(insur_port);
	cout<<"Phase 3: Doctor 1 receives the request from the patient with port number "<<insurance[1]<<" and the insurance plan "<<insurance[0]<<"."<<endl;

	for(int j = 0; j < 3; j++) {
		if(insureInfos[j].insure == insurance[0]) {
			if((sendPrice = sendto(sockfd, insureInfos[j].price.c_str(), insureInfos[j].price.length() + 1, 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
				perror("Send Price failure.");
        		exit(1);
			}
			cout<<"Phase 3: Doctor 1 has sent estimated price "<<insureInfos[j].price<<"$ to patient with port number "<<insurance[1]<<"."<<endl;
			break;
		}
	}
	
    close(sockfd);
	}//for loop end
	}//parent process
	else if(pid == 0) {
		//---------------get nunki.usc.edu IP addr----------------------------------------------
	he = gethostbyname("nunki.usc.edu");
	addr_list = (struct in_addr **)he->h_addr_list;
	cout<<"Phase 3: Doctor 2 has a static UDP port "<<DOC2PORT<<" and IP address "<<inet_ntoa(*addr_list[0])<<"."<<endl;
	
	getInsureInfo_doc2();

for(int i = 0; i < 2; i++) {
	//FROM Beej's Guide to Network Programming
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, DOC2PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';
    string insur_port = buf;
    vector<string> insurance = split(insur_port);
	cout<<"Phase 3: Doctor 2 receives the request from the patient with port number "<<insurance[1]<<" and the insurance plan "<<insurance[0]<<"."<<endl;

	for(int j = 0; j < 3; j++) {
		if(insureInfos[j].insure == insurance[0]) {
			if((sendPrice = sendto(sockfd, insureInfos[j].price.c_str(), insureInfos[j].price.length() + 1, 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
				perror("Send Price failure.");
        		exit(1);
			}
			cout<<"Phase 3: Doctor 2 has sent estimated price "<<insureInfos[j].price<<"$ to patient with port number "<<insurance[1]<<"."<<endl;
			break;
		}
	}
	
    close(sockfd);
	}//for loop end
	}//child process
	else {
		cout<<"fork() failed."<<endl;
		return 1;
	}
    return 0;
}
