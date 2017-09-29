#include "Prometheus.h"

const Gauge &Registry::gauge(const std::string &name, const std::string &desc, const LabelsRequired &labels) {
  auto i = this->metrics.find(name);
  if (i == this->metrics.end()) {
    return static_cast<Gauge &>(
        this->metrics
            .emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, desc, labels))
            .first->second);
  } else {
    return static_cast<Gauge &>(i->second);
  }
}

const Gauge &Registry::gauge(const std::string &name, const std::string &desc) { this->gauge(name, desc, {{}}); };

std::string Metric::labelsRepresent() {
  if (this->labelsMap.size() == 0) {
    return "";
  }

  std::string rv("{");

  auto first = true;
  for (const auto &kvPair : this->labelsMap) {
    if (!first) {
      rv += ",";
    } else {
      first = false;
    }

    // TODO: escape values
    rv += std::string("\"") + kvPair.first + std::string("\"=\"") + kvPair.second + std::string("\"");
  }
  rv += "}";
  return rv;
}

std::string Gauge::represent() { return ""; };

Metric::Metric(const std::string &name, const std::string &description, const Registry::LabelsRequired &labels) : name(name), description(description), requiredLabels(labels) {

};

// const std::string &Counter::represent() { return std""; };
