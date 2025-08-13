import os
import subprocess
import sys
import json
import re
import os
import datetime

def make_timestamped_directory():
    """
    Creates a new directory with the current timestamp in the format YYYYMMDD-HHMMSS.

    The directory is created in the current working directory.
    If the directory already exists, a message is printed and no new directory is created.

    Returns:
        str: The name of the newly created directory.
    """
    # Get the current time
    now = datetime.datetime.now()

    # Format the time as a string (e.g., 20250811-010038)
    dir_name = now.strftime('%Y%m%d-%H%M%S')

    # Check if the directory already exists
    if not os.path.exists(dir_name):
        # Create the directory
        os.makedirs(dir_name)
        print(f"Directory '{dir_name}' created successfully.")
    else:
        print(f"Directory '{dir_name}' already exists.")
        
    return dir_name

data = dict()

def restolist(result):
    ret = []
    for line in result.split('\n'):
        # print(line)
        if line.startswith("Transaction"):
            v = float(line.split()[-2])
            ret.append(v)
    return ret

dir_name = make_timestamped_directory()
f = open(f"{dir_name}/res_percpu", 'w')
for inc,dec in [(5,0), (10,0), (20,0), (50,0)]:
    print(f"{inc} {dec} ", file=f)
    for rr_llc in [0,1]:
        for a in [0,1]:
            print(f"{'rr_llc' if rr_llc else 'seq'} {'atomic' if a else 'percpuctr'}", file=f)
            d = dict()
            for core in range(1,10):
                try:
                    cmdline = f"./counter_benchmark {core} {a} {rr_llc} {inc} {dec}"
                    result = subprocess.run(cmdline, shell=True, capture_output=True)
                    d[core] = restolist(result.stdout.decode())
                    if len(d[core]) > 0:
                        print(a, core, sum(d[core])/len(d[core]) ,file=f)
                    else:
                        print('no data for ',a,core,file=f)
                except Exception as e:
                    # Handle the exception
                    print(f"An error occurred: {e}",file=f)
                    # You can also access the type of exception
                    print(f"Type of error: {type(e)}",file=f)
                    print("error for ",a,core,file=f)
            data[str((inc,dec,rr_llc,a))] = d



with open(f"{dir_name}/output.json", "w") as json_file:
    json.dump(data, json_file, indent=4)