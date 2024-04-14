#include "transport_router.h"


namespace transport {

    const double METERS_IN_KM = 1000;
    const double MINUTES_IN_HOUR = 60;

    // скорость км/ч в м/мин
    static double ConvertSpeed() {
        return METERS_IN_KM / MINUTES_IN_HOUR;
    }

    const graph::DirectedWeightedGraph<double>& Router::BuildGraph(const TransportCatalogue& catalogue) {

        // сортированные списки маршрутов и остановок
        const auto& sort_stops = catalogue.GetSortedStops();
        const auto& sort_buses = catalogue.GetSortedBuses();

        graph::DirectedWeightedGraph<double> stops_graph(sort_stops.size() * 2);
        std::map<std::string, graph::VertexId> stop_ids;
        graph::VertexId vertex_id = 0;

        // формируем ребра ожиданий для каждой остновки
        for (const auto& [stop_name, stop_info] : sort_stops) {
            stop_ids[stop_info->name] = vertex_id;
            stops_graph.AddEdge({ stop_info->name,
                                  0,
                                  vertex_id,
                                  ++vertex_id,
                                  static_cast<double>(settings_.bus_wait_time_)
                                });
            ++vertex_id;
        }
        stop_ids_ = std::move(stop_ids);

        // формируем ребра маршрута
        std::for_each(sort_buses.begin(), sort_buses.end(),
                      [&stops_graph, this, &catalogue](const auto& buses_for_stops) {

                      const auto& bus_info = buses_for_stops.second;
                      const auto& stops = bus_info->route;
                      size_t stops_count = stops.size();

                      for (size_t i = 0; i < stops_count; ++i) {
                          for (size_t j = i + 1; j < stops_count; ++j) {
                              const Stop* stop_from = stops[i];
                              const Stop* stop_to = stops[j];
                              int dist_sum = 0;
                              int dist_sum_inverse = 0;

                              for (size_t k = i + 1; k <= j; ++k) {
                                  auto sum1 = catalogue.GetStopPairDistances(stops[k - 1], stops[k]);
                                  auto sum2 = catalogue.GetStopPairDistances(stops[k], stops[k - 1]);
                                  if (sum1 && sum2) {
                                      dist_sum += sum1.value();
                                      dist_sum_inverse += sum2.value();
                                  }
                              }

                              stops_graph.AddEdge({ bus_info->name,
                                                    j - i,
                                                    stop_ids_.at(stop_from->name) + 1,
                                                    stop_ids_.at(stop_to->name),
                                                    static_cast<double>(dist_sum) / (settings_.bus_velocity_ * ConvertSpeed()) });

                              if (!bus_info->is_roundtrip) {
                                  stops_graph.AddEdge({ bus_info->name,
                                                        j - i,
                                                        stop_ids_.at(stop_to->name) + 1,
                                                        stop_ids_.at(stop_from->name),
                                                        static_cast<double>(dist_sum_inverse) / (settings_.bus_velocity_ * ConvertSpeed()) });
                              }
                          }
                      }
                      });

        graph_ = std::move(stops_graph);
        
        router_ = std::make_unique<graph::Router<double>>(graph_);

        return graph_;
    }

    const std::optional<graph::Router<double>::RouteInfo> Router::FindRoute(const std::string_view stop_from, const std::string_view stop_to) const {
        return router_->BuildRoute(stop_ids_.at(std::string(stop_from)), stop_ids_.at(std::string(stop_to)));
    }

    const graph::DirectedWeightedGraph<double>& Router::GetGraph() const {
        return graph_;
    }


}  // namespace transport