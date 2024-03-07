#pragma once
#include "transport_catalogue.h"
#include "map_renderer.h"

#include <optional>
#include <set>

namespace transport {

	class RequestHandler {
	public:
		RequestHandler(const TransportCatalogue& catalogue, const renderer::MapRenderer& renderer)
			: db_(catalogue)
			, renderer_(renderer)
		{
		}

		// Возвращает информацию о маршруте (запрос Bus)
		std::optional<domain::BusInfo> GetBusInfo(const std::string_view& bus_name) const;

		// Возвращает маршруты, проходящие через
		std::optional<domain::StopInfo> GetBusesByStop(const std::string_view& stop_name) const;

		svg::Document RenderMap() const;

	private:
		const TransportCatalogue db_;
		const renderer::MapRenderer& renderer_;
	};

}  // namespace transport