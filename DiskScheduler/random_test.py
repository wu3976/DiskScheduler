import sys, os, random
from batch_test import batch_testing

def generateRandFile(fname, reqnum):
    fi = open(fname, "w+")
    for i in range(reqnum):
        fi.write(f"{random.randint(0, 999)}\n")
    fi.close()
    
def generateRandFiles(filenum, reqnum_limit):
    for i in range(filenum):
        reqnum = random.randint(0, reqnum_limit)
        fname = f"random/{i}_{reqnum}.txt"
        generateRandFile(fname, reqnum)

def randomize(capacities_limit, filenum, reqnum_limit, itr, seed_in_name):
    if not os.path.isdir("random"):
        os.mkdir("random")
        generateRandFiles(filenum, reqnum_limit)
    if not os.path.isdir("random_result"):
        os.mkdir("random_result")
    for i in range(itr):
        batch_testing(list(range(1, capacities_limit)), ["random/" + ele for ele in os.listdir("random")], seed_in_name, f"random_result/itr_{i}.txt")
    
if __name__ == "__main__":
    _, capacities_limit, filenum, reqnum_limit, itr = sys.argv
    randomize(int(capacities_limit), int(filenum), int(reqnum_limit), int(itr), 0)
