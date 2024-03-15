#ifndef _GRAPH_HPP_
#define _GRAPH_HPP_

#include <cmath>
#include <cstdint>
#include <vector>
#include <iostream>
#include <algorithm>
#include <atomic>

#include "utility.hpp"

struct point {
    double x, y;
    point() {}
    point(double _x, double _y) : x(_x), y(_y) {}
    point(const point& p) : x(p.x), y(p.y) {}
};

typedef std::vector<point> point_set;
typedef std::size_t vertex_id;
typedef float path_cost;
typedef std::tuple<path_cost, vertex_id> vertex_rec;
typedef std::vector<std::vector<vertex_id>> edge_set;

const path_cost INF = 1000000.0; // infinity

inline path_cost get_distance(const point& p1, const point& p2) {
    path_cost xdiff = p1.x - p2.x, ydiff = p1.y - p2.y;
    return sqrt(xdiff * xdiff + ydiff * ydiff);
}

class graph {
public:
    point_set vertices; // vertices
    edge_set edges; // edges
    vertex_id* predecessor; // for recreating path from src to dst
    path_cost* f_distance; // estimated distances at particular vertex
    path_cost* g_distance; // current shortest distances from src vertex

    bool use_packed_predecessor_and_g_dist;
    std::atomic_uint64_t* pre_g_dist;

    graph(size_t num_vertices) {
        this->num_vertices = num_vertices;
        vertices.resize(num_vertices);
        edges.resize(num_vertices);

        predecessor = new vertex_id[num_vertices];
        g_distance = new path_cost[num_vertices];
        f_distance = new path_cost[num_vertices];

        pre_g_dist = new std::atomic_uint64_t[num_vertices];

        init();
    }

    ~graph() {
        delete[] predecessor;
        delete[] f_distance;
        delete[] g_distance;
        delete[] pre_g_dist;
    }

    void trace_back(vertex_id src, vertex_id dst, std::vector<vertex_id>& path);
    void print_path(vertex_id src, vertex_id dst);
    void reset();

    size_t get_num_vertices();

protected:
    size_t num_vertices;
    void init();
};

union uint32_float_conv {
    float f;
    uint32_t i;
};

inline float get_g_dist_from_pack(uint64_t p) {
    uint32_t i = (uint64_t)(p & 0x00000000ffffffff);
    uint32_float_conv conv;
    conv.i = i;
    return conv.f;
}

inline size_t get_predecessor_from_pack(uint64_t p) {
    return (size_t)((p & 0xffffffff00000000) >> 32);
}

inline uint64_t pack_predecessor_g_dist(size_t pre, float g_dist) {
    uint32_float_conv conv;
    conv.f = g_dist;
    uint64_t pack = (((uint64_t)(pre)) << 32) | ((uint64_t)(conv.i));
    return pack;
}

#endif