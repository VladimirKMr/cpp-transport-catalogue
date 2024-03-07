#include "map_renderer.h"
#include <variant>

using namespace std::string_literals;

namespace renderer {

    MapRenderer::MapRenderer(RenderSettings&& render_settings, const std::vector<geo::Coordinates>& stops_coords)
            : render_settings_(std::move(render_settings)),
              sphere_proj_(stops_coords.begin(), stops_coords.end(), 
                           render_settings_.width, 
                           render_settings_.height, 
                           render_settings_.padding)
    {
    }

	void MapRenderer::BusRouteRender(svg::Document& render_doc, const domain::Bus& bus, size_t color) const {
		// создаём Polyline из данных render_settings_
		svg::Polyline poly = svg::Polyline()
			.SetFillColor(svg::NoneColor)
			.SetStrokeColor(render_settings_.color_palette.at(color))
			.SetStrokeWidth(render_settings_.line_width)
			.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
			.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

		// "проецируем" точки в poly, с помощью sphere_proj_ для кольцевого маршрута
		if (bus.is_roundtrip) {
			for (const auto& elem : bus.route) {
				poly.AddPoint(sphere_proj_(elem->coordinates));
			}
		}

		// "проецируем" для некольцевого
		if (!bus.is_roundtrip) {
			for (const auto& elem : bus.route) {
				poly.AddPoint(sphere_proj_(elem->coordinates));
			}
		}

		render_doc.Add(std::move(poly));
	}
	
	svg::Text MapRenderer::BusTextRenderSettings(const std::string& bus_name, const domain::Stop* stop, bool is_underlayer, size_t color) const {
		svg::Text bus_text = svg::Text()
			.SetPosition(sphere_proj_(stop->coordinates))
			.SetOffset(render_settings_.bus_label_offset)
			.SetFontSize(render_settings_.bus_label_font_size)
			.SetFontFamily("Verdana"s)
			.SetFontWeight("bold"s)
			.SetData(bus_name);
		if (is_underlayer) {
			bus_text
				.SetFillColor(render_settings_.underlayer_color)
				.SetStrokeColor(render_settings_.underlayer_color)
				.SetStrokeWidth(render_settings_.underlayer_width)
				.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
				.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
		}
		else {
			bus_text.SetFillColor(render_settings_.color_palette.at(color));
		}
		return bus_text;
	}
	
	void MapRenderer::BusTextRender(svg::Document& render_doc, const domain::Bus& bus, size_t color) const {
		render_doc.Add(std::move(BusTextRenderSettings(bus.name, bus.route.front(), true)));
		render_doc.Add(std::move(BusTextRenderSettings(bus.name, bus.route.front(), false, color)));

		auto bus_route_end = bus.route.size() / 2;
		auto back_stop_it = bus.route.begin() + bus_route_end;
		if (!bus.is_roundtrip && bus.route.size() > 3 && *bus.route.begin() != *back_stop_it) {  // для некольцевых длиной больше 3х остановок (н-р: A-B-C-B-A)
			render_doc.Add(std::move(BusTextRenderSettings(bus.name, *back_stop_it, true)));
			render_doc.Add(std::move(BusTextRenderSettings(bus.name, *back_stop_it, false, color)));
		}
		if (!bus.is_roundtrip && bus.route.size() == 3 && bus.route.front() != *back_stop_it) {  // для некольцевых длиной 3 остановки, вместе с возвратом (н-р: A-B-A)
			render_doc.Add(std::move(BusTextRenderSettings(bus.name, *back_stop_it, true)));
			render_doc.Add(std::move(BusTextRenderSettings(bus.name, *back_stop_it, false, color)));
		}
	
	}

	svg::Text MapRenderer::StopTextRenderSettings(const domain::Stop* stop, bool is_underlayer) const {
		svg::Text stop_text = svg::Text()
			.SetPosition(sphere_proj_(stop->coordinates))
			.SetOffset(render_settings_.stop_label_offset)
			.SetFontSize(render_settings_.stop_label_font_size)
			.SetFontFamily("Verdana"s)
			.SetData(stop->name);
		if (is_underlayer) {
			stop_text.SetFillColor(render_settings_.underlayer_color)
				.SetStrokeColor(render_settings_.underlayer_color)
				.SetStrokeWidth(render_settings_.underlayer_width)
				.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
				.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
		}
		else {
			stop_text.SetFillColor("black"s);
		}
		return stop_text;
	}

	void MapRenderer::StopRender(svg::Document& render_doc, const domain::Stop* stop) const {
		render_doc.Add(std::move(svg::Circle()
			.SetCenter(sphere_proj_(stop->coordinates))
			.SetRadius(render_settings_.stop_radius)
			.SetFillColor("white"s)));
	}

	// основной метод рендера
	svg::Document MapRenderer::RenderRoutes(const std::deque<domain::Bus>& buses_storage, const std::deque<const domain::Stop*> buses_for_stop) const {
		svg::Document render_doc;

		
		std::deque<domain::Bus> buses;
		for (const domain::Bus& bus : buses_storage) {
			if (!bus.route.empty()) {
				buses.push_back(bus);
			}

		}

		std::sort(buses.begin(), buses.end(), [](const domain::Bus& lhs, const domain::Bus& rhs) {
			return lhs.name < rhs.name;
			}); 

		size_t bus_counter = 0;
		for (const auto& bus : buses) {
			BusRouteRender(render_doc, bus, bus_counter % render_settings_.color_palette.size());
			++bus_counter;
		}
		
		bus_counter = 0;
		for (const auto& bus : buses) {
			BusTextRender(render_doc, bus, bus_counter % render_settings_.color_palette.size());
			++bus_counter;
		}
		
		for (const domain::Stop* stop : buses_for_stop) {
			StopRender(render_doc, stop);
		}
		for (const domain::Stop* stop : buses_for_stop) {
			StopTextRender(render_doc, stop);
		}

		return render_doc;
	}

}