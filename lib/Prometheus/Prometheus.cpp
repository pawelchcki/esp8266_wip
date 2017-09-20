#include "Prometheus.h"

const Gauge &Registry::gauge(const std::string &name, const std::string &desc,
                             const LabelsMap &map) {
  auto &i = this->metrics.find(name);
  if (== this->metrics.end()) {
    return this->metrics.emplace(name, desc, map).second();
  } else {
    i.second()
  }
}
