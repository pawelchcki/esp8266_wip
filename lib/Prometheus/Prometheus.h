#include <map>
#include <string>
#include <vector>

class Metric;
class Gauge;

class Registry {
 public:
  typedef std::map<std::string, const std::string> LabelsMap;
  typedef std::vector<std::string> LabelsAllowed;

  //TODO: make into a template to possibly help implement Histograms and summaries  

 private:
  std::map<std::string, Metric> metrics;

 public:
  Registry(){};
  const Gauge &gauge(const std::string &name, const std::string &description, const LabelsMap &map);
  const Gauge &gauge(const std::string &name, const std::string &description);

  const Gauge &counter(const std::string &name, const std::string &description, const LabelsMap &map);
  const Gauge &counter(const std::string &name, const std::string &description);
};

class Metric {
 protected:
  const Registry::LabelsMap labelsMap;
  std::string labelsRepresent();

 public:
  Metric(const std::string &description, const Registry::LabelsMap &labelsMap);
  Metric(const std::string &description, const LabelsAllowed &labelsMap);
  Metric(const Metric &) = delete;
  Metric(Metric &&) = delete;

  // TODO: refactor it to something more generic
  virtual std::string represent() { return ""; };
};

class Gauge : public Metric {
 protected:
  double value;

 public:
  double get() { return value; };
  void set(double value);

  std::string represent() override;
};

class Counter : public Metric {
 protected:
  double count = 0;

 public:
  double get() { return count; };
  void increment();
  void increment(double inc);

  std::string represent() override;
};

extern Registry Prometheus;