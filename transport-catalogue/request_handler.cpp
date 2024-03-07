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

}  // namespace transport