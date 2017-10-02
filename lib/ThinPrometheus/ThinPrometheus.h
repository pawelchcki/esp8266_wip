#include <map>
#include <vector>

class GaugeCounter {    
public:
    typedef std::map<String, const String> LabelsMap;
    
private:
    const String name;
    const String description;    
    const String type;
    
    mutable std::map<LabelsMap, double> values;
    mutable LabelsMap defaultLabels; // TODO: could this be const ?

    String stringifyLabels(const LabelsMap &labels) const {
        if (labels.size() == 0){
            return "";
        }

        String rv("{");

        bool first = true;
        for (const auto& label: labels){
            if (first){ first = false; } else { rv += ','; }
            rv += String("\"") + label.first + String("\"=\"") + label.second + String("\"");
        }
        rv += '}';
        return rv;
    }

    String toString(double value) const {        
        return String(value);
    }
    
public:    
    GaugeCounter(const String &name,
        const String &description, 
        const String &type, 
        const std::vector<String> &requiredLabels, //TODO: implement required label names to safeguard against differing label sets
        const LabelsMap &defaultLabels ) 
        : name(name), description(description), type(type), defaultLabels(defaultLabels) {};

    double get(const LabelsMap &labels) const { return this->values[labels]; };

    // this CONST is a LIE, but maybe it gives better API ?
    void set(const LabelsMap &labels, double value) const {  this->values[labels] = value; };
    void increment(const LabelsMap &labels, double value) const {  this->values[labels] += value; };
    void increment(const LabelsMap &labels) const {  this->values[labels]++; }; 
    
    
    double get() const { return  this->values[defaultLabels]; };
    void set(double value) const {  this->values[defaultLabels] = value; }; 
    void increment(double value) const {  this->values[defaultLabels] += value; };
    void increment() const { this->values[defaultLabels]++; };

    String represent() const {
        String rv;
        rv += String("# HELP ") + this->name + String(" ") + this->description + String("\n");        
        rv +=  String("# TYPE ") + this->name + String(" ") + this->type + String("\n");  

        if (values.size() == 0) {
            rv += this->name + this->stringifyLabels( this->defaultLabels) + String(" ") + this->toString(0.0) + String("\n");
        }

        for (const auto &labelValue: values){
            rv += this->name + this->stringifyLabels(labelValue.first) + String(" ") + this->toString(labelValue.second) + String("\n");
        }
        return rv;
    };
};

class Registry {
    std::map<String, const GaugeCounter> metrics;

    const GaugeCounter &counterGauge(const String &name, const String &desc, const String &type, const std::vector<String> &requiredLabels, const GaugeCounter::LabelsMap &labelsMap) {
        auto i = this->metrics.find(name);
        if (i == this->metrics.end()) {
            return this->metrics
                    .emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, desc, type, requiredLabels, labelsMap))
                    .first->second;
        } else {
            return i->second;
        }
    };

public:
    const GaugeCounter &counter(const String &name, const String &desc){        
        return counterGauge(name, desc, "counter", std::vector<String>{}, GaugeCounter::LabelsMap{});
    };

    const GaugeCounter &gauge(const String &name, const String &desc){
        static auto &allocCount = counterGauge("prometheus_gauge_alloc_count", "prometheus_gauge_alloc_count", "counter",std::vector<String>{}, GaugeCounter::LabelsMap{});
        allocCount.increment(1);     

        return counterGauge(name, desc, "gauge", std::vector<String>{}, GaugeCounter::LabelsMap{});
    };

    const GaugeCounter &counter(const String &name, const String &desc, const std::vector<String> &requiredLabels, const GaugeCounter::LabelsMap &labelsMap){
        return counterGauge(name, desc, "counter", requiredLabels, labelsMap);
    };

    const GaugeCounter &gauge(const String &name, const String &desc, const std::vector<String> &requiredLabels, const GaugeCounter::LabelsMap &labelsMap){
        static auto &allocCount = counterGauge("prometheus_gauge_alloc_count", "prometheus_gauge_alloc_count", "counter",std::vector<String>{}, GaugeCounter::LabelsMap{});
        allocCount.increment(1);

        return counterGauge(name, desc, "gauge", requiredLabels, labelsMap);
    };

    String represent() const {
        String rv;
        for (const auto &metric: metrics) {
            rv += metric.second.represent();
        }
        return rv;
    };
};
