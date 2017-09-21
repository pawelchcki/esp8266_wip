#include <map>
#include <string>
#include <vector>

class Metric;
class Gauge;

class Registry {
 public:
  typedef std::map<std::string, const std::string> LabelsMap;

 private:
  std::map<std::string, Metric> metrics;

 public:
  Registry(){};
  const Gauge &gauge(const std::string &name, const std::string &description, const LabelsMap &map);
  const Gauge &gauge(const std::string &name, const std::string &description);
};

class Metric {
 private:
  const Registry::LabelsMap labelsMap;

 public:
  Metric(const std::string &description, const Registry::LabelsMap &labelsMap);  
  double get();
};

class Gauge : public Metric {
public:
  Gauge(const Gauge&) = delete;
  Gauge(Gauge&&) = delete;
  Gauge();
  void set(double value);
};

extern Registry Prometheus;