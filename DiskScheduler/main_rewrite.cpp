//
//  main_rewrite.cpp
//  DiskScheduler
//
//  Created by 吴辰杰 on 9/20/21.
//

#include "thread.h"
#include "disk.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <stdlib.h>
#include <cassert>
#include <cmath>

// send request need to know:
//  1. filename of track sequence
//  2. requester #.
struct RequesterParams {
    std::string fname;
    uint32_t requester;
};

// a struct of request body.
struct RequestBody {
    // track requested to access
    int track;
    // whether this is the last request of a requester.
    bool last;
};

struct Disk {
    // message queue for IO requests
    std::unordered_map<uint32_t, RequestBody> req_queue;
    // specifys maximum capacity of request queue
    size_t queue_capacity;
    // R/W head posision
    int rw_head;
};

// RAII cult
class Lock {
    mutex &m;
public:
    Lock(mutex &m_in): m(m_in) {
        m.lock();
    }
    ~Lock() {
        m.unlock();
    }
};

struct CmdArg {
    int argc;
    const char **argv;
};

//------------------- GLOBAL vars (Shared concurrency states) --------------------

// track the number of active requesters
uint32_t active_requester_count;

// initalize the disk
Disk disk;

//--------------------------------------------------------------------------------

//---------- Monitor--------------
// only mutex
mutex m;

// CVs that see if waiting queue is filled to the most extent
cv queue_bestfilled, queue_underfilled;
//--------------------------------

/**
 Must be called in a mutex seesion
 */
bool is_underfilled() {
    return disk.req_queue.size() < std::min(static_cast<size_t>(active_requester_count), disk.queue_capacity);
}

/**
 return the requester of one of closet request
 must be called in a mutex session
 */
uint32_t find_closet_track_requester() {
    uint32_t best_requester = 10000000;
    int smallest_diff = 1000;
    for (auto ele: disk.req_queue) {
        int curr_diff = std::abs(ele.second.track - disk.rw_head);
        if (curr_diff < smallest_diff) {
            smallest_diff = curr_diff;
            best_requester = ele.first;
        }
    }
    return best_requester;
}

void process_request(void *_) {
    Lock l(m);
    while (disk.req_queue.size() || active_requester_count) {
        while (is_underfilled()) {
            queue_bestfilled.wait(m);
        }
        int best_requester = find_closet_track_requester(),
            best_track = disk.req_queue[best_requester].track;
        print_service(best_requester, best_track);
        disk.rw_head = best_track;
        active_requester_count -= disk.req_queue[best_requester].last;
        disk.req_queue.erase(best_requester);
        queue_underfilled.broadcast();
    }
}

void send_request(void *param) {
    Lock l(m);
    RequesterParams *p = (RequesterParams *)param;
    std::ifstream in(p->fname);
    std::vector<int> tracks;
    int temp;
    while (in >> temp) {
        tracks.push_back(temp);
    }
    for (size_t i = 0; i < tracks.size(); i++) {
        while (!is_underfilled() || disk.req_queue.find(p->requester) != disk.req_queue.end()) {
            queue_underfilled.wait(m);
        }
        print_request(p->requester, tracks[i]);
        disk.req_queue[p->requester] = RequestBody{tracks[i], i == tracks.size() - 1};
        queue_bestfilled.signal();
    }
}

void booting(void *arg) {
    CmdArg *cmdarg_ptr = (CmdArg *)arg;
    int argc = cmdarg_ptr->argc;
    const char **argv = cmdarg_ptr->argv;
    disk.queue_capacity = static_cast<size_t>(std::atoi(argv[1]));
    
    // set the initial states
    active_requester_count = static_cast<uint32_t>(argc - 2);
    disk.rw_head = 0;
    disk.queue_capacity = static_cast<size_t>(atoi(argv[1]));
    
    // check if any of input files are empty
    for (int i = 2; i < argc; i++) {
        std::ifstream in((std::string(argv[i])));
        if (in.peek() == std::istream::traits_type::eof()) {
            active_requester_count--;
        }
        in.close();
    }
    
    // instigate the scheduler thread
    thread((thread_startfunc_t) process_request, (void *)nullptr);
    
    // instigate each of the requester threads
    for (int i = 2; i < argc; i++) {
        RequesterParams *params_ptr= new RequesterParams{std::string(argv[i]),
            static_cast<uint32_t>(i - 2)};
        thread((thread_startfunc_t) send_request, (void *) params_ptr);
    }
}

/* The first argument specifies the maximum number of requests that the disk queue can hold.
 The rest of the arguments specify a list of input files (one input file per requester).*/
int main(int argc, const char * argv[]) {
    cpu::boot((thread_startfunc_t) booting, (void *) new CmdArg{argc, argv}, 0);
}
