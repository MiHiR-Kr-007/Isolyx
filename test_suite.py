import subprocess
import time
import re
import sys
import threading
import getpass

def run_tests():
    print("Testing Isolyx")
    
    process = subprocess.Popen(
        ['./build/isolyx'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )

    def read_output(proc, output_list):
        for line in iter(proc.stdout.readline, ''):
            if line.strip():
                clean_line = line.replace('isolyx>', '').strip()
                if clean_line:
                    print(f"  {clean_line}")
                    output_list.append(clean_line)

    output_lines = []
    t = threading.Thread(target=read_output, args=(process, output_lines))
    t.daemon = True
    t.start()

    commands = [
        "/while1 &",              # Timeout test
        "/host_file_escape &",    # Host file escape try
        "/denylist_syscall &",    # Seccomp denylist
        "/forkbomb &",            # Fork bomb 
        "/memory_bomb &",         # Memory bomb 
    ]
    
    # 10 concurrent submissions
    for i in range(10):
        commands.append("/while1 &")
        
    for cmd in commands:
        print(f"Submitting: {cmd}")
        process.stdin.write(cmd + '\n')
        process.stdin.flush()
        time.sleep(0.1)

    print("All jobs submitted. Waiting for them to finish.")
    time.sleep(15) # Wait for timeouts to hit
    
    process.stdin.write("exit\n")
    process.stdin.flush()
    process.wait()

    print("\nResults:")
    
    results = {
        'timeout': 0,
        'seccomp': 0,
        'escape': False,
        'forkbomb': False,
        'membomb': False,
        'concurrent': 0
    }

    host_user = getpass.getuser()
    host_user_found = False

    for line in output_lines:
        if host_user in line:
            host_user_found = True
        
        if "Job" in line and "finished" in line:
            if "/while1" in line and "TIMEOUT" in line:
                results['timeout'] += 1
                results['concurrent'] += 1
            if "/denylist_syscall" in line and "SECCOMP_VIOLATION" in line:
                results['seccomp'] += 1
            if "/host_file_escape" in line:
                results['escape'] = True
            if "/forkbomb" in line and ("KILLED" in line or "TIMEOUT" in line):
                results['forkbomb'] = True
            if "/memory_bomb" in line and ("KILLED" in line or "TIMEOUT" in line):
                results['membomb'] = True

    if host_user_found:
        results['escape'] = False
        print(f"ERROR: Host file leaked (found '{host_user}')!")

    all_passed = True

    if results['timeout'] >= 1:
        print("Pass: Timeout test")
    else:
        print("Fail: Timeout test")
        all_passed = False

    if results['seccomp'] == 1:
        print("Pass: Seccomp Denylist Test")
    else:
        print("Fail: Seccomp Denylist Test")
        all_passed = False

    if results['escape']:
        print("Pass: Host File Escape Test")
    else:
        print("Fail: Host File Escape Test")
        all_passed = False

    if results['forkbomb']:
        print("Pass: Fork Bomb Test")
    else:
        print("Fail: Fork Bomb Test")
        all_passed = False

    if results['membomb']:
        print("Pass: Memory Bomb Test")
    else:
        print("Fail: Memory Bomb Test")
        all_passed = False

    if results['concurrent'] >= 10:
        print("Pass: 10+ Concurrent Submissions")
    else:
        print(f"Fail: 10+ Concurrent Submissions (expected 10+ timeouts, got {results['concurrent']})")
        all_passed = False

    if all_passed:
        print("\nAll integration tests PASSED!")
        sys.exit(0)
    else:
        print("\nSome integration tests FAILED.")
        sys.exit(1)

if __name__ == "__main__":
    run_tests()
