/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Nikita Kelwada
	UIN: 434003223
	Date: 9/19/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/time.h>
#include <iomanip>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>  // Add this for strcmp

using namespace std;

int main(int argc, char *argv[])
{
    // Process -m flag first to get buffer capacity before forking
    int buffercapacity = MAX_MESSAGE; // default
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            buffercapacity = atoi(argv[i + 1]);
            break;
        }
    }

    int pid = fork();

    if (pid == 0)
    {
        string buffer_val = to_string(buffercapacity);
        char* const sv_argv[] = { 
            (char*)"server", 
            (char*)"-m", 
            (char*)buffer_val.c_str(), 
            nullptr 
        };
        execvp("./server", sv_argv);
    }
    else
    {
        //Parent process
        FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

        //need to sure the command line arguments are correct
        int c = 0; //will be used to process command line arguments

        int patient = -1;
        double time = -1;
        int ecg = 0;
        bool newChannel = false; //boolean to determine if user wants a new channel
        string fileName = ""; // string to hold file name if user wants to transfer a file

        // NOTE: buffercapacity is already declared above, don't redeclare it

        //process command line arguments
        while ((c = getopt(argc, argv, "p:t:e:f:cm:")) != -1)
        { //this loop processes each command line argument
            switch (c)
            {
            case 'p': 
                if (optarg) // if p flag is present, then we set patient to the value of the argument
                {
                    patient = atoi(optarg);
                } 
                break;
            case 't':
                if (optarg)
                {
                    time = atof(optarg); // if t flag is present, then we set time to the value of the argument
                }
                break;
            case 'e': 
                if (optarg)
                {
                    ecg = atoi(optarg); // if e flag is present, then we set ecg to the value of the argument
                }
                break;
            case 'f': 
                if (optarg)
                {
                    fileName = string(optarg); // if f flag is present, then we set fileName to the value of the argument 
                }
                break;
            case 'c':
                newChannel = true; //set new channel boolean to true
                break;
            case '?':
                EXITONERROR("Invalid Option");// if user enters an invalid option, we exit
                break;
            case 'm':
                cout << "Changing initial buffer capacity of: " << buffercapacity << "..." << endl;// if m flag is present, then we change the buffer capacity to the value of the argument
                buffercapacity = atoi(optarg);
                cout << "DONE! New buffer capacity is: " << buffercapacity << endl;// if m flag is present, then we change the buffer capacity to the value of the argument
                break;
            }
        }

        // now we need to validate the command line arguments
        if ((time < 0 || time > 59.996) && time != -1) {EXITONERROR("Invalid time");}// time must be between 0 and 59.996 seconds
        if (ecg < 0 || ecg > 2) {EXITONERROR("Invalid ecg value");}// ecg can only be 0, 1, or 2

        if (time != -1 && ecg != 0) // if the user wants an individual data point
        {
            // validate patient number
            if (patient < 1 || patient > 15){EXITONERROR("Invalid patient");}
            datamsg dataPoint(patient, time, ecg);// create data message
            chan.cwrite(&dataPoint, sizeof(datamsg));// send data message to server

            double result = 0.0;
            chan.cread(&result, sizeof(double));// read result from server
            cout << "For person " << patient
                 << ", at time " << fixed << setprecision(3) << time // set precision to 3 decimal places because time is in milliseconds
                 << ", the value of ecg " << ecg
                 << " is " << fixed << setprecision(2) << result << endl;
        }

        // here itll handle the case where the user wants all data points for a patient
        else if (patient != -1)
        {
            if (patient < 1 || patient > 15)//just in case user enters invalid patient number
            {
                EXITONERROR("Invalid patient");
            }

            struct timeval start; // will be used to get the time taken for the data transfer
            gettimeofday(&start, NULL);
            double totalTime = 0;
            //  define the output file
            ofstream myfile;
            mkdir("received", 0777);
            string fileName = "received/x1.csv"; // Fixed: should always be x1.csv
            myfile.open(fileName);

            //the loop will run 1000 times to get all the data points for the patient
            for (int i = 0; i < 1000; i++)
            {
                //first ecg column
                datamsg req1(patient, totalTime, 1);
                chan.cwrite(&req1, sizeof(datamsg));

                double received1 = 0.0;
                chan.cread(&received1, sizeof(double));
                myfile << totalTime << "," << received1 << ","; // write time and ecg1 value to file

                //second ecg column
                datamsg req2(patient, totalTime, 2);
                chan.cwrite(&req2, sizeof(datamsg));
                double received2 = 0.0;
                chan.cread(&received2, sizeof(double));
                myfile << received2 << endl;
                totalTime += 0.004;
            }

            struct timeval end;
            gettimeofday(&end, NULL);

            // then  calculate the time taken for the data transfer
            double totalStart = 0;
            double totalEnd = 0;
            totalStart = (double)start.tv_usec + (double)start.tv_sec * 1000000;
            totalEnd = (double)end.tv_usec + (double)end.tv_sec * 1000000;
            cout << "The data exchange performed took: " << totalEnd - totalStart << " microseconds." << endl; // print time taken
        }

        // then handle the case where the user wants to transfer a file
        else if (fileName != "") // if the user wants to transfer a file
        {
            // first we need to get the length of the file
            filemsg getLen(0, 0);
            std::vector<char> initial(sizeof(filemsg) + fileName.size() + 1);
            memcpy(initial.data(), &getLen, sizeof(filemsg));
            memcpy(initial.data() + sizeof(filemsg), fileName.c_str(), fileName.size() + 1);
            chan.cwrite(initial.data(), initial.size());
            __int64_t filelen;
            chan.cread(&filelen, sizeof(__int64_t));
            cout << "File length: " << filelen << endl;

            // define the output file
            ofstream myfile;
            string file_name = "received/" + fileName;
            myfile.open(file_name, ios::out | ios::binary); // make sure output file is binary
            struct timeval start;
            if (gettimeofday(&start, NULL) != 0) {
                perror("gettimeofday");
                exit(EXIT_FAILURE);
            } // will be used to get the time taken for the file transfer

            // then we need to transfer the file in chunks
            __int64_t offset = 0;
            __int64_t length = filelen;
            std::vector<char> payload(buffercapacity);
            while (length > offset) // while there is still data to be transferred
            {
                size_t chunk = static_cast<size_t>(min<__int64_t>(buffercapacity, length - offset));
                filemsg segment(offset, chunk);
                std::vector<char> header(sizeof(filemsg) + fileName.size() + 1);
                memcpy(header.data(), &segment, sizeof(filemsg));
                memcpy(header.data() + sizeof(filemsg), fileName.c_str(), fileName.size() + 1);
                chan.cwrite(header.data(), header.size());

                chan.cread(payload.data(), chunk);  // FIXED: read 'chunk' bytes, not 'buffercapacity'
                myfile.write(payload.data(), chunk);
                offset += chunk;
            }
            //this will get the end time for the file transfer
            struct timeval end;
            gettimeofday(&end, NULL);

            double totalStart = 0;
            double totalEnd = 0;
            totalStart = (double)start.tv_usec + (double)start.tv_sec * 1000000; // convert to microseconds
            totalEnd = (double)end.tv_usec + (double)end.tv_sec * 1000000;

            cout << "The data exchange performed took: " << totalEnd - totalStart << " microseconds" << endl; // print time taken
        }
        else if (newChannel) // if the user wants a new channel
        {
            //test with new channel
            MESSAGE_TYPE n = NEWCHANNEL_MSG;
            chan.cwrite(&n, sizeof(MESSAGE_TYPE));
            //then  need to read the name of the new channel that the server sends back
            //but first  need to create a buffer to hold the name of the new channel
            std::vector<char> newChan(30);
            chan.cread(newChan.data(), buffercapacity);
            FIFORequestChannel newChannel(newChan.data(), FIFORequestChannel::CLIENT_SIDE);
            //cout << "New Channel Start" << endl;

            datamsg testMessage(5, 0.32, 1);
            newChannel.cwrite(&testMessage, sizeof(datamsg));

            double received = 0.0;
            newChannel.cread(&received, sizeof(double));
            cout << "The ecg 1 value for person 5 at time 0.32 was: " << received << endl;

            //would be good to close the new channel when done
            MESSAGE_TYPE close = QUIT_MSG;
            newChannel.cwrite(&close, sizeof(MESSAGE_TYPE));
            //cout << "New Channel will end " << endl;
        }
        MESSAGE_TYPE m = QUIT_MSG;
        chan.cwrite(&m, sizeof(MESSAGE_TYPE));
        // wait(NULL);
        usleep(1000000);
    }
}