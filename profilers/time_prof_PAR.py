import resource
import os


def run_prog(prog: str) -> tuple[float, float]:
    # Get resource usage before running the program
    usage_before = resource.getrusage(resource.RUSAGE_SELF)

    # Run your program here
    # For example:
    os.system(prog)

    # Get resource usage after running the program
    usage_after = resource.getrusage(resource.RUSAGE_SELF)

    # Calculate CPU times
    user_time = usage_after.ru_utime - usage_before.ru_utime
    system_time = usage_after.ru_stime - usage_before.ru_stime
    return user_time, system_time


NUM_ITERS = 100
MAX_SIZE = 15

print(f"Size, User Time, System Time")
for i in range(1, MAX_SIZE + 1):
    prog = f"/home/w732t351/repos/dnf-sat/src/converter_new_par_opt /home/w732t351/repos/dnf-sat/test_suite/test_MC_{i}.cnf > /dev/null"
    user_time, system_time = 0, 0
    for _ in range(NUM_ITERS):
        user_time_iter, system_time_iter = run_prog(prog)
        user_time += user_time_iter
        system_time += system_time_iter
    user_time /= NUM_ITERS
    system_time /= NUM_ITERS
    print(f"{i}, {user_time:.10f}, {system_time:.10f}")
