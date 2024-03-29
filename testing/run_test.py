#!/usr/bin/python3

import os
import re
import sys
import argparse
from multiprocessing import Pool
from subprocess import Popen, PIPE
import pathlib
import shutil

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

    return parser

# Run a single circuit through VPR route flow.
# Uses pre-existing netlist and placement to save time for the whole flow.
def run_vpr_route(thread_args):
    # Parse the thread arguments
    reference_dir = thread_args[0]
    working_dir = thread_args[1]
    circuit_name = thread_args[2]
    arch_name = thread_args[3]

    # Change directory to the working directory
    os.chdir(working_dir)

    # Get the current location of the VTR directory
    # TODO: Make this a command line argument
    vtr_dir = os.path.expanduser("~") + "/vtr-verilog-to-routing"

    # Get the vpr executable and other required information.
    vpr_exec = vtr_dir + "/vpr/vpr"
    arch = reference_dir + "/../../../arch/" + arch_name
    circuit = reference_dir + "/" + circuit_name
    circuit_base, _ = os.path.splitext(circuit)
    sdc_file = circuit_base + ".sdc"
    place_file = circuit_base + ".place"
    net_file = circuit_base + ".net"

    # Run the process with the correct arguments
    process = Popen([vpr_exec,
        arch,
        circuit,
        "--route_chan_width", "300",
        "--max_router_iterations", "400",
        "--router_lookahead", "map",
        "--initial_pres_fac", "1.0",
        "--router_profiler_astar_fac", "1.5",
        "--seed", "3",
        "--sdc_file", sdc_file,
        "--net_file", net_file,
        "--place_file", place_file,
        "--route",
        "--analysis"],
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
def run_parse_vtr_task(test_dir):
    vtr_dir = os.path.expanduser("~") + "/vtr-verilog-to-routing"
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

    reference_dir = tests_reference_dir + "/stratixiv_arch.timing.xml"
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

    arch = "stratixiv_arch.timing.xml"
    arch_dir = run_dir + "/" + arch
    os.mkdir(arch_dir)
    print(arch_dir)

    thread_args = []
    for circuit in circuits:
        circuit_path = arch_dir + "/" + circuit
        circuit_common_path = circuit_path + "/common"
        os.mkdir(circuit_path)
        os.mkdir(circuit_common_path)
        thread_args.append([reference_dir + "/" + circuit + "/common", circuit_common_path, circuit, arch])

    pool = Pool(args.j)
    pool.map(run_vpr_route, thread_args)
    pool.close()

    run_parse_vtr_task(test_dir)

if __name__ == "__main__":
    run_test_main(sys.argv[1:])

