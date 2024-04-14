#pragma once

#include <map>
#include <memory>

#include "transport_catalogue.h"
#include "router.h"

namespace transport {

	struct RouterSettings {
		int bus_wait_time_ = 0;
		double bus_velocity_ = 0.0;
	};

	class Router {
	public:
		Router() = default;

		Router(const RouterSettings& settings, const TransportCatalogue& catalogue) 
			: settings_(settings)
		{
			BuildGraph(catalogue);
		}

		const graph::DirectedWeightedGraph<double>& BuildGraph(const TransportCatalogue& catalogue);
		const std::optional<graph::Router<double>::RouteInfo> FindRoute(const std::string_view stop_from, const std::string_view stop_to) const;
		const graph::DirectedWeightedGraph<double>& GetGraph() const;

	private:
		RouterSettings settings_;
		graph::DirectedWeightedGraph<double> graph_;
		std::map<std::string, graph::VertexId> stop_ids_;
		std::unique_ptr<graph::Router<double>> router_;
	};

}  // namespace transport