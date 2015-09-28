#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

#define PORT "21273" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXSIZE 100

//FROM Beej's Guide to Network Programming
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
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

//-------------------------------------split------------------------------------
vector<string> split(string str) {
	vector<string> items;
	int start = 0;
	for (int i = 0; i < str.length();) {
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

//-------------------get all patients' login information---------------------
struct PatientLogin 
{
	string username;
	string password;
	PatientLogin(string u, string p){
		username = u;
		password = p;
	}
};

vector<PatientLogin> patients;

//-------------------------read patients' information------------------------------
void PatientsInfo ()
{
	ifstream fin("users.txt");
	string patient;
	//char* token;
	if(fin == NULL)
	{
		perror("Error opening Users.txt");
		exit(1);
	}
	
	while (getline(fin, patient) != NULL) {
		vector<string> loginInfo = split(patient);
		PatientLogin p(loginInfo[0], loginInfo[1]);
		patients.push_back(p);
		
	}
	//cout<<patients[0].username<<patients[0].password<<endl;
	//cout<<patients[1].username<<patients[1].password<<endl;
	fin.close();
}

//--------------------------------get all avaliable reservations-----------------------------------
struct avlbReserv {
	string index;
	string day;
	string time;
	string docID;
	string port;
	bool reserved;
	avlbReserv(string i, string d, string t, string di, string p, bool r) {
		index = i;
		day = d;
		time = t;
		docID = di;
		port = p;
		reserved = r;
	}
};

vector<avlbReserv> available;

void avlbTime () {
	ifstream fin("availabilities.txt");
	string oneline;
	if(fin == NULL){
		perror("Error opening availabilities.txt");
		exit(1);
	}
	while(getline(fin, oneline) != NULL) {
		vector<string> timeInfo = split(oneline);
		avlbReserv a(timeInfo[0], timeInfo[1], timeInfo[2], timeInfo[3], timeInfo[4], false);
		available.push_back(a);
	}
	/*
	cout<<available[0].index<<available[0].day<<available[0].time<<available[0].docID<<available[0].port<<available[0].reserved<<endl;
	*/
	fin.close();
}

int main(void)
{
	//FROM Beej's Guide to Network Programming
    int sockfd;  // listen on sock_fd
    int new_fd[2] = {0, 0}; //new connection on new_fd
	int patientNamePass, receiveAva, recvIndex;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
	char buf[MAXSIZE], buf_ava[100], buf_index[12];
//-------use for gethostby name---------
	struct hostent *he;
	struct in_addr **addr_list;

	
	PatientsInfo();//get all patients info stored in mem
	avlbTime();//get all available reservations
	
	//FROM Beej's Guide to Network Programming
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    //FROM Beej's Guide to Network Programming
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

//---------------------------------get nunki.usc.edu IP addr----------------------------------------------
	he = gethostbyname("nunki.usc.edu");
	addr_list = (struct in_addr **)he->h_addr_list;
	cout<<"Phase 1: The Health Center Server has port number "<<PORT<<" and IP address "<<inet_ntoa(*addr_list[0])<<"."<<endl;


//---------------------------------waiting for clients connection---------------------------------------
	vector<vector<string> > SplitRecvPatient(2);
    for (int i = 0; i < 2; i++) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd[i] = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd[i] == -1) {
            perror("accept");
            continue;
        }
        
//-------------------------------receive patient login information----------------------------	
        if ((patientNamePass = recv(new_fd[i], buf, MAXSIZE-1, 0)) == -1) {
			perror("Receive Patient Username and Password failure.");
		    exit(1);
		}
		string RecvPatient = buf;
		SplitRecvPatient[i] = split(RecvPatient);
		cout<<"Phase 1: The Health Center Server has received request from a patient with username "<<SplitRecvPatient[i][1]<<" and password "<<SplitRecvPatient[i][2]<<"."<<endl;
		
//-----------------------------------------authentication------------------------------------------------
		bool isExisted = false;
		for(int j = 0; j < 2; j++) {
			if(patients[j].username == SplitRecvPatient[i][1] && patients[j].password == SplitRecvPatient[i][2]) {
				isExisted = true;
				break;
			}	
		}
		if(isExisted) {
			cout<<"Phase 1: The Health Server sends the response success to patient with username "<<SplitRecvPatient[i][1]<<"."<<endl;
			if (send(new_fd[i], "success", 8, 0) == -1) 
				{
    				perror("Send Authentication Response Failure.");
				}
		}
		else {
			cout<<"Phase 1: The Health Center Server sends the response failure to patient with username "<<SplitRecvPatient[i][1]<<"."<<endl;
			if (send(new_fd[i], "failure", 8, 0) == -1) 
				{
    				perror("Send Authentication Response Failure.");
				}
			exit(1);
		}
    }
/*
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
*/
        //printf("server: got connection from %s\n", s);

		
//-------------------------------------------phase 2-------stage 1--------------------------------------
	vector<vector<string> > recvAva_vec(2);
	for (int i = 0; i < 2; i++) {
		if((receiveAva = recv(new_fd[i], buf_ava, 100, 0)) == -1) {
			perror("Receive Avaliable Failure.");
		}
		buf_ava[receiveAva] = '\0';
		string recvAva = buf_ava;
		recvAva_vec[i] = split(recvAva);
		string avaResv; //store all available reservations into this string and send to patient.
		if(recvAva_vec[i][0] == "available") {
			cout<<"Phase 2: The Health Center Server, receives a request for available time slots from patients with port number "<<recvAva_vec[i][1]<<" and IP address "<<recvAva_vec[i][2]<<"."<<endl;
//------------------------------------send available reservations to patient---------------------------
			for(int k = 0; k < 6; k++){
				if(available[k].reserved == false) {
					avaResv = avaResv + available[k].index + " " + available[k].day + " " + available[k].time + " ";
				}
			}
			if(send(new_fd[i], avaResv.c_str(), avaResv.length() + 1, 0) == -1) {
				perror("Send available reservations failure.");
			}
			cout<<"Phase 2: The Health Center Server sends available time slots to patient with username "<<SplitRecvPatient[i][1]<<"."<<endl;
		}
	}
	
	
//------------------------------------receive reservation index from patient-----------------------------------
	for(int i = 0; i < 2; i++){
		if((recvIndex = recv(new_fd[i], buf_index, 12, 0)) == -1) {
			perror("Receive index failure.");
		}
		buf_index[recvIndex] = '\n';
		string recvIndex_str = buf_index;
		vector<string> recvIndex_VecStr = split(recvIndex_str);
		cout<<"Phase 2: The Health Center Server receives a request for appointment "<<recvIndex_VecStr[1]<<" from patient with port number "<<recvAva_vec[i][1]<<" and username "<<SplitRecvPatient[i][1]<<"."<<endl;
		
		int index_num = atoi(recvIndex_VecStr[1].c_str());
		if(available[index_num - 1].reserved == false) {
			available[index_num - 1].reserved = true;
			//-----send doc info to patient-----
			string docInfo = available[index_num - 1].docID + " " + available[index_num - 1].port;
			if(send(new_fd[i], docInfo.c_str(), docInfo.length() + 1, 0) == -1) {
				perror("Send doctor information failure.");
			}
			cout<<"Phase 2: The Health Center Server confirms the following appointment "<<recvIndex_VecStr[1]<<" to patient with username "<<SplitRecvPatient[i][1]<<"."<<endl;
		}
		else {
			//--------send nonavailable to patient--------
			if(send(new_fd[i], "notavailable", 13, 0) == -1) {
				perror("Send notavailable failure.");
			}
			cout<<"Phase 2: The Health Center Server rejects the following appointment "<<recvIndex_VecStr[1]<<" to patient with username "<<SplitRecvPatient[i][1]<<"."<<endl;
		}

        close(new_fd[i]);  // parent doesn't need this
    }//waiting for client connection for loop

    return 0;
}
