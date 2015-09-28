#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

#define PORT "21273" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
//FROM Beej's Guide to Network Programming
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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

//-----------read patient1 login info----------------
string patientInfo() {
	ifstream fin("patient2.txt");
	string namepass;
	if(fin == NULL) {
		perror("Error opening patient2.txt");
		exit(-1);
	}
	if(getline(fin, namepass) == NULL) {
		perror("Error getting Patient2 Information");
		fin.close();
		exit(-1);
	}
	fin.close();
	return namepass;	
}

string insuranceInfo() {
	ifstream fin("patient2insurance.txt");
	string namepass;
	if(fin == NULL) {
		perror("Error opening patient2.txt");
		exit(-1);
	}
	if(getline(fin, namepass) == NULL) {
		perror("Error getting Patient2 Information");
		fin.close();
		exit(-1);
	}
	fin.close();
	return namepass;	
}

int main(int argc, char *argv[])
{
	//FROM Beej's Guide to Network Programming
    int sockfd, authResponse, avaResv, confirm_num, sockfd_udp, sendInsur, recvPrice;  
    char buf_auth[8];//buffer authentication response
    char buf_avaResv[200];//buffer available reservations
    char buf_confirm[13];//buffer confirmation info
    char buf_price[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p, hints_udp, *servinfo_udp, *p_udp;
    struct sockaddr_in sin, sin_udp;
    struct sockaddr_storage their_addr;
    int rv, rv_udp;
    char s[INET6_ADDRSTRLEN];
/*
    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }
*/
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo("nunki.usc.edu", PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    //printf("client: connecting to %s\n", s);
    socklen_t len = sizeof sin;
    if(getsockname(sockfd, (struct sockaddr *) &sin, &len) == -1) {
    	perror("getsockname");
    	exit(1);
    }
    string myIP = inet_ntoa(sin.sin_addr);
	cout<<"Phase 1: Patient 2 has TCP port number "<<sin.sin_port<<" and IP address "<<myIP<<"."<<endl;
	
    freeaddrinfo(servinfo); // all done with this structure


//------------------ReadUserFile and send to server-------------------------------
	string insurance = insuranceInfo();
    string patient2 = patientInfo();
    vector<string> patient2UP = split(patient2);
	patient2 = "authenticate " + patient2;
	//cout<<patient1<<endl;

    if (send(sockfd, patient2.c_str(), patient2.length() + 1, 0) == -1) 
	{
    	perror("Send Username and Password Failure.");
	}
	cout<<"Phase 1: Authentication request from Patient 2 with username "<<patient2UP[0]<<" and password "<<patient2UP[1]<<" has been sent to the Health Center Server."<<endl;
	
//-------------------------get authentication response-------------------------------	
	if ((authResponse = recv(sockfd, buf_auth, 8, 0)) == -1) {
        perror("Receive authentication response failure.");
        exit(1);
    }
    buf_auth[authResponse] = '\0';
	
	string auth = buf_auth;
	if(auth == "success") {
		cout<<"Phase 1: Patient 2 authentication result: success."<<endl;
		cout<<"Phase 1: End of Phase 1 for Patient2"<<endl;
	}
	else {
		cout<<"Phase 1: Patient 2 authentication result: failure."<<endl;
		exit(1);
	}

//-------------------------------------phase 2------stage1------------------------------------------------
//----------------asking for availabilities---------------
	stringstream ss;
	ss << sin.sin_port;
	string port_str = ss.str();
	string ava_ip_port = "available " + port_str + " " + inet_ntoa(sin.sin_addr);
	if(send(sockfd, ava_ip_port.c_str(), ava_ip_port.length() + 1, 0) == -1) {
		perror("Request Availablilities Failure.");
	}
//--------------receive available reservations-------------
	if((avaResv = recv(sockfd, buf_avaResv, 200, 0)) == -1) {
		perror("Receive available reservations failure.");
	}
	buf_avaResv[avaResv] = '\0';
	string avaResv_str = buf_avaResv;
	vector<string> avaResv_VecStr = split(avaResv_str);
	cout<<"Phase 2: The following appointments are available for Patient 2:"<<endl;
	for(int i = 0; i < avaResv_VecStr.size() - 1; i = i + 3) {
		cout<<avaResv_VecStr[i]<<" "<<avaResv_VecStr[i+1]<<" "<<avaResv_VecStr[i+2]<<endl;
	}
	
//------------------------get patient's entered index-----------------------------	
	cout<<"Please enter the preferred appointment index and press enter: ";
	while(1) {
		bool isExist = false;
		string index;
		cin>>index;
		for(int j = 0; j < avaResv_VecStr.size() - 1; j = j + 3) {
			//cout<<avaResv_VecStr[j]<<endl;
			if(index == avaResv_VecStr[j]) {
				index = "selection " + index;
				if(send(sockfd, index.c_str(), index.length() + 1, 0) == -1) {
					perror("Send index failure.");
				}
				isExist = true;
				break;
			}
		}
		if(isExist) {
			break;
		}
		else {
			cout<<"Invalid Index. Please enter another valid index and press enter: ";
		}
	}
//-------------------receive reservation confirmation---------------------------
	if((confirm_num = recv(sockfd, buf_confirm, 13, 0) ) == -1) {
		perror("Receiver reservation confirmation failure.");
	}
	buf_confirm[confirm_num] = '\n';
	string confirm_str = buf_confirm;
	vector<string> confirm_VecStr = split(confirm_str);
	if(confirm_str == "notavailable") {
		cout<<"Phase 2: The requested appointment from Patient 2 is not available. Exiting..."<<endl;
		close(sockfd);
		exit(1);
	}
	else {
		confirm_VecStr[1][2] = '2';
		confirm_VecStr[1][3] = '7';
		confirm_VecStr[1][4] = '3';
		cout<<"Phase 2: The requested appointment is available and reserved to Patient2. The assigned doctor port number is "<<confirm_VecStr[1]<<"."<<endl;
		close(sockfd);
	}
//--------------------------------------Phase 3-----------------------------------------------------
//----------------create UDP-------------------------------
//FROM Beej's Guide to Network Programming
	memset(&hints_udp, 0, sizeof hints_udp);
    hints_udp.ai_family = AF_UNSPEC;
    hints_udp.ai_socktype = SOCK_DGRAM;
    
    if ((rv_udp = getaddrinfo("nunki.usc.edu", confirm_VecStr[1].c_str(), &hints_udp, &servinfo_udp)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_udp));
        return 1;
    }
    
    // loop through all the results and make a socket
    for(p_udp = servinfo; p_udp != NULL; p_udp = p_udp->ai_next) {
        if ((sockfd_udp = socket(p_udp->ai_family, p_udp->ai_socktype,
                p_udp->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }
    
     if (p_udp == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }
//The getsockname retrieves the local address associated with the socket. The UDT socket must be bound explicitly (via bind) or implicitly (via connect), otherwise this method will fail because there is no meaningful address bound to the socket.

    sin_udp.sin_family = AF_INET;
    sin_udp.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("");
    //inet_aton("68.180.201.3",&sin_udp.sin_addr.s_addr);
    sin_udp.sin_port = 0;
    
    if (bind(sockfd_udp, (struct sockaddr *) &sin_udp, sizeof sin_udp) == -1) {
            close(sockfd_udp);
            perror("server: bind");
        }   
    
    socklen_t len_udp = sizeof sin_udp;
    if(getsockname(sockfd_udp, (struct sockaddr *) &sin_udp, &len_udp) == -1) {
    	perror("getsockname");
    	exit(1);
    }
	cout<<"Phase 3: Patient 2 has a dynamic UDP port number "<<sin_udp.sin_port<<" and IP address "<<myIP<<"."<<endl;
 
 //---------------------------send insurance to doctor--------------------------------
 	stringstream ss_udp;
	ss_udp << sin_udp.sin_port;
	string port_udp_str = ss_udp.str();
	   
    string insurPort = insurance + " " + port_udp_str;
    if ((sendInsur = sendto(sockfd_udp, insurPort.c_str(), insurPort.length() + 1, 0,
             p_udp->ai_addr, p_udp->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    
    cout<<"Phase 3: The cost estimation request from Patient 2 with insurance plan "<<insurance<<" has been sent to the doctor with port number "<<confirm_VecStr[1]<<" and IP address "<<myIP<<"."<<endl;
 
 //----------------------------------get price----------------------------------------   
    socklen_t addr_len = sizeof their_addr;
    if ((recvPrice = recvfrom(sockfd_udp, buf_price, MAXDATASIZE , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("Receive price failure.");
        exit(1);
    }
    buf_price[recvPrice] = '\0';
    cout<<"Phase 3: Patient 2 receives "<<buf_price<<"$ estimation cost from doctor with port number "<<confirm_VecStr[1]<<" and name "<<insurance<<"."<<endl;
    cout<<"Phase 3: End of Phase 3 for Patient 2."<<endl;

    freeaddrinfo(servinfo_udp);
    

    close(sockfd_udp);

    return 0;
}
