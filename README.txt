What I have done in the assignment
I simulated an online medical appointment system using TCP and UDP sockets. There are 3 phases. Phase 1: patients authenticating into the health center server; Phase 2: Patients requesting available appointments and reserving one of them; Phase 3: Patients sending insurance information to the corresponding doctor to get an estimated cost for the visit.

What my code files are and what each one of them does.
My code files are healthcenterserver.cpp, doctor.cpp, patient1.cpp and patient2.cpp.

healthcenterserver.cpp: It is the server of the health center. It authenticates the patients' username and password. It sends available reservations to patients and receives patients' reservation requests. If the reservation is success, it sends the corresponding doctor's port number to the patient.

doctor.cpp(used fork()): It receives patients' insurance information and sends back the estimated cost for the visit.

patient1.cpp: It receives available reservations from the health center server after successfully login into the server. After reserving an available reservation, it receives corresponding doctor's port number and sends its insurance information to the doctor and gets back the estimated cost for the visit.

patient2.cpp: It receives available reservations from the health center server after successfully login into the server. After reserving an available reservation, it receives corresponding doctor's port number and sends its insurance information to the doctor and gets back the estimated cost for the visit.

What should do to run my programs.
1.Use command make to compile all the cpp files.
2.Run healthcenterserver
3.Run doctor
4.Run patient1
5.Run patient2
If you want to delete all the executable files, use command make clean.

The format of all the messages exchanged.
All messages exchanged are in char array.
Phase1: Patients send to health center server: "authenticate username password"
Phase1: Health center server send to patients: "success/failure"
Phase2: Patients send to health center server: "available port# IP_address"
Phase2: Health center server send to patients: "TimeIndex# Day Time"
Phase2: Patients send to health center server: "selection TimeIndex#"
Phase2: Health center server send to patients: "Doctor_ID Doctor_port#"(if it is an available reservation)
Phase2: Health center server send to patients: "not available"(if it is not an available reservation)
Phase3: Patients send to doctor: "insurance port#"
Phase3: Doctor send to patients: "price"
