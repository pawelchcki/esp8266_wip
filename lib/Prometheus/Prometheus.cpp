#include "Prometheus.h"
#include <algorithm>

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

Metric::Metric(const std::string &name, const std::string &description, const Registry::LabelsRequired &labels) : name(name), description(description), requiredLabels(labels) {};

bool Metric::keyAllowed(const std::string &key){
  //TODO: make this faster than O(n)
  return std::find(this->requiredLabels.start(), this->requiredLabels.end(), key) != this->requiredLabels.end();
}

Gauge::Gauge(const std::string &name, const std::string &description, const Registry::LabelsRequired &labels) : Metric(name, description, labels){
  if (labels.size() > 0) {  
    for(auto const& label: labels){
      this->defaultLabels[label] = "";    
    }
  }
};

const Gauge & Gauge::setDefaultLabels(const Registry::LabelsRequired &defaultLabels){
  // TODO: handle errors.
  this->defaultLabels = defaultLabels;
  return this;
}

double Gauge::get() {  
  return this->values[defaultLabels];
}

void Gauge::set(double value){
  this->values[defaultLabels] = value;
}


// const std::string &Counter::represent() { return std""; };
