#include "Prometheus.h"

Gauge dummy;

const Gauge &Registry::gauge(const std::string &name, const std::string &desc,
                             const LabelsMap &map) {
  auto i = this->metrics.find(name);
  if (i == this->metrics.end()) {
    return static_cast<Gauge&>(this->metrics.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(desc, map)).first->second);
  } else {
    return static_cast<Gauge&>(i->second);
  }
  // return dummy;
}

