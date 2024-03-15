#include "graph.hpp"

// generates random points on 2D plane within a box of maxsize width & height
inline point generate_random_point(utility::FastRandom& mr) {
    const std::size_t maxsize = 500;
    double x = (double)(mr.get() % maxsize);
    double y = (double)(mr.get() % maxsize);
    return point(x, y);
}

// weighted toss makes closer nodes (in the point vector) heavily connected
inline bool die_toss(std::size_t a, std::size_t b, utility::FastRandom& mr) {
    int node_diff = std::abs(int(a - b));
    // near nodes
    if (node_diff < 16)
        return true;
    // mid nodes
    if (node_diff < 64)
        return ((int)mr.get() % 8 == 0);
    // far nodes
    if (node_diff < 512)
        return ((int)mr.get() % 16 == 0);
    return false;
}

void graph::trace_back(vertex_id src, vertex_id dst, std::vector<vertex_id>& path) {
    vertex_id at = use_packed_predecessor_and_g_dist ? get_predecessor_from_pack(pre_g_dist[dst])
                                                     : predecessor[dst];
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
    path_cost path_length = 0.0;
#ifdef DEBUG
    printf("\n      ");
#endif
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (path[i] != dst) {
            path_cost seg_length = get_distance(vertices[path[i]], vertices[path[i + 1]]);
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
    for (size_t r = 0; r < num_vertices; r += 64) {
        utility::FastRandom my_random(r);
        for (size_t i = r; i < std::min(r + 64, num_vertices); ++i) {
            vertices[i] = generate_random_point(my_random);
        }
    }

    for (size_t r = 0; r < num_vertices; r += 64) {
        utility::FastRandom my_random(r);
        for (size_t i = r; i < std::min(r + 64, num_vertices); ++i) {
            for (size_t j = 0; j < i; ++j) {
                if (die_toss(i, j, my_random))
                    edges[i].push_back(j);
            }
        }
    }

    for (size_t i = 0; i < num_vertices; ++i) {
        for (size_t j = 0; j < edges[i].size(); ++j) {
            vertex_id k = edges[i][j];
            edges[k].push_back(i);
        }
    }

    size_t num_edges = 0;
    for (size_t i = 0; i < num_vertices; ++i) {
        num_edges += edges[i].size();
    }

    printf("Graph(#V=%ld, #E=%ld) is initialized\n", num_vertices, num_edges);
}

void graph::reset() {
    uint64_t packed = pack_predecessor_g_dist(num_vertices, (float)INF);
    for (size_t i = 0; i < num_vertices; ++i) {
        f_distance[i] = g_distance[i] = INF;
        predecessor[i] = num_vertices;
        pre_g_dist[i] = packed;
    }
}

size_t graph::get_num_vertices() {
    return num_vertices;
}
