#include <map>
#include <string>
#include <vector>

class Registry {
  typedef std::map<std::string, const std::string> LabelsMap;

 private:
  std::map<std::string, Metric> metrics;

 public:
  Registry(){};
  const &Gauge gauge(const std::string &name, const LabelsMap &map);
  const &Gauge gauge(const std::string &name);
};

class Metric {
  Metric(const Registry::LabelsMap &labelsMap);

 private:
  const Registry::LabelsMap labelsMap;

 public:
  double get();
};

class Gauge : public Metric {
  void set(double value);
}

extern Registry Prometheus;