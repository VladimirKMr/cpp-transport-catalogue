#pragma once
#include <istream>
#include <sstream>

#include "request_handler.h"
#include "transport_catalogue.h"
#include "json.h"

namespace json_reader {

	class JsonReader {
	public:
		JsonReader(std::istream& in)
			: in_(in)
			, json_document_(json::Load(in_))
		{
		}

		// ќбработка запросов к каталогу через RequestHandler
		void ResponseRequests(std::ostream& os, const transport::RequestHandler& rq) const;

		// «аполнение транспортного каталога из Json
		transport::TransportCatalogue TransportCatalogueFromJson() const;

		renderer::RenderSettings ParseRenderSettings() const;
		renderer::MapRenderer MapRenderFromJson(const transport::TransportCatalogue& ts) const;

	private:
		std::istream& in_;
		json::Document json_document_;

		domain::Bus ParseBusQuery(const transport::TransportCatalogue& ts, const json::Node& type_bus) const;
		domain::Stop ParseStopQuery(const json::Node& type_stop) const;

		void ParseStopQueryDistance(transport::TransportCatalogue& ts, const json::Node& type_stop) const;

		json::Dict StopResponseToJsonDict(int request_id, const std::optional<domain::StopInfo>& stop_info) const;
		json::Dict BusResponseToJsonDict(int request_id, const std::optional<domain::BusInfo>& bus_info) const;
		json::Dict MapResponseToJsonDict(int request_id, const svg::Document& render_doc) const;

		svg::Color ParseColor(const json::Node& node) const;
	};


}  // namespace json_reader