import subprocess
import psutil
import statistics


# Function to run memory profiling on the separate C program
def profile_memory(args: list[str]):
    cmd = ["/usr/bin/z3"] + args
    process = subprocess.Popen(
        cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    process_pid = process.pid
    # Track memory usage of the process
    memory_usage = {
        "rss": float("-inf"),
        "vms": float("-inf"),
        "shared": float("-inf"),
        "text": float("-inf"),
        "lib": float("-inf"),
        "data": float("-inf"),
        "dirty": float("-inf"),
        "uss": float("-inf"),
        "pss": float("-inf"),
        "swap": float("-inf"),
    }
    while process.poll() is None:
        memory_info = psutil.Process(process_pid).memory_full_info()
        for key, val in memory_info._asdict().items():
            if val > memory_usage[key]:
                memory_usage[key] = val
    return memory_usage


NUM_ITERS = 100
MAX_SIZE = 15

print(f"Size, rss, vms, shared, text, lib, data, dirty, uss, pss, swap")
for i in range(1, MAX_SIZE + 1):
    args = [f"/home/w732t351/repos/dnf-sat/test_suite/test_MC_{i}.cnf"]
    memory_profiles = {
        "rss": [],
        "vms": [],
        "shared": [],
        "text": [],
        "lib": [],
        "data": [],
        "dirty": [],
        "uss": [],
        "pss": [],
        "swap": [],
    }
    for _ in range(NUM_ITERS):
        memory_usage = profile_memory(args)
        for key, val in memory_usage.items():
            memory_profiles[key].append(val)
    final_profile = {
        "rss": float("-inf"),
        "vms": float("-inf"),
        "shared": float("-inf"),
        "text": float("-inf"),
        "lib": float("-inf"),
        "data": float("-inf"),
        "dirty": float("-inf"),
        "uss": float("-inf"),
        "pss": float("-inf"),
        "swap": float("-inf"),
    }
    for key in memory_profiles:
        final_profile[key] = statistics.mean(memory_profiles[key])
    print(
        f"{i}, {final_profile['rss']}, {final_profile['vms']}, {final_profile['shared']}, {final_profile['text']}, {final_profile['lib']}, {final_profile['data']}, {final_profile['dirty']}, {final_profile['uss']}, {final_profile['pss']}, {final_profile['swap']}"
    )
    # print("Memory Profile", final_profile)

# Calculate average memory usage across all profiles
# for i in range(1, MAX_SIZE + 1):
#     user_time, system_time = 0, 0
#     for _ in range(NUM_ITERS):
#         user_time_iter, system_time_iter = run_prog(prog)
#         user_time += user_time_iter
#         system_time += system_time_iter
#     user_time /= NUM_ITERS
#     system_time /= NUM_ITERS
#     print(f"{i}, {user_time:.10f}, {system_time:.10f}")
