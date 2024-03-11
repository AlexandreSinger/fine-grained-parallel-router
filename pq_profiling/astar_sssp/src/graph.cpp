#include "graph.hpp"

void graph::trace_back(vertex_id src, vertex_id dst, std::vector<vertex_id>& path) {
    vertex_id at = predecessor[dst];
    if (at == num_vertices)
        path.push_back(src);
    else if (at == src) {
        path.push_back(src);
        path.push_back(dst);
    }
    else {
        trace_back(src, at, path);
        path.push_back(dst);
    }
}

void graph::print_path(vertex_id src, vertex_id dst) {
    std::vector<vertex_id> path;
    trace_back(src, dst, path);
    double path_length = 0.0;
#ifdef DEBUG
    printf("\n      ");
#endif
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (path[i] != dst) {
            double seg_length = get_distance(vertices[path[i]], vertices[path[i + 1]]);
#ifdef DEBUG
            printf("%6.1f       ", seg_length);
#endif
            path_length += seg_length;
        }
    }
#ifdef DEBUG
    printf("\n");
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (path[i] != dst)
            printf("(%4d)------>", (int)path[i]);
        else
            printf("(%4d)\n", (int)path[i]);
    }

    printf("Total distance = %5.1f\n", path_length);
#endif

    printf(" %5.1f\n", path_length);
}

void graph::init() {
    oneapi::tbb::global_control c(oneapi::tbb::global_control::max_allowed_parallelism,
                                  utility::get_default_num_threads());
    vertices.resize(num_vertices);
    edges.resize(num_vertices);
    predecessor.resize(num_vertices);
    g_distance.resize(num_vertices);
    f_distance.resize(num_vertices);

    // printf("Generating vertices...\n");
    oneapi::tbb::parallel_for(
        oneapi::tbb::blocked_range<std::size_t>(0, num_vertices, 64),
        [&](oneapi::tbb::blocked_range<std::size_t>& r) {
            utility::FastRandom my_random(r.begin());
            for (std::size_t i = r.begin(); i != r.end(); ++i) {
                vertices[i] = generate_random_point(my_random);
            }
        },
        oneapi::tbb::simple_partitioner());

    // printf("Generating edges...\n");
    oneapi::tbb::parallel_for(
        oneapi::tbb::blocked_range<std::size_t>(0, num_vertices, 64),
        [&](oneapi::tbb::blocked_range<std::size_t>& r) {
            utility::FastRandom my_random(r.begin());
            for (std::size_t i = r.begin(); i != r.end(); ++i) {
                for (std::size_t j = 0; j < i; ++j) {
                    if (die_toss(i, j, my_random))
                        edges[i].push_back(j);
                }
            }
        },
        oneapi::tbb::simple_partitioner());

    for (std::size_t i = 0; i < num_vertices; ++i) {
        for (std::size_t j = 0; j < edges[i].size(); ++j) {
            vertex_id k = edges[i][j];
            edges[k].push_back(i);
        }
    }

    size_t num_edges = 0;
    for (std::size_t i = 0; i < num_vertices; ++i) {
        num_edges += edges[i].size();
    }
    printf("Graph(#V=%ld, #E=%ld) is initialized\n", num_vertices, num_edges);
    fflush(stdout);
}

void graph::reset() {
    oneapi::tbb::global_control c(oneapi::tbb::global_control::max_allowed_parallelism,
                                  utility::get_default_num_threads());
    oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<std::size_t>(0, num_vertices),
                              [&](oneapi::tbb::blocked_range<std::size_t>& r) {
                                  for (std::size_t i = r.begin(); i != r.end(); ++i) {
                                      f_distance[i] = g_distance[i] = INF;
                                      predecessor[i] = num_vertices;
                                  }
                              });
}

size_t graph::get_num_vertices() {
    return num_vertices;
}
