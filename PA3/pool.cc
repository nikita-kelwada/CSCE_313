//Nikita Kelwada CSCE 313 PA3

#include "pool.h"
#include <mutex>
#include <iostream>

using namespace std; //added to remove the std:: prefixes later in the code

Task::Task() = default;
Task::~Task() = default;

ThreadPool::ThreadPool(int num_threads) {
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(new thread(&ThreadPool::run_thread, this));
    }
}

ThreadPool::~ThreadPool() {
    for (thread *t: threads) {
        delete t;
    }
    threads.clear();

    for (Task *q: queue) {
        delete q;
    }
    queue.clear();
}

//TODO -> DONE
void ThreadPool::SubmitTask(const string &name, Task *task) {
    mtx.lock(); //needed for locking the queue

    if (done){ //check if pool is stopped
        mtx.unlock();
        cout << "Cannot added task to queue" << endl;
        return;
    }
    //TODO: Add task to queue, make sure to lock the queue
    task->name = name; 
    queue.push_back(task); //add task to queue
    num_tasks_unserviced++; //increment unserviced task count

    mtx.unlock(); //unlock before notifying
    cout << "Added task " << name << endl;
}

//TODO -> DONE
void ThreadPool::run_thread() {
    while (true) {

        mtx.lock(); //lock the queue
        if (done && queue.empty()) { //check if done and no tasks left
            mtx.unlock(); //unlock before breaking
            cout << "Stopping thread " << this_thread::get_id() << endl;
            break;
        }
        
        if (queue.empty()) { //check if no tasks in queue
            mtx.unlock();
            this_thread::sleep_for(chrono::milliseconds(10));
            continue;
        }

        Task *task = queue.front(); //get the first task
        queue.erase(queue.begin()); //remove it from the queue
        task->running = true;
        mtx.unlock();

        cout << "Started task " << task->name << endl;
        task->Run();
        cout << "Finished task " << task->name << endl;

        mtx.lock(); //lock to update task status
        task->running = false; //mark task as not running
        num_tasks_unserviced--; //decrement unserviced task count
        mtx.unlock(); //unlock after updating

        delete task;
    }
}


// Remove Task t from queue if it's there
void ThreadPool::remove_task(Task *t) {
    mtx.lock();
    for (auto it = queue.begin(); it != queue.end();) {
        if (*it == t) {
            queue.erase(it);
            mtx.unlock();
            return;
        }
        ++it;
    }
    mtx.unlock();
}

//TODO -> DONE
void ThreadPool::Stop() {
    cout << "Called Stop()" << endl;
    
    mtx.lock(); //lock to set done flag
    done = true;
    mtx.unlock();
    
    for (thread *t : threads) { //join all threads
        if (t->joinable()) {
            t->join();
        }
    }
}