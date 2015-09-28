all: healthcenterserver doctor patient1 patient2

healthcenterserver: healthcenterserver.cpp
	g++ -o healthcenterserver healthcenterserver.cpp -lsocket -lnsl -lresolv
	
doctor: doctor.cpp
	g++ -o doctor doctor.cpp -lsocket -lnsl -lresolv
	
patient1: patient1.cpp
	g++ -o patient1 patient1.cpp -lsocket -lnsl -lresolv
	
patient2: patient2.cpp
	g++ -o patient2 patient2.cpp -lsocket -lnsl -lresolv
	
clean:
	\rm healthcenterserver doctor patient1 patient2
