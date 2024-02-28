#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Feb 12 17:43:05 2024

@author: alex
"""

import pandas as pd
import numpy as np
from scipy import stats
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick

# The name of the circuit to analyze the connections of
CIRCUIT_NAME = "gsm_switch"

# Calculate the percentage of the runtime that the top 1% of the connections are
# taking. Or, in other words, 1% of the connections make up X% of the runtime.
def percentSplit(arr):
    sorted_arr = np.sort(arr)
    idx = int(len(sorted_arr) * 0.99)
    return sorted_arr[idx:].sum() * 100.0 / sorted_arr.sum()

# Helper method for plotting the CDF of the connection route times.
def plotCDF(arr):
    fig = plt.figure()
    # Start by sorting the array and calculating the PDF
    sorted_arr = np.sort(arr)
    pdf = sorted_arr / sorted_arr.sum()
    # Use the cumsum method on the PDF to get the CDF
    cdf = np.cumsum(pdf)
    # Plot the CDF (in percentage) using a log x scale.
    plt.plot(sorted_arr, cdf*100)
    plt.xscale('log')
    plt.xlabel("Connection Route Time (microseconds)")
    plt.ylabel("Cummulative Percent of Total Run Time")
    plt.title("Cummulative Percent of Total Run Time vs Connection Route Time for " + CIRCUIT_NAME)
    # Set the tick marks to something a bit easier to read
    major_ticks = np.linspace(0.0, 100, 11)
    minor_ticks = np.linspace(0.0, 100, 101)
    plt.yticks(major_ticks)
    plt.yticks(minor_ticks, minor=True)
    plt.grid(which='minor', alpha=0.2)
    plt.grid(which='major', alpha=0.5)
    # Set the yaxis to use percentages
    plt.gca().yaxis.set_major_formatter(mtick.PercentFormatter())
    # Save the figure to the output figures directory
    plt.savefig("output_figures/connection_route_time_cdf_" + CIRCUIT_NAME + ".png", dpi=300)

# Helper method for plotting the connection route times.
def plotConnectionRouteTimes(df):
    # Set of unique colors to use to show distinct router iterations
    colors = np.array(['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b', '#e377c2', '#7f7f7f', '#bcbd22', '#17becf'])
    num_colors = len(colors)
    # set the color of each point based on their router iteration
    conn_color = colors[df["Router Iteration"].astype(int).values % num_colors]
    # Plot the connection route times in a logy scale.
    ax = df.plot.scatter(x="index", y=2, c=conn_color, logy=True, title="Connection Route Time Per Execution Call for " + CIRCUIT_NAME)
    # Save the figure to the output figures directory
    plt.savefig("output_figures/connection_route_time_plot_" + CIRCUIT_NAME + ".png", dpi=300)

# Read the CSV file of the data
print("Reading data for:", CIRCUIT_NAME)
df = pd.read_csv("input_data/" + CIRCUIT_NAME + "_connection_data.csv")
# Reset the index so that index is a column
df = df.reset_index()

# Print data on the overall distribution
print("For the overall distribution: ")
print("Mean:", df.iloc[:, 2].mean(), "microseconds")
print("Geomean:", stats.gmean(df.iloc[:, 2]), "microseconds")
print("STDEV:", df.iloc[:, 2].std(), "microseconds")
print("Median:", df.iloc[:, 2].median(), "microseconds")
print("Total time:", df.iloc[:, 2].sum() / 1000000, "seconds")
print("Total number of heap pops:", df.iloc[:, 10].sum())
print("Geomean number of heap pops:", stats.gmean(df.iloc[:, 10]))
print("Geomean number of heap pushes:", stats.gmean(df.iloc[:, 9]))
print("Max number of heap pops:", df.iloc[:, 10].max())
print("Max number of heap pushes:", df.iloc[:, 9].max())
print("Number of connections routed:", len(df.iloc[:, 2]))
print("Percent split: 1% of the connections make up", percentSplit(df.iloc[:, 2]), "% of the time.")

# Print the correlations between each of the metrics and the connection runtime
print("Correlations:")
print(df.corr().iloc[:, 2])

# Plot the connection route times
plotConnectionRouteTimes(df)

# Plot the CDF
plotCDF(df.iloc[:, 2])
