#!/usr/bin/python3

import os
import re
import sys
import argparse
from multiprocessing import Pool
from subprocess import Popen, PIPE
import pathlib
import shutil

# Helper method to get the architecture file name from a directory.
def get_arch_file_from_dir(dir_path):
    res = []
    for f in os.listdir(dir_path):
        if f.endswith(".xml"):
            res.append(f)
    assert(len(res) == 1)
    return res[0]

# Helper method to parse the config file for vpr command line arguments.
def get_vpr_args_from_config(config_file):
    with open(config_file, 'r') as f:
        for line in f:
            if line.startswith("script_params="):
                script_params = line.split("=", 1)[1].strip()
                if script_params == "":
                    return []
                # Split the script_params string into individual parameters
                params_list = script_params.split(" ")
                return params_list
    return None

# Helper method to extract the run number from the given run directory name.
# For example "run003" would return 3.
def extract_run_number(run):
    if run is None:
        return None
    match = re.match(r"run(\d+)", run)
    if match:
        return int(match.group(1))
    else:
        return None

# Helper method to generate a parser for the command line interface.
def command_parser(prog=None):
    description = "Runs a given test."
    parser = argparse.ArgumentParser(
        prog=prog,
        description=description,
        epilog="",
    )

    parser.add_argument("test_name")

    parser.add_argument(
        "-j",
        default=1,
        type=int,
        metavar="NUM_PROC",
    )

    default_vtr_dir = os.path.expanduser("~") + "/vtr-verilog-to-routing"
    parser.add_argument(
        "-vtr_dir",
        default=default_vtr_dir,
        type=str,
        metavar="VTR_DIR",
    )

    return parser

# Run a single circuit through VPR route flow.
# Uses pre-existing netlist and placement to save time for the whole flow.
def run_vpr_route(thread_args):
    # Parse the thread arguments
    reference_dir = thread_args[0]
    working_dir = thread_args[1]
    circuit_name = thread_args[2]
    arch_name = thread_args[3]
    vtr_dir = thread_args[4]
    config_file = thread_args[5]

    # Change directory to the working directory
    os.chdir(working_dir)

    # Get the vpr executable and other required information.
    vpr_exec = vtr_dir + "/vpr/vpr"
    arch = reference_dir + "/../../../arch/" + arch_name
    circuit = reference_dir + "/" + circuit_name
    circuit_base, _ = os.path.splitext(circuit)
    sdc_file = circuit_base + ".sdc"
    place_file = circuit_base + ".place"
    net_file = circuit_base + ".net"

    config_args = get_vpr_args_from_config(config_file)

    # Check if an sdc file exists and if so add it to the args
    sdc_args = []
    if os.path.isfile(sdc_file):
        sdc_args = ["--sdc_file", sdc_file]

    # Run the process with the correct arguments
    process = Popen([vpr_exec,
        arch,
        circuit,
        "--net_file", net_file,
        "--place_file", place_file,
        "--route",
        "--analysis"] + config_args + sdc_args,
        stdout=PIPE,
        stderr=PIPE)

    # Store the output of the command
    stdout, stderr = process.communicate()

    with open("vpr.out", "w") as f:
        f.write(stdout.decode())
        f.close()

    with open("vpr_err.out", "w") as f:
        f.write(stderr.decode())
        f.close()

    print(f"{circuit_name} is done!")

# Helper method to parse the QoR and Runtimes of the run.
def run_parse_vtr_task(test_dir, vtr_dir):
    parse_vtr_task_exec = vtr_dir + "/vtr_flow/scripts/python_libs/vtr/parse_vtr_task.py"

    print("Parsing VTR Task...")
    process = Popen([parse_vtr_task_exec,
        test_dir])
    process.communicate()
    print("Parse VTR Task Completed")

def run_test_main(arg_list, prog=None):
    # Load the arguments
    args = command_parser(prog).parse_args(arg_list)

    tests_reference_dir = "/home/singera8/research/fine-grained-parallel-router/testing/tests/" + args.test_name
    if not os.path.isdir(tests_reference_dir):
        print("Invalid test")
        return

    arch = get_arch_file_from_dir(tests_reference_dir + "/arch")

    reference_dir = tests_reference_dir + "/" + arch
    circuits = os.listdir(reference_dir)
    
    script_dir = str(pathlib.Path(__file__).parent.resolve())
    test_dir = script_dir + "/" + args.test_name
    os.makedirs(test_dir, exist_ok=True)
    config_dir = test_dir + "/config"
    if not os.path.isdir(config_dir):
        shutil.copytree(reference_dir + "/../config", config_dir)

    # Get the run name
    runs = os.listdir(test_dir)
    last_run_num = 0
    if len(runs) is not 0:
        last_run = sorted(runs)[-1]
        last_run_num = extract_run_number(last_run)
        if last_run_num is None:
            last_run_num = 0
    run_num = last_run_num + 1
    run_name = "run{:03d}".format(run_num)
    run_dir = test_dir + "/" + run_name
    os.mkdir(run_dir)

    arch_dir = run_dir + "/" + arch
    os.mkdir(arch_dir)
    print(arch_dir)

    thread_args = []
    for circuit in circuits:
        circuit_path = arch_dir + "/" + circuit
        circuit_common_path = circuit_path + "/common"
        os.mkdir(circuit_path)
        os.mkdir(circuit_common_path)
        thread_args.append([reference_dir + "/" + circuit + "/common", circuit_common_path, circuit, arch, args.vtr_dir, config_dir + "/config.txt"])

    pool = Pool(args.j)
    pool.map(run_vpr_route, thread_args)
    pool.close()

    run_parse_vtr_task(test_dir, args.vtr_dir)

    # Parse the out files to get data on the run.
    print("*" * 30)
    print("*     Routing Information    *")
    print("*" * 30)
    print("Circuit:\tCPD(ns)\tRun-time(s)\tWirelength")
    geomean_runtime = 1
    geomean_cpd = 1
    geomean_wl = 1
    count = 0
    for circuit in circuits:
        print(circuit, end=':\t')
        circuit_path = arch_dir + "/" + circuit
        circuit_common_path = circuit_path + "/common"
        vpr_out_file = circuit_common_path + "/vpr.out"

        with open(vpr_out_file, 'r') as f:
            routing_time_pattern = r"Routing took (\d+\.\d+) seconds.*max_rss (\d+\.\d+) MiB"
            cpd_pattern = r"Critical path: (\d+\.\d+) ns"
            wl_pattern = r"Total wirelength: (\d+), average net length"
            for line in f:
                routing_time_match = re.search(routing_time_pattern, line)
                if routing_time_match:
                    time_taken = float(routing_time_match.group(1))
                    max_rss = float(routing_time_match.group(2))
                    print(time_taken, end="\t")
                    geomean_runtime *= time_taken
                    count += 1
                cpd_match = re.search(cpd_pattern, line)
                if cpd_match:
                    cpd = float(cpd_match.group(1))
                    print(cpd, end="\t")
                    geomean_cpd *= cpd
                wl_match = re.search(wl_pattern, line)
                if wl_match:
                    wl = int(wl_match.group(1))
                    print(wl, end="\t")
                    geomean_wl *= wl

            print("")
    geomean_runtime = geomean_runtime ** (1.0 / count)
    geomean_cpd = geomean_cpd ** (1.0 / count)
    geomean_wl = geomean_wl ** (1.0 / count)
    print(f"Geomean:\t{geomean_runtime}\t{geomean_cpd}\t{geomean_wl}")

if __name__ == "__main__":
    run_test_main(sys.argv[1:])

