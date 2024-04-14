#include "domain.h"
#include "request_handler.h"

namespace transport {

	std::optional<domain::BusInfo> RequestHandler::GetBusInfo(const std::string_view& name_bus) const {
		return db_.GetBusInfo(name_bus);
	}

	std::optional<domain::StopInfo> RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
		return db_.GetBusesForStop(stop_name);
	}

	svg::Document RequestHandler::RenderMap() const {
		return renderer_.RenderRoutes(db_.GetBuses(), db_.BusesForStop());
	}

	const std::optional<graph::Router<double>::RouteInfo> RequestHandler::GetOptimalRoute(const std::string_view stop_from, const std::string_view stop_to) const {
		return router_.FindRoute(stop_from, stop_to);
	}

	const graph::DirectedWeightedGraph<double>& RequestHandler::GetRouterGraph() const {
		return router_.GetGraph();
	}

}  // namespace transport