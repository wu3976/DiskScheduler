import sys

# TODO: modify the marking of decrement active thread. 
# the verifier does not check I/O error.
def verify(req_queue_capacity, fname):
    seq = []
    f = open(fname, "r");
    while True:
        line = f.readline();
        if len(line) == 0:
            break;
        tokens = line.strip().split()
        if tokens[0] == "requester":
            seq.append(["r", int(tokens[1]), int(tokens[3])])
        elif tokens[0] == "service":
            seq.append(["s", int(tokens[2]), int(tokens[4]), False])
    
    # label the last request of each user
    last_checker = set()
    for i in reversed(range(len(seq))):
        if seq[i][0] == "s" and seq[i][1] not in last_checker:
            seq[i][3] = True
            last_checker.add(seq[i][1])
    
    #print("event sequence: ", end="")
    #print(seq)
    
    # check
    waiting_queue = {}
    last_track = 0
    active_thread_count = len(last_checker)
    for i in range(len(seq)):
        #print(f"event {i} rw head {last_track} {seq[i][0]} ")
        #print(f"waiting queue: {waiting_queue}") # debug
        # case request
        if seq[i][0] == "r":
            if len(waiting_queue) >= min(req_queue_capacity, active_thread_count):
                print("request made to a full request queue")
                return 1
            if seq[i][1] in waiting_queue:
                print("multiple request of same user in the waiting queue")
                return 1
            waiting_queue[seq[i][1]] = seq[i][2]
        # case service
        else:
            if len(waiting_queue) < min(req_queue_capacity, active_thread_count):
                print("a request is serviced while waiting queue is not full")
                return 1
            if seq[i][1] not in waiting_queue or waiting_queue[seq[i][1]] != seq[i][2]:
                print("a not-in-queue request is served")
                return 1
            smallest_diff = 1000
            # find the smallest distance from last track to those
            for k, v in waiting_queue.items():
                smallest_diff = min(smallest_diff, abs(v - last_track))
            if smallest_diff != abs(seq[i][2] - last_track):
                print(f"expected serving request of distance {smallest_diff}, got serving requester {seq[i][1]} track {seq[i][2]} with distance {abs(seq[i][2] - last_track)}\nthe serving of IO request is not optimal")
                return 1
            last_track = seq[i][2]
            active_thread_count -= 1 if seq[i][3] else 0
            del waiting_queue[seq[i][1]]
    
    if len(waiting_queue) > 0:
        print("some requests aren\'t processed")

    print("no error found")
    return 0

if __name__ == "__main__":
    _, req_queue_capacity, fname = sys.argv
    verify(int(req_queue_capacity), fname)
