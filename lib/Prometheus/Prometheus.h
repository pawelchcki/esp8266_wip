#include <map>
#include <string>
#include <vector>

class Metric;
class Gauge;
class Counter;

class Registry {
 public:
  typedef std::map<std::string, const std::string> LabelsMap;
  typedef std::vector<std::string> LabelsRequired;

  // TODO: make into a template to possibly help implement Histograms and summaries

 private:
  std::map<std::string, Metric> metrics;

 public:
  Registry(){};
  const Gauge &gauge(const std::string &name, const std::string &description, const LabelsRequired &labels);

  const Gauge &gauge(const std::string &name, const std::string &description);

  const Counter &counter(const std::string &name, const std::string &description, const LabelsRequired &labels);
  const Counter &counter(const std::string &name, const std::string &description);
};

class Metric {
  // typedef std::map<LabelsMap, T> LabelsValueMap<T>;

 protected:
  const std::string description;
  const std::string name;
  const Registry::LabelsRequired requiredLabels;

  const Registry::LabelsMap labelsMap;
  std::string labelsRepresent();

 public:
  Metric(const std::string &name, const std::string &description, const Registry::LabelsRequired &labels);
  Metric(const Metric &) = delete;
  Metric(Metric &&) = delete;

  // TODO: refactor it to something more generic
  virtual std::string represent() { return ""; };
};

class Gauge : public Metric {  
 protected:
  std::map<Registry::LabelsMap, double> values;
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