//
//  main.cpp
//  DiskScheduler
//
//  Created by 吴辰杰 on 9/18/21.
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
    uint32_t requester;
    std::string fname;
};

// Disk definition
struct Disk {
    // message queue for IO requests
    std::unordered_map<uint32_t, int> req_queue;
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
mutex cykablyat;

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
 run on thread that rep disk controller
 Process IO does not need parameter
 */
void process_request(void *_) {
    Lock l(cykablyat);
    while (disk.req_queue.size() || active_requester_count) {
        // while the queue is not filled to the best.
        while (is_underfilled()) {
            queue_underfilled.signal();
            queue_bestfilled.wait(cykablyat);
        }
        uint32_t best_requester = 10000000;
        int smallest_diff = 1000;
        for (auto req : disk.req_queue) {
            int diff = std::abs(req.second - disk.rw_head);
            if (diff < smallest_diff) {
                smallest_diff = diff;
                best_requester = req.first;
            }
        }
        print_service(best_requester, disk.req_queue[best_requester]);
        disk.rw_head = disk.req_queue[best_requester];
        disk.req_queue.erase(best_requester);
    }
}

/**
 run on thread that represent requesters
 */
void send_request(void *param) {
    Lock l(cykablyat);
    RequesterParams *p = (RequesterParams *)param;
    std::ifstream in(p->fname);
    //std::cout << p->requester << std::endl;
    int track;
    std::pair<uint32_t, int> last_request{p->requester, -1};
    while (in >> track) {
        // while this requester cannot send more request
        while (disk.req_queue.size() >= disk.queue_capacity
               || disk.req_queue.find(p->requester) != disk.req_queue.end()){
            if (is_underfilled()){ queue_underfilled.signal(); }
            else { queue_bestfilled.broadcast(); }
            queue_underfilled.wait(cykablyat);
        }
        assert(disk.req_queue.size() < disk.queue_capacity);
        assert(disk.req_queue.find(p->requester) == disk.req_queue.end());
        last_request.second = track;
        print_request(p->requester, track);
        disk.req_queue.insert(last_request);
    }
    active_requester_count--;
    if (is_underfilled()){ queue_underfilled.signal(); }
    else { queue_bestfilled.broadcast(); }
}

void booting(void *arg) {
    //std::cout << "here" << std::endl;

    CmdArg *cmdarg_ptr = (CmdArg *)arg;
    int argc = cmdarg_ptr->argc;
    const char **argv = cmdarg_ptr->argv;
    disk.queue_capacity = static_cast<size_t>(std::atoi(argv[1]));
    active_requester_count = static_cast<uint32_t>(argc - 2); // must preset it right now
    // or the scheduler thread could end prematurely.
    disk.rw_head = 0;
    
    thread disk_controller((thread_startfunc_t) process_request, (void *) nullptr);
    for (uint32_t i = 2; i < static_cast<uint32_t>(argc); i++) {
        thread requester((thread_startfunc_t) send_request, (void *) new RequesterParams{
            i - 2,
            std::string(argv[i])
        });
    }
}

/* The first argument specifies the maximum number of requests that the disk queue can hold.
 The rest of the arguments specify a list of input files (one input file per requester).*/
int main(int argc, const char * argv[]) {
    cpu::boot((thread_startfunc_t) booting, (void *) new CmdArg{argc, argv}, 4);
}
