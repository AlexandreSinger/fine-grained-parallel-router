#!/usr/bin/python3

import copy
import os
import re
import sys
import argparse
from multiprocessing import Pool
from subprocess import Popen, PIPE, TimeoutExpired
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

    # Timeout in seconds, 0 implies no timeout
    parser.add_argument(
        "-timeout",
        default=0,
        type=float,
        metavar="TIMEOUT",
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
    timeout = thread_args[7]

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
    if timeout == 0.0:
        timeout = None
    circuit_timed_out = False
    # https://docs.python.org/3/library/subprocess.html#subprocess.Popen.communicate
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except TimeoutExpired:
        process.kill()
        stdout, stderr = process.communicate()
        circuit_timed_out = True

    with open("vpr.out", "w") as f:
        f.write(stdout.decode())
        f.close()

    with open("vpr_err.out", "w") as f:
        f.write(stderr.decode())
        f.close()

    if not circuit_timed_out:
        print(f"{circuit_name} is done!")
    else:
        print(f"{circuit_name} timed out after {timeout} seconds!")

# Helper method to parse the QoR and Runtimes of the run.
def run_parse_vtr_task(test_dir, vtr_dir):
    parse_vtr_task_exec = vtr_dir + "/vtr_flow/scripts/python_libs/vtr/parse_vtr_task.py"

    print("Parsing VTR Task...")
    process = Popen([parse_vtr_task_exec,
        test_dir])
    process.communicate()
    print("Parse VTR Task Completed")

class RunData:
    circuit_name: str = None
    cpd: float = None
    wl: int = None
    runtime: float = None
    sssp_runtime: float = None
    min_chan_width: int = None
    vtr_magic_cookie: int = None
    total_magic_cookie: int = None

    def __init__(self, circuit_name):
        self.circuit_name = circuit_name

    def has_min_chan_width(self):
        return (self.min_chan_width != None)

    def has_complete_data(self, should_have_min_chan_width):
        if self.cpd == None:
            return False
        if self.wl == None:
            return False
        if self.runtime == None:
            return False
        if self.sssp_runtime == None:
            return False
        if self.min_chan_width == None and should_have_min_chan_width:
            return False
        if self.vtr_magic_cookie == None:
            return False
        if self.total_magic_cookie == None:
            return False
        return True

    def print(self, has_min_chan_width):
        if not self.has_complete_data(has_min_chan_width):
            if has_min_chan_width:
                print(f"{self.circuit_name}:\t-\t-\t-\t-\t-\t-")
            else:
                print(f"{self.circuit_name}:\t-\t-\t-\t-\t-")
            return
        if has_min_chan_width:
            print(f"{self.circuit_name}:\t{self.cpd}\t{self.runtime}\t{self.sssp_runtime}\t{self.wl}\t{self.vtr_magic_cookie}\t{self.min_chan_width}")
        else:
            print(f"{self.circuit_name}:\t{self.cpd}\t{self.runtime}\t{self.sssp_runtime}\t{self.wl}\t{self.vtr_magic_cookie}")

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
    if len(runs) != 0:
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
    for circuit in sorted(circuits):
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

        thread_args.append([reference_dir + "/" + circuit + "/common", circuit_common_path, circuit, arch, args.vtr_dir, config_dir + "/config.txt", circuit_extra_vpr_args, args.timeout])

    pool = Pool(args.j)
    pool.map(run_vpr_route, thread_args)
    pool.close()

    # Commented out since it was hardly working and never used.
    # run_parse_vtr_task(test_dir, args.vtr_dir)

    # Collect the runtimes, CPD, and wirelengths of the circuits
    circuit_run_data = dict()
    for circuit in sorted(circuits):
        runtimes = []
        sssp_runtimes = []
        cpds = []
        wls = []
        min_chan_widths = []
        magic_cookies = []

        circuit_path = arch_dir + "/" + circuit
        circuit_common_path = circuit_path + "/common"
        vpr_out_file = circuit_common_path + "/vpr.out"
        with open(vpr_out_file, 'r') as f:
            routing_time_pattern = r"Routing took (\d+\.\d+) seconds.*max_rss (\d+\.\d+) MiB"
            sssp_time_pattern = r"Time spent computing SSSP: (\d+\.\d+) seconds"
            cpd_pattern = r"Critical path: (\d+\.\d+) ns"
            wl_pattern = r"Total wirelength: (\d+), average net length"
            min_chan_width_pattern = r"Best routing used a channel width factor of (\d+)."
            magic_cookie_pattern = r"Serial number \(magic cookie\) for the routing is: (-?\d+)"
            for line in f:
                routing_time_match = re.search(routing_time_pattern, line)
                if routing_time_match:
                    time_taken = float(routing_time_match.group(1))
                    max_rss = float(routing_time_match.group(2))
                    runtimes.append(time_taken)
                sssp_time_match = re.search(sssp_time_pattern, line)
                if sssp_time_match:
                    time_taken = float(sssp_time_match.group(1))
                    sssp_runtimes.append(time_taken)
                cpd_match = re.search(cpd_pattern, line)
                if cpd_match:
                    cpd = float(cpd_match.group(1))
                    cpds.append(cpd)
                wl_match = re.search(wl_pattern, line)
                if wl_match:
                    wl = int(wl_match.group(1))
                    wls.append(wl)
                min_chan_width_match = re.search(min_chan_width_pattern, line)
                if min_chan_width_match:
                    min_chan_width = int(min_chan_width_match.group(1))
                    min_chan_widths.append(min_chan_width)
                magic_cookie_match = re.search(magic_cookie_pattern, line)
                if magic_cookie_match:
                    magic_cookie = int(magic_cookie_match.group(1))
                    magic_cookies.append(magic_cookie)
        run_data = RunData(circuit)
        if len(cpds) != 0:
            run_data.cpd = cpds[-1]
        if len(wls) != 0:
            run_data.wl = wls[-1]
        if len(magic_cookies) != 0:
            run_data.vtr_magic_cookie = magic_cookies[-1]
        if len(runtimes) != 0:
            run_data.runtime = runtimes[-1]
        if len(sssp_runtimes) != 0:
            run_data.sssp_runtime = sum(sssp_runtimes)
        if len(min_chan_widths) != 0:
            run_data.min_chan_width = min_chan_widths[-1]
        # Compute the magic number (used to quickly check determinism)
        # Note: we use the previous magic number in a tuple to make enforce order.
        total_magic_number = 0
        for cpd in cpds:
            total_magic_number = hash((total_magic_number, cpd))
        for wl in wls:
            total_magic_number = hash((total_magic_number, wl))
        for magic_cookie in magic_cookies:
            total_magic_number = hash((total_magic_number, magic_cookie))
        run_data.total_magic_cookie = total_magic_number

        circuit_run_data[circuit] = run_data

    # Check if any of the circuits have minimum channel width information.
    # This would mean we are doing a min channel width search
    has_min_chan_widths = False
    for circuit in sorted(circuits):
        run_data = circuit_run_data[circuit]
        if run_data.has_min_chan_width():
            has_min_chan_widths = True
            break;

    # Count the number of circuits which routed successfully (have all of their data).
    count = 0.0
    for circuit in sorted(circuits):
        run_data = circuit_run_data[circuit]
        if run_data.has_complete_data(has_min_chan_widths):
            count += 1.0

    # If no circuits have routed successfully, stop.
    if count == 0.0:
        print(f"ERROR: No circuits routed successfully!")
        return

    if count != len(circuits):
        print(f"WARNING: Not all circuits routed successfully! {int(len(circuits) - count)} unrouted.")

    # Calculate the interesting information
    geomean_runtime = 1
    geomean_sssp_runtime = 1
    geomean_cpd = 1
    geomean_wl = 1
    geomean_min_chan_width = 1
    magic_number = 0
    # Note: This needs to be sorted so the magic number always returns the correct
    #       magic number regardless of machine.
    for circuit in sorted(circuits):
        run_data = circuit_run_data[circuit]
        if not run_data.has_complete_data(has_min_chan_widths):
            continue;
        # Calculate the geomeans
        geomean_cpd *= run_data.cpd
        geomean_runtime *= run_data.runtime
        geomean_sssp_runtime *= run_data.sssp_runtime
        geomean_wl *= run_data.wl
        if has_min_chan_widths:
            geomean_min_chan_width *= run_data.min_chan_width
        # Compute the magic number (used to quickly check determinism)
        # Note: we use the previous magic number in a tuple to make enforce order.
        magic_number = hash((magic_number, run_data.total_magic_cookie))

    geomean_run_data = RunData("Geomean")
    geomean_run_data.runtime = geomean_runtime ** (1.0 / count)
    geomean_run_data.sssp_runtime = geomean_sssp_runtime ** (1.0 / count)
    geomean_run_data.cpd = geomean_cpd ** (1.0 / count)
    geomean_run_data.wl = geomean_wl ** (1.0 / count)
    geomean_run_data.min_chan_width = geomean_min_chan_width ** (1.0 / count)
    geomean_run_data.vtr_magic_cookie = magic_number
    geomean_run_data.total_magic_cookie = magic_number

    print("*" * 30)
    print("*     Routing Information    *")
    print("*" * 30)
    if has_min_chan_widths:
        print("Circuit:\tCPD(ns)\tRun-time(s)\tSSSP-Run-time(s)\tWirelength\tMagic-cookie\tmin_chan_width")
    else:
        print("Circuit:\tCPD(ns)\tRun-time(s)\tSSSP-Run-time(s)\tWirelength\tMagic-cookie")
    for circuit in sorted(circuits):
        run_data = circuit_run_data[circuit]
        run_data.print(has_min_chan_widths)

    geomean_run_data.print(has_min_chan_widths)

if __name__ == "__main__":
    run_test_main(sys.argv[1:])

