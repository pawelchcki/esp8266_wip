#include <map>
#include <string>
#include <sstream>
#include <vector>

class GaugeCounter {
    typedef std::map<std::string, const std::string> LabelsMap;
    
    const std::string name;
    const std::string description;    
    const std::string type;
    
    LabelsMap defaultLabels;
    std::map<LabelsMap, double> values;

private:
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
    GaugeCounter(const std::string &name, const std::string &description, const std::string &type) : name(name), description(description), type(type) {};


    double get(const LabelsMap &labels) { return this->values[labels]; };
    void set(const LabelsMap &labels, double value) {  this->values[labels] = value; };
    void increase(const LabelsMap &labels, double value) {  this->values[labels] += value; }; 
    
    double get() { return  this->values[defaultLabels]; };
    void set(double value) {  this->values[defaultLabels] = value; };
    void increase(double value) {  this->values[defaultLabels] += value; };

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

    const GaugeCounter &counterGauge(const std::string &name, const std::string &desc, const std::string &type) {
        auto i = this->metrics.find(name);
        if (i == this->metrics.end()) {
            return this->metrics
                    .emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name, desc, type))
                    .first->second;
        } else {
            return i->second;
        }
    };

public:
    const GaugeCounter &counter(const std::string &name, const std::string &desc){
        return counterGauge(name, desc, "counter");
    };

    const GaugeCounter &gauge(const std::string &name, const std::string &desc){
        return counterGauge(name, desc, "gauge");
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