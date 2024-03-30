#!/usr/bin/python3

import sys
import argparse

import math
import csv

# Helper method to generate a parser for the command line interface.
def command_parser(prog=None):
    description = "Runs a given test."
    parser = argparse.ArgumentParser(
        prog=prog,
        description=description,
        epilog="",
    )

    parser.add_argument("csv_file")

    return parser

def parse_mod_list_profile_main(arg_list, prog=None):
    # Load the arguments
    args = command_parser(prog).parse_args(arg_list)

    with open(args.csv_file) as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=",")
        avg_mod_list_size = 0
        geomean_mod_list_size = 0.0
        count = 0

        avg_dist = 0
        geomean_dist = 0.0

        total_num_nodes_set = set()
        for row in csv_reader:
            # Assumign no header information
            mod_list_size = int(row[0])
            total_num_nodes = int(row[1])
            min_node_id_chanx = int(row[2])
            max_node_id_chanx = int(row[3])
            min_node_id_chany = int(row[4])
            max_node_id_chany = int(row[5])
            min_node_id_other = int(row[6])
            max_node_id_other = int(row[7])

            total_num_nodes_set.add(total_num_nodes)
            
            avg_mod_list_size += mod_list_size
            geomean_mod_list_size += math.log(mod_list_size)

            dist = 0
            if (min_node_id_chanx <= max_node_id_chanx):
                dist += max_node_id_chanx - min_node_id_chanx + 1
            if (min_node_id_chany <= max_node_id_chany):
                dist += max_node_id_chany - min_node_id_chany + 1
            if (min_node_id_other <= max_node_id_other):
                dist += max_node_id_other - min_node_id_other + 1
            # dist = max_node_id - min_node_id
            avg_dist += dist
            geomean_dist += math.log(dist)

            count += 1

        assert(len(total_num_nodes_set) == 1)
        total_num_nodes = total_num_nodes_set.pop()

        avg_mod_list_size /= count
        geomean_mod_list_size = math.exp(geomean_mod_list_size / float(count))

        avg_dist /= count
        geomean_dist = math.exp(geomean_dist / float(count))

        print(f"Average Mod List size: {avg_mod_list_size}")
        print(f"Geomean Mod List size: {geomean_mod_list_size}")
        print(f"Average dist: {avg_dist}")
        print(f"Geomean dist: {geomean_dist}")
        print(f"Total num nodes: {total_num_nodes}")

if __name__ == "__main__":
    parse_mod_list_profile_main(sys.argv[1:])

