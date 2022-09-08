import sys, os
from verifier import verify

# test all capacities of the files
def batch_testing(capacities, filenames, seed_in_name, fout):
    resultf = open(fout, "w+")
    for c in capacities:
        os.system("make clean")
        os.system("make scheduler")
        file_str = " ".join(filenames)
        outfilename = f"c{c}f{len(filenames)}s{seed_in_name}_.txt"
        os.system(f"./scheduler {c} {file_str} > {outfilename}")
        result = verify(c, outfilename)
        if result == 0:
            resultf.write(f"capacity {c} passes\n")
        else:
            resultf.write(f"capacity {c} failed\n")
        os.system("make clean")

if  __name__ == "__main__":
    # python3 batch_test.py cap_from cap_til seed [files]
    capacities = list(range(int(sys.argv[1]), int(sys.argv[2])))
    filenames = list(sys.argv[4:])
    seed_in_name = sys.argv[3]
    batch_testing(capacities, filenames, seed_in_name, "result.txt")
