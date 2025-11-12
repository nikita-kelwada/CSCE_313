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
    unique_lock<mutex> lock(mtx); //needed for locking the queue

    if (done){ //check if pool is stopped
        cout << "Cannot added task to queue" << endl;
        return;
    }
    //TODO: Add task to queue, make sure to lock the queue
    task->name = name; 
    queue.push_back(task); //add task to queue
    num_tasks_unserviced++; //increment unserviced task count

    lock.unlock(); //unlock before notifying
    cout << "Added task" << endl;
    cv.notify_one(); //notify one thread that a task is available

}

//TODO -> DONE
void ThreadPool::run_thread() {
    while (true) {
        Task *task = nullptr;
        { //start scope block for lock
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [this]() { return done || !queue.empty(); }); //wait until there's a task or done is true

            //TODO1: if done and no tasks left, break
            if (done && queue.empty()) {//check if done and no tasks left
                lock.unlock(); //unlock before breaking
                cout << "Stopping thread " << this_thread::get_id() << endl; 
                break;
            }

            //TODO2: if no tasks left, continue
            if (queue.empty()) { //check if no tasks left
                continue;
            }
            //TODO3: get task from queue, remove it from queue, and run it
            task = queue.front(); //get task from queue
            queue.erase(queue.begin()); //remove it from queue
            task->running = true; //mark task as running
        }//end scope block for lock

            cout << "Started task " << task->name << endl;
            task->Run();
            cout << "Finished task " << task->name << endl;

            { //start scope block for lock
                unique_lock<mutex> lock(mtx);
                task->running = false; //mark task as not running
                num_tasks_unserviced--; //decrement unserviced task count
            } //end scope block for lock
            //TODO4: delete task
            delete task; //delete task
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
    //TODO: Delete threads, but remember to wait for them to finish first
    cout << "Called Stop()" << endl;
    
    {//scope block for lock
        unique_lock<mutex> lock(mtx);
        done = true;
    }//scope block ends, lock is released
    
    cv.notify_all();
    
    for (thread *t : threads) {
        if (t->joinable()) {
            t->join();
        }
    }
}