#include <initializer_list>
#include <utility>
#include <tuple>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <iostream>
#include <iterator>
#include <algorithm>

template<class _WeightType>
struct weighed { typedef _WeightType weight_type; };

template<class = void>
struct non_weighed { typedef double weight_type; };

template<
	class WightingPolicy,
	class VertexType,
	class Enable = void
>
struct wighting_policy_traits;

template<
		 class WightingPolicy,
	     class VertexType
	    >
struct wighting_policy_traits<WightingPolicy, VertexType, std::enable_if_t<std::is_same<WightingPolicy, weighed<typename WightingPolicy::weight_type > >::value > >
{
	typedef typename std::tuple<VertexType, VertexType, typename WightingPolicy::weight_type> edge_type;
	typedef typename std::tuple<VertexType, typename WightingPolicy::weight_type > link_type;

	static VertexType extract_vertex1(const edge_type& edge) {
		return std::get<0>(edge);
	}

	static VertexType extract_vertex2(const edge_type& edge) {
		return std::get<1>(edge);
	}

	static typename WightingPolicy::weight_type extract_weight(const edge_type& edge) {
		return std::get<2>(edge);
	}
};

template<
		 class WightingPolicy,
	     class VertexType
	    >
struct wighting_policy_traits<WightingPolicy, VertexType, std::enable_if_t<std::is_same<WightingPolicy, non_weighed<>>::value> >
{
	typedef typename std::tuple<VertexType, VertexType> edge_type;
	typedef typename VertexType link_type;

	static VertexType extract_vertex1(const edge_type& edge) {
		return std::get<0>(edge);
	}

	static VertexType extract_vertex2(const edge_type& edge) {
		return std::get<1>(edge);
	}

	static typename double extract_weight(const edge_type&) {
		return 1;
	}
};

struct oriented { };
struct non_oriented { };


template<
	     class VertexType, 
	     class WightingPolicy, 
	     class OrientationPolicy
        >
class adjacency_lists {

public:
	typedef typename wighting_policy_traits<WightingPolicy, VertexType>::edge_type edge_type;
	typedef std::tuple<VertexType, typename WightingPolicy::weight_type> link_type;
	typedef typename WightingPolicy::weight_type weight_type;

private:

	std::function<VertexType(const edge_type&)> extract_vertex1 = std::bind(&wighting_policy_traits<WightingPolicy, VertexType>::extract_vertex1, std::placeholders::_1);
	std::function<VertexType(const edge_type&)> extract_vertex2 = std::bind(&wighting_policy_traits<WightingPolicy, VertexType>::extract_vertex2, std::placeholders::_1);
	std::function<typename WightingPolicy::weight_type(const edge_type&)> extract_weight = std::bind(&wighting_policy_traits<WightingPolicy, VertexType>::extract_weight, std::placeholders::_1);

	std::map<VertexType, std::vector<link_type> >storage;

public:


	template<class _OrientationPolicy = OrientationPolicy>
	typename std::enable_if_t<std::is_same<_OrientationPolicy, oriented >::value >
	insert(const edge_type& edge) {
		storage[extract_vertex1(edge)].push_back( make_link(extract_vertex2(edge), extract_weight(edge) ) );
	}

	template<class _OrientationPolicy = OrientationPolicy>
	typename std::enable_if_t<std::is_same<_OrientationPolicy, non_oriented >::value >
	insert(const edge_type& edge) {
		storage[extract_vertex1(edge)].push_back(make_link(extract_vertex2(edge), extract_weight(edge)));
		storage[extract_vertex2(edge)].push_back(make_link(extract_vertex1(edge), extract_weight(edge)));
	}

	bool is_adjacent(const VertexType& vertex1, const VertexType& vertex2) {
		auto& adjacency_list = storage[vertex1];
		return ( std::find(adjacency_list.begin(), adjacency_list.end(), vertex2) != adjacency_list.end() );
	}

	template<class _OrientationPolicy = OrientationPolicy>
	typename std::enable_if_t<std::is_same<_OrientationPolicy, oriented >::value, weight_type>
	weight(const VertexType& vertex1, const VertexType& vertex2) {
		auto& adjacency_list = storage[vertex1];
		auto it = std::find(adjacency_list.begin(), adjacency_list.end(), vertex2);
		if (it != adjacency_list.end()) return std::get<1>(*it);

		auto& adjacency_list2 = storage[vertex2];
		it = std::find(adjacency_list2.begin(), adjacency_list2.end(), vertex1);
		return ( it == adjacency_list2.end() ? weight_type() : std::get<1>(*it) );
	}

	template<class _OrientationPolicy = OrientationPolicy>
	typename std::enable_if_t<std::is_same<_OrientationPolicy, non_oriented >::value, weight_type>
		weight(const VertexType& vertex1, const VertexType& vertex2) {
		auto& adjacency_list = storage[vertex1];
		auto it = std::find(adjacency_list.begin(), adjacency_list.end(), vertex2);
		return (it == adjacency_list.end() ? weight_type() : std::get<1>(*it));
	}

	template<class ContainerType >
	typename std::enable_if_t<std::is_same< ContainerType, std::vector<link_type> >::value, ContainerType>
		get_adjacent_vertices(const VertexType& vertex) {
		ContainerType result;
		return storage[vertex];
	}

	template<class ContainerType >
	typename std::enable_if_t<!std::is_same< ContainerType, std::vector<link_type> >::value, ContainerType>
		get_adjacent_vertices(const VertexType& vertex) {
		auto& vertices = storage[vertex];
		return ContainerType(vertices.begin(), vertices.end());
	}
private:

	template<class ...Args>
	link_type make_link(Args&& ...args) {
		return std::make_tuple(std::forward<Args>(args)...);
	}
};

template< 
	     class VertexType, 
		 class WightingPolicy,
		 class OrientationPolicy,
		 template<class,class,class>	 
		 class StorageType = adjacency_lists
		> 
class graph {
	StorageType<VertexType, WightingPolicy, OrientationPolicy> storage;

public:

	typedef typename StorageType<VertexType, WightingPolicy, OrientationPolicy>::link_type link_type;
	typedef typename wighting_policy_traits<WightingPolicy, VertexType>::edge_type edge_type;
	typedef typename WightingPolicy::weight_type weight_type;
	typedef VertexType vertex_type;
	

	graph() = default;

	graph(std::initializer_list<edge_type>edges) {
		for (auto& edge : edges) {
			storage.insert(edge);
		}
	}

	graph(const graph& other)
		: storage(other.storage)
	{ }
	graph(graph&& other) 
		: storage(std::move(other.storage)) 
	{ }

	graph& operator=(const graph& other) {
		storage = other.storage;
	}

	graph& operator=(graph&& other) {
		storage = std::move(other.storage);
	}

	bool is_adjacent(const vertex_type& vertex1, const vertex_type& vertex2) {
		return storage.is_adjacent(vertex1, vertex2);
	}

	weight_type weight(const vertex_type& vertex1, const vertex_type& vertex2) {
		return storage.weight(vertex1, vertex2);
	}

	std::vector<link_type> get_adjacent_vertices(const vertex_type& vertex) {
		return storage.get_adjacent_vertices<std::vector<link_type> >(vertex);
	}
	template<template<class> class ContainerType>
	ContainerType<link_type> get_adjacent_vertices(const vertex_type& vertex) {
		return storage.get_adjacent_vertices<ContainerType<link_type> >(vertex);
	}

	template<class ...Args>
	void insert(Args&& ...args);

	template<class T, class W, class... Args>
	void insert(T&& vertex, T&& adj_vertex, W&& weight, Args&& ...args) {
		storage.insert({std::forward<T>(vertex), std::forward<T>(adj_vertex), std::forward<W>(weight) });
		insert(std::forward<T>(vertex), std::forward<Args>(args)...);
	}

	template<class T, class W>
	void insert(T&& vertex, T&& adj_vertex, W&& weight) {
		storage.insert({ std::forward<T>(vertex), std::forward<T>(adj_vertex), std::forward<W>(weight) });
	}
};

int main()
{
    graph<std::string, weighed<double>, oriented > gr
    {
        std::make_tuple("1","2", 7),
        std::make_tuple("1","3", 9),
        std::make_tuple("1","6", 14) 
    };
	gr.insert("2", 
              "3", 10 , 
              "4", 15
             );
	gr.insert("3" ,
              "6" , 2  ,
              "4" , 11 ,
              "5" , 15
              );
	auto list = gr.get_adjacent_vertices("3");

	for (auto it : list) {
		std::cout << "Vertex:" << std::get<0>(it) << " weight:" << std::get<1>(it) << std::endl;
	}
    return 0;
}

