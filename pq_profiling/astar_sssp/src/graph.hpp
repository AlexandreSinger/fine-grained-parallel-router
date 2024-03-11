#ifndef _GRAPH_HPP_
#define _GRAPH_HPP_

#include <cmath>
#include "utility.hpp"

struct point {
    double x, y;
    point() {}
    point(double _x, double _y) : x(_x), y(_y) {}
    point(const point& p) : x(p.x), y(p.y) {}
};

inline double get_distance(const point& p1, const point& p2) {
    double xdiff = p1.x - p2.x, ydiff = p1.y - p2.y;
    return sqrt(xdiff * xdiff + ydiff * ydiff);
}

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

typedef std::vector<point> point_set;
typedef std::size_t vertex_id;
typedef double path_cost;
typedef std::tuple<path_cost, vertex_id> vertex_rec;
typedef std::vector<std::vector<vertex_id>> edge_set;

const double INF = 1000000.0; // infinity

#include "oneapi/tbb/spin_mutex.h"
#include "oneapi/tbb/parallel_for.h"
#include "oneapi/tbb/blocked_range.h"
#include "oneapi/tbb/global_control.h"

class graph {
public:
    point_set vertices; // vertices
    edge_set edges; // edges
    std::vector<vertex_id> predecessor; // for recreating path from src to dst
    std::vector<path_cost> f_distance; // estimated distances at particular vertex
    std::vector<path_cost> g_distance; // current shortest distances from src vertex

    graph(size_t num_vertices) {
        this->num_vertices = num_vertices;
        init();
    }
    void trace_back(vertex_id src, vertex_id dst, std::vector<vertex_id>& path);
    void print_path(vertex_id src, vertex_id dst);
    void reset();

    size_t get_num_vertices();

private:
    size_t num_vertices;
    void init();
};

#endif