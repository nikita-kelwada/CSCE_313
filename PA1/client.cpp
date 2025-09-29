/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Nikita Kelwada
	UIN: 434003223
	Date: 9/27/25 (for new PA requirements)
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/time.h>
#include <iomanip>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

int main(int argc, char *argv[])
{
    // fork the server process first
    int pid = fork();
    
    if (pid == 0) {
        // child process - run the server
        char* server_args[] = {(char*)"server", nullptr};
        execvp("./server", server_args);
    }
    
    // parent process continues here
    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
    
    // instantiat variables for command line arguments
    int opt;
    int p = -1;
    double t = -1.0;
    int e = 0;
    string filename = "";
    bool new_channel = false;
    int buffer_cap = MAX_MESSAGE;
    
    // parse through the command line arguments
    while ((opt = getopt(argc, argv, "p:t:e:f:cm:")) != -1) { 
        switch (opt) {
            case 'p':
                p = atoi(optarg);
                break;
            case 't':
                t = atof(optarg);
                break;
            case 'e':
                e = atoi(optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'c':
                new_channel = true;
                break;
            case 'm':
                cout << "Changing buffer capacity from " << buffer_cap << "..." << endl;
                buffer_cap = atoi(optarg);
                cout << "New buffer capacity: " << buffer_cap << endl;
                break;
        }
    }
    
    // validate inputs
    if (t != -1.0 && (t < 0 || t > 59.996)) { 
        EXITONERROR("Invalid time");
    }
    if (e < 0 || e > 2) {
        EXITONERROR("Invalid ecg value");
    }
    
    // Case 1: request single data point
    if (t != -1.0 && e != 0) {
        if (p < 1 || p > 15) {
            EXITONERROR("Invalid patient");
        }
        // send data message
        datamsg x(p, t, e);
        chan.cwrite(&x, sizeof(datamsg));
        // read reply
        double reply;
        chan.cread(&reply, sizeof(double));
        
        cout << "For person " << p << ", at time " << fixed << setprecision(3) << t 
             << ", the value of ecg " << e << " is " << fixed << setprecision(2) << reply << endl;
    }
    // Case 2: request first 1000 data points for a patient
    else if (p != -1) {
        if (p < 1 || p > 15) {
            EXITONERROR("Invalid patient");
        }
        
        struct timeval start_time;
        gettimeofday(&start_time, NULL);
        
        // create received directory if it doesn't exist
        mkdir("received", 0777);
        
        ofstream outfile;
        string out_filename = "received/x" + to_string(p) + ".csv";
        outfile.open(out_filename);
        
        double time_stamp = 0.0;
        
        // get 1000 data points
        //the for loop works by requesting ecg1 and ecg2 values for the same timestamp in each iteration
        for (int i = 0; i < 1000; i++) {
            // request ecg1
            datamsg msg1(p, time_stamp, 1); 
            chan.cwrite(&msg1, sizeof(datamsg));
            double val1;
            chan.cread(&val1, sizeof(double));
            
            outfile << time_stamp << "," << val1 << ",";
            
            // request ecg2
            datamsg msg2(p, time_stamp, 2);
            chan.cwrite(&msg2, sizeof(datamsg));
            double val2;
            chan.cread(&val2, sizeof(double));
            
            outfile << val2 << endl;
            
            time_stamp += 0.004;
        }
        
        outfile.close();
        
        struct timeval end_time;
        gettimeofday(&end_time, NULL);
        
        double start_usec = (double)start_time.tv_sec * 1000000 + (double)start_time.tv_usec;
        double end_usec = (double)end_time.tv_sec * 1000000 + (double)end_time.tv_usec;
        
        cout << "The data exchange performed took: " << end_usec - start_usec << " microseconds." << endl;
    }
    // Case 3: request a file
    //this part is adapted from client_correct.cpp provided in the project description
    else if (filename != "") {
        // first get the file length
        filemsg len_request(0, 0);
        int msg_len = sizeof(filemsg) + filename.size() + 1;
        char* buf = new char[msg_len];
        memcpy(buf, &len_request, sizeof(filemsg));
        strcpy(buf + sizeof(filemsg), filename.c_str());
        // send file length request
        chan.cwrite(buf, msg_len);
        delete[] buf;
        // read file length
        __int64_t file_length;
        chan.cread(&file_length, sizeof(__int64_t));
        cout << "File lenght: " << file_length << endl;
        
        // open output file
        ofstream outfile;
        string output_name = "received/" + filename;
        outfile.open(output_name, ios::out | ios::binary);
        
        struct timeval start_time;
        gettimeofday(&start_time, NULL);
        
        // request file in chunks
        //the buffer_cap variable is used to determine the chunk size
        __int64_t offset = 0;
        char* data_buf = new char[buffer_cap];
        
        while (offset < file_length) {
            __int64_t remaining = file_length - offset;
            int chunk_size = (remaining < buffer_cap) ? remaining : buffer_cap;
            
            filemsg file_request(offset, chunk_size);
            int request_len = sizeof(filemsg) + filename.size() + 1;
            char* request_buf = new char[request_len];
            memcpy(request_buf, &file_request, sizeof(filemsg));
            strcpy(request_buf + sizeof(filemsg), filename.c_str());
            
            // send file request
            chan.cwrite(request_buf, request_len);
            delete[] request_buf;
            
            // read file data
            chan.cread(data_buf, buffer_cap);
            outfile.write(data_buf, chunk_size);
            // update offset
            offset += chunk_size;
        }
    
        delete[] data_buf;
        outfile.close();
        
        struct timeval end_time;
        gettimeofday(&end_time, NULL);
        // calculate time taken
        double start_usec = (double)start_time.tv_sec * 1000000 + (double)start_time.tv_usec;
        double end_usec = (double)end_time.tv_sec * 1000000 + (double)end_time.tv_usec;
        
        cout << "The data exchange performed took: " << end_usec - start_usec << " microseconds" << endl;
    }
    // Case 4: create new channel
    else if (new_channel) {
        MESSAGE_TYPE msg_type = NEWCHANNEL_MSG;
        chan.cwrite(&msg_type, sizeof(MESSAGE_TYPE));
        // read new channel name
        char new_chan_name[30];
        chan.cread(new_chan_name, buffer_cap);
        
        FIFORequestChannel new_chan(new_chan_name, FIFORequestChannel::CLIENT_SIDE);
        
        // test the new channel with a data request
        datamsg test(5, 0.32, 1);
        new_chan.cwrite(&test, sizeof(datamsg));
        
        double result;
        new_chan.cread(&result, sizeof(double));
        cout << "The ecg 1 value for person 5 at time 0.32 was: " << result << endl;
        
        // close new channel
        MESSAGE_TYPE quit = QUIT_MSG;
        new_chan.cwrite(&quit, sizeof(MESSAGE_TYPE));
    }
    
    // close control channel
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
    
    // wait for server to finish
    usleep(1000000);
    
    return 0;
}