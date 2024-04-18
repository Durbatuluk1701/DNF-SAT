import subprocess

import psutil


def run_prog(prog: str) -> tuple[float, float]:
    process = subprocess.Popen(
        prog, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )

    # Get the time the process took to run and store in user_time and system_time variables
    user_time = 0
    system_time = 0
    while process.poll() is None:
        cpu_times = psutil.Process(process.pid).cpu_times()
        user_time = cpu_times.user
        system_time = cpu_times.system

    return user_time, system_time


NUM_ITERS = 1000
MAX_SIZE = 20
DNF = "/home/w732t351/repos/dnf-sat/src/converter_new_opt"
PAR = "/home/w732t351/repos/dnf-sat/src/converter_new_par_opt"
Z3 = "/usr/bin/z3"

print(
    f"Size, User Time - DNF, Sys Time - DNF, Total Time - DNF, User Time - PAR, Sys Time - PAR, Total Time - PAR, User Time - Z3, Sys Time - Z3, Total Time - Z3"
)
for i in range(1, MAX_SIZE + 1):
    print(f"{i},", end="")
    prog = f"{DNF} /home/w732t351/repos/dnf-sat/test_suite/test_MC_{i}.cnf > /dev/null"
    user_time, system_time = 0, 0
    for _ in range(NUM_ITERS):
        user_time_iter, system_time_iter = run_prog(prog)
        user_time += user_time_iter
        system_time += system_time_iter
    user_time /= NUM_ITERS
    system_time /= NUM_ITERS
    total_time = user_time + system_time
    print(f"{user_time:.10f}, {system_time:.10f}, {total_time:.10f}, ", end="")
    prog = f"{PAR} /home/w732t351/repos/dnf-sat/test_suite/test_MC_{i}.cnf > /dev/null"
    user_time, system_time = 0, 0
    for _ in range(NUM_ITERS):
        user_time_iter, system_time_iter = run_prog(prog)
        user_time += user_time_iter
        system_time += system_time_iter
    user_time /= NUM_ITERS
    system_time /= NUM_ITERS
    total_time = user_time + system_time
    print(f"{user_time:.10f}, {system_time:.10f}, {total_time:.10f}, ", end="")
    prog = f"{Z3} /home/w732t351/repos/dnf-sat/test_suite/test_MC_{i}.cnf > /dev/null"
    user_time, system_time = 0, 0
    for _ in range(NUM_ITERS):
        user_time_iter, system_time_iter = run_prog(prog)
        user_time += user_time_iter
        system_time += system_time_iter
    user_time /= NUM_ITERS
    system_time /= NUM_ITERS
    total_time = user_time + system_time
    print(f"{user_time:.10f}, {system_time:.10f}, {total_time:.10f}")
