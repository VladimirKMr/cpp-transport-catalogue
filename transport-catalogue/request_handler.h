#pragma once
#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.h"

#include <optional>
#include <set>

namespace transport {

	class RequestHandler {
	public:
		RequestHandler(const TransportCatalogue& catalogue, const renderer::MapRenderer& renderer, const transport::Router& router)
			: db_(catalogue)
			, renderer_(renderer)
			, router_(router)
		{
		}

		// Возвращает информацию о маршруте (запрос Bus)
		std::optional<domain::BusInfo> GetBusInfo(const std::string_view& bus_name) const;

		// Возвращает маршруты, проходящие через
		std::optional<domain::StopInfo> GetBusesByStop(const std::string_view& stop_name) const;
		
		// построение SVG
		svg::Document RenderMap() const;

		const std::optional<graph::Router<double>::RouteInfo> GetOptimalRoute(const std::string_view stop_from, const std::string_view stop_to) const;

		const graph::DirectedWeightedGraph<double>& GetRouterGraph() const;

	private:
		const TransportCatalogue db_;
		const renderer::MapRenderer& renderer_;
		const transport::Router& router_;
	};

}  // namespace transport