#ifndef _GRAPH_HPP_
#define _GRAPH_HPP_

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

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

typedef std::vector<point> point_set;
typedef std::size_t vertex_id;
typedef double path_cost;
typedef std::tuple<path_cost, vertex_id> vertex_rec;
typedef std::vector<std::vector<vertex_id>> edge_set;

const double INF = 1000000.0; // infinity

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