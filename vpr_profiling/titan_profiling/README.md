# Titan Profiling

After the initial profiling, we were interested if the same results can be observed in the Titan benchmarks, which are a bit closer to the real world.

Vaughn recommended the following
- Avoid testcases at the edge of routability.
- LU230 is a good one.
- Do not find the minimum channel width (this will take too long). Use a fixed channel width of 300.
- GSM Switch is another good one (27 min route time and is pretty big).
- Gaussian Blur is a final challenge if needed.

Vaughn showed a table in the VTR8.0 paper which has information on these benchmarks.

## Installing the Titan Benchmarks

Followed the instructions found here to install the Titan architecture and benchmarks into VTR:
https://docs.verilogtorouting.org/en/latest/tutorials/titan_benchmarks/#titan-benchmarks-tutorial

## Running Placement

Decided to get the Placement, Netlist, RR_graph, and lookahead graph in one run so that profiling can be as straightforward as possible.

This requires route to be run as well (at least to get the router lookahead); this also ensure that it does route.

```
bash place_and_route.sh > debug.dump &
```

Note: these runs can take a very long time, so it is a good idea to run this in a process.

### Storing the RR graph / router lookahead
NOTE:

It failed to write the rr graph to a capnp file. It through the following exception:
```
terminate called after throwing an instance of 'kj::ExceptionImpl'
  what():  capnp/layout.c++:1220: failed: total size of struct list is larger than max segment size
```

Will remove saving the rr graph and router lookahead for now. Will just have to subtract off their time.

According to Amin, we can use XML instead of bin for the RR graph (it may default to capnp).
- Through looking at the VPR documentation, they both can be set to XML: https://docs.verilogtorouting.org/en/latest/vpr/command_line_usage/#cmdoption-vpr-write_rr_graph
- Set them both to XML and ran again.

## Running Profiling

After modifying VPR to collect data on the connections, the following can be run to just run routing on the benchmark:
```
bash route_sink_profile.sh > debug.dump &
```

This will generate a file with all the profiled data sent to the std output. This data will need to be cleaned into a CSV file
by removing the parts of the output that are not necessary.

Useful vim command that will delete all lines with '%' in it:
```
:g/%/d
```