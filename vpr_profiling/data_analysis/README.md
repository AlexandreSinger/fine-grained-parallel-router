# Data Analysis

This folder contains scripts used to analyze the data produced by profiling the AIR router in VPR.

## parse_connection_data.py

Anaconda was used to handle all of the dependencies.

With Anaconda installed, use the following to run analysis:
```
python3 parse_connection_data.py
```

The circuit to analyze is set within the file.

The CSV data for the circuits should be put into the `circuit_data` directory using the naming convention:
```
<circuit_name>_connection_data.csv
```

NOTE: The CSV data collected is not included in the Git repo since the files are so large.
