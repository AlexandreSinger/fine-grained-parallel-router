#!/usr/bin/python3

import copy
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

def get_min_chan_width(circuit, config_dir):
    min_chan_width_file = config_dir + "/min_w.txt"
    with open(min_chan_width_file, 'r') as f:
        for line in f:
            if line.startswith(circuit):
                min_chan_width = line.split(" ", 1)[1].strip()
                assert(min_chan_width != "")
                return int(min_chan_width)
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

    default_tests_reference_dir = "/home/singera8/research/fine-grained-parallel-router/testing/tests"
    parser.add_argument(
        "-tests-reference-dir-base",
        default=default_tests_reference_dir,
        type=str,
        metavar="TESTS_REFERENCE_DIR_BASE",
    )


    parser.add_argument(
        "-extra-vpr-args",
        default="",
        type=str,
        metavar="EXTRA_VPR_ARGS",
    )

    parser.add_argument(
        "-run-at-min-chan-width",
        action='store_true'
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
    extra_vpr_args = thread_args[6]

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

    # Check if the rr graph file exists and if so add it to the args
    rr_graph_args = []
    rr_graph_file = circuit_base + ".rr_graph.bin"
    if os.path.isfile(rr_graph_file):
        rr_graph_args = ["--read_rr_graph", rr_graph_file]

    # Check if the router lookahead file exists and if so add it to the args
    router_lookahead_args = []
    router_lookahead_file = circuit_base + ".router_lookahead.capnp"
    if os.path.isfile(rr_graph_file):
        router_lookahead_args = ["--read_router_lookahead", router_lookahead_file]

    # Run the process with the correct arguments
    process = Popen([vpr_exec,
        arch,
        circuit,
        "--net_file", net_file,
        "--place_file", place_file,
        "--route",
        "--analysis"] + config_args + sdc_args + rr_graph_args + router_lookahead_args + extra_vpr_args,
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

    tests_reference_dir = args.tests_reference_dir_base + "/" + args.test_name
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

    # Handle the extra vpr args passed in by the user
    extra_vpr_args = args.extra_vpr_args.split(" ")
    if (len(extra_vpr_args) != 0 and extra_vpr_args[0] == ""):
        extra_vpr_args = []

    thread_args = []
    for circuit in circuits:
        circuit_path = arch_dir + "/" + circuit
        circuit_common_path = circuit_path + "/common"
        os.mkdir(circuit_path)
        os.mkdir(circuit_common_path)

        circuit_extra_vpr_args = copy.deepcopy(extra_vpr_args)
        if args.run_at_min_chan_width:
            # get the minimum channel width
            min_chan_width = get_min_chan_width(circuit, config_dir)
            assert(min_chan_width != None)
            circuit_extra_vpr_args += ["--route_chan_width", str(min_chan_width)];

        thread_args.append([reference_dir + "/" + circuit + "/common", circuit_common_path, circuit, arch, args.vtr_dir, config_dir + "/config.txt", circuit_extra_vpr_args])

    pool = Pool(args.j)
    pool.map(run_vpr_route, thread_args)
    pool.close()

    # Commented out since it was hardly working and never used.
    # run_parse_vtr_task(test_dir, args.vtr_dir)

    # Collect the runtimes, CPD, and wirelengths of the circuits
    runtimes = dict()
    cpds = dict()
    wls = dict()
    min_chan_widths = dict()
    magic_cookies = dict()
    for circuit in circuits:
        runtimes[circuit] = []
        cpds[circuit] = []
        wls[circuit] = []
        min_chan_widths[circuit] = []
        magic_cookies[circuit] = []

        circuit_path = arch_dir + "/" + circuit
        circuit_common_path = circuit_path + "/common"
        vpr_out_file = circuit_common_path + "/vpr.out"
        with open(vpr_out_file, 'r') as f:
            routing_time_pattern = r"Routing took (\d+\.\d+) seconds.*max_rss (\d+\.\d+) MiB"
            cpd_pattern = r"Critical path: (\d+\.\d+) ns"
            wl_pattern = r"Total wirelength: (\d+), average net length"
            min_chan_width_pattern = r"Best routing used a channel width factor of (\d+)."
            magic_cookie_pattern = r"Serial number \(magic cookie\) for the routing is: (-?\d+)"
            for line in f:
                routing_time_match = re.search(routing_time_pattern, line)
                if routing_time_match:
                    time_taken = float(routing_time_match.group(1))
                    max_rss = float(routing_time_match.group(2))
                    runtimes[circuit].append(time_taken)
                cpd_match = re.search(cpd_pattern, line)
                if cpd_match:
                    cpd = float(cpd_match.group(1))
                    cpds[circuit].append(cpd)
                wl_match = re.search(wl_pattern, line)
                if wl_match:
                    wl = int(wl_match.group(1))
                    wls[circuit].append(wl)
                min_chan_width_match = re.search(min_chan_width_pattern, line)
                if min_chan_width_match:
                    min_chan_width = int(min_chan_width_match.group(1))
                    min_chan_widths[circuit].append(min_chan_width)
                magic_cookie_match = re.search(magic_cookie_pattern, line)
                if magic_cookie_match:
                    magic_cookie = int(magic_cookie_match.group(1))
                    magic_cookies[circuit].append(magic_cookie)

    # Quick safety check to ensure that the circuits have routed.
    # Assumption: For a circuit to route, it must have a CPD
    all_circuits_have_routed = True
    has_min_chan_widths = False
    for circuit in circuits:
        if len(cpds[circuit]) == 0:
            print("{circuit}")
            all_circuits_have_routed = False
        if len(min_chan_widths[circuit]) != 0:
            has_min_chan_widths = True

    if not all_circuits_have_routed:
        print("ERROR: Not all circuits routed successfully!")
        return

    # Calculate the interesting information
    geomean_runtime = 1
    geomean_cpd = 1
    geomean_wl = 1
    geomean_min_chan_width = 1
    magic_number = 0
    # Note: This needs to be sorted so the magic number always returns the correct
    #       magic number regardless of machine.
    for circuit in sorted(circuits):
        # Calculate the geomeans
        cpd = cpds[circuit][-1]
        runtime = runtimes[circuit][-1]
        wl = wls[circuit][-1]
        geomean_cpd *= cpd
        geomean_runtime *= runtime
        geomean_wl *= wl
        if has_min_chan_widths:
            min_chan_width = min_chan_widths[circuit][-1]
            geomean_min_chan_width *= min_chan_width
        # Compute the magic number (used to quickly check determinism)
        # Note: we use the previous magic number in a tuple to make enforce order.
        for cpd in cpds[circuit]:
            magic_number = hash((magic_number, cpd))
        for wl in wls[circuit]:
            magic_number = hash((magic_number, wl))
        for magic_cookie in magic_cookies[circuit]:
            magic_number = hash((magic_number, magic_cookie))
    count = len(circuits)
    geomean_runtime = geomean_runtime ** (1.0 / count)
    geomean_cpd = geomean_cpd ** (1.0 / count)
    geomean_wl = geomean_wl ** (1.0 / count)
    geomean_min_chan_width = geomean_min_chan_width ** (1.0 / count)

    print("*" * 30)
    print("*     Routing Information    *")
    print("*" * 30)
    if has_min_chan_widths:
        print("Circuit:\tCPD(ns)\tRun-time(s)\tWirelength\tMagic-cookie\tmin_chan_width")
    else:
        print("Circuit:\tCPD(ns)\tRun-time(s)\tWirelength\tMagic-cookie")
    for circuit in circuits:
        cpd = cpds[circuit][-1]
        runtime = runtimes[circuit][-1]
        wl = wls[circuit][-1]
        magic_cookie = magic_cookies[circuit][-1]
        if has_min_chan_widths:
            min_chan_width = min_chan_widths[circuit][-1]
            print(f"{circuit}:\t{cpd}\t{runtime}\t{wl}\t{magic_cookie}\t{min_chan_width}")
        else:
            print(f"{circuit}:\t{cpd}\t{runtime}\t{wl}\t{magic_cookie}")

    if has_min_chan_widths:
        print(f"Geomean:\t{geomean_cpd}\t{geomean_runtime}\t{geomean_wl}\t{hex(magic_number)}\t{geomean_min_chan_width}")
    else:
        print(f"Geomean:\t{geomean_cpd}\t{geomean_runtime}\t{geomean_wl}\t{hex(magic_number)}")

if __name__ == "__main__":
    run_test_main(sys.argv[1:])

