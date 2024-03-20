#pragma once

#include "geo.h"
#include "svg.h"
#include "domain.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <optional>
#include <vector>


namespace renderer {

	inline const double EPSILON = 1e-6;

	inline bool IsZero(double value) {
		return std::abs(value) < EPSILON;
	}

    class SphereProjector {
    public:
        
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                        double max_width, double max_height, double padding);

        
        svg::Point operator()(geo::Coordinates coords) const {
            return {
                (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_
            };
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    struct RenderSettings {
        
        double width;
        double height;

        double padding;

        double line_width;

        double stop_radius;

        int bus_label_font_size;

        svg::Point bus_label_offset;

        int stop_label_font_size;

        svg::Point stop_label_offset;

        svg::Color underlayer_color;

        double underlayer_width;

        std::vector<svg::Color> color_palette;
    };

    template <typename PointInputIt>
    SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                                     double max_width, double max_height, double padding)
        : padding_(padding)
    {
        
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    class MapRenderer {
    public:
        MapRenderer(RenderSettings&& render_settings, const std::vector<geo::Coordinates>& stops_coords);

        void BusRouteRender(svg::Document& render_doc, const domain::Bus& bus, size_t color) const;
        svg::Text BusTextRenderSettings(const std::string& bus_name, const domain::Stop* stop, bool is_underlayer, size_t color = 0) const;

        void BusTextRender(svg::Document& render_doc, const domain::Bus& bus, size_t color) const;
        svg::Text StopTextRenderSettings(const domain::Stop* stop, bool is_underlayer) const;

        inline void StopTextRender(svg::Document& render_doc, const domain::Stop* stop) const {
            render_doc.Add(std::move(StopTextRenderSettings(stop, true)));
            render_doc.Add(std::move(StopTextRenderSettings(stop, false)));
        }

        void StopRender(svg::Document& render_doc, const domain::Stop* stop) const;

        svg::Document RenderRoutes(const std::deque<domain::Bus>& buses_storage, const std::deque<const domain::Stop*> buses_for_stop) const;

    private:
        RenderSettings render_settings_;
        const SphereProjector sphere_proj_;
    };

}  // namespace renderer