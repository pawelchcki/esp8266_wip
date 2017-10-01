#include <map>
#include <string>
#include <sstream>
#include <vector>

class GaugeCounter {    
public:
    typedef std::map<std::string, const std::string> LabelsMap;
    
private:
    const std::string name;
    const std::string description;    
    const std::string type;
    
    mutable std::map<LabelsMap, double> values;
    mutable LabelsMap defaultLabels; // TODO: could this be const ?

    std::string stringifyLabels(const LabelsMap &labels) const {
        if (labels.size() == 0){
            return "";
        }

        std::string rv("{");
        rv.reserve(labels.size() * 60);

        bool first = true;
        for (const auto& label: labels){
            if (first){ first = false; } else { rv += ','; }
            rv += std::string("\"") + label.first + std::string("\"=\"") + label.second + std::string("\"");
        }
        rv += '}';
        rv.shrink_to_fit();
        return rv;
    }

    std::string toString(double value) const {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }
    
public:    
    GaugeCounter(const std::string &name,
        const std::string &description, 
        const std::string &type, 
        const std::vector<std::string> &requiredLabels, //TODO: implement required label names to safeguard against differing label sets
        const LabelsMap &defaultLabels ) 
        : name(name), description(description), type(type), defaultLabels(defaultLabels) {};

    double get(const LabelsMap &labels) const { return this->values[labels]; };

    // THIS CONST IS A LIE, but maybe it gives better API ?
    void set(const LabelsMap &labels, double value) const {  this->values[labels] = value; };
    void increment(const LabelsMap &labels, double value) const {  this->values[labels] += value; }; 
    
    double get() const { return  this->values[defaultLabels]; };
    void set(double value) const {  this->values[defaultLabels] = value; }; 
    void increment(double value) const {  this->values[defaultLabels] += value; };

    std::string represent() const {
        std::string rv;
        rv.reserve(20000);
        rv += std::string("# HELP ") + this->name + std::string(" ") + this->description + std::string("\n");        
        rv +=  std::string("# TYPE ") + this->name + std::string(" ") + this->type + std::string("\n");  

        if (values.size() == 0) {
            rv += this->name + this->stringifyLabels( this->defaultLabels) + std::string(" ") + this->toString(0.0) + std::string("\n");
        }

        for (const auto &labelValue: values){
            rv += this->name + this->stringifyLabels(labelValue.first) + std::string(" ") + this->toString(labelValue.second) + std::string("\n");
        }
        rv.shrink_to_fit();
        return rv;
    };
};

class Registry {
    std::map<std::string, const GaugeCounter> metrics;

    const GaugeCounter &counterGauge(const std::string &name, const std::string &desc, const std::string &type, const std::vector<std::string> &requiredLabels, const GaugeCounter::LabelsMap &labelsMap) {
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
    const GaugeCounter &counter(const std::string &name, const std::string &desc){
        return counterGauge(name, desc, "counter", std::vector<std::string>{}, GaugeCounter::LabelsMap{});
    };

    const GaugeCounter &gauge(const std::string &name, const std::string &desc){
        return counterGauge(name, desc, "gauge", std::vector<std::string>{}, GaugeCounter::LabelsMap{});
    };

    const GaugeCounter &counter(const std::string &name, const std::string &desc, const std::vector<std::string> &requiredLabels, const GaugeCounter::LabelsMap &labelsMap){
        return counterGauge(name, desc, "counter", requiredLabels, labelsMap);
    };

    const GaugeCounter &gauge(const std::string &name, const std::string &desc, const std::vector<std::string> &requiredLabels, const GaugeCounter::LabelsMap &labelsMap){
        return counterGauge(name, desc, "gauge", requiredLabels, labelsMap);
    };

    std::string represent() const {
        std::string rv;
        rv.reserve(100000);
        for (const auto &metric: metrics) {
            rv += metric.second.represent();
        }
        rv.shrink_to_fit();
        return rv;
    };
};