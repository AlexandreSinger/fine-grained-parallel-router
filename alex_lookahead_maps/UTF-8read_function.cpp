/*
 * Parses the custom lookahead map files provided by Alex Poupakis.
 * The function resizes & modifies the provided cost_map.
 * No architecture ID or size checks are made.
 */
void read_reduced_delta_cost_map_from_file(t_wire_cost_map& cost_map, const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    size_t chan_index = 0;
    size_t seg_index = 0;

    // Variables to capture INFO metadata
    size_t num_segment_types = 0, num_channels = 0, delta_xs = 0, delta_ys = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line.rfind("INFO: Total:", 0) == 0) {
            // Parse metadata
            std::istringstream iss(line);
            std::string info, total, label;
            char comma;

            iss >> info >> total >> label >> num_segment_types >> comma;
            iss >> label >> num_channels >> comma;
            iss >> label >> delta_xs >> comma;
            iss >> label >> delta_ys;

            cost_map.resize({0, 0, num_channels, num_segment_types, delta_xs, delta_ys});
        }
        else if (line.rfind("INFO: ", 0) == 0) {
            // Ignore
        }
        else if (line.rfind("SegTypeId", 0) == 0) {
            // Parse segment and channel indices
            std::istringstream iss(line);
            std::string _, segment_name;
            iss >> _ >> seg_index >> _ >> _ >> _ >> _ >> chan_index;
        }
        else {
            // Parse data line: i, j : delay, congestion
            size_t i, j;
            float delay, congestion;
            char _;

            std::istringstream iss(line);
            iss >> i >> _ >> j >> _ >> delay >> _ >> congestion;

            cost_map[0][0][chan_index][seg_index][i][j].delay = delay;
            cost_map[0][0][chan_index][seg_index][i][j].congestion = congestion;
        }
    }

    file.close();
}
