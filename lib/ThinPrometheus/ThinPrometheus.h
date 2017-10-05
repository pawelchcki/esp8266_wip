#ifndef THIN_PROMETHEUS_H
#define THIN_PROMETHEUS_H

#include <map>
#include <vector>
#include <functional>

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
    typedef std::function<void(Registry&)> CollectorFunction;
    std::map<String, const GaugeCounter> metrics;
    std::vector<CollectorFunction> collectorFunctions;

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
        static auto &allocCount = counterGauge("prometheus_gauge_alloc_total", "prometheus_gauge_alloc_count", "counter",std::vector<String>{}, GaugeCounter::LabelsMap{});
        allocCount.increment(1);     

        return counterGauge(name, desc, "gauge", std::vector<String>{}, GaugeCounter::LabelsMap{});
    };

    const GaugeCounter &counter(const String &name, const String &desc, const std::vector<String> &requiredLabels, const GaugeCounter::LabelsMap &labelsMap){
        return counterGauge(name, desc, "counter", requiredLabels, labelsMap);
    };

    const GaugeCounter &gauge(const String &name, const String &desc, const std::vector<String> &requiredLabels, const GaugeCounter::LabelsMap &labelsMap){
        static auto &allocCount = counterGauge("prometheus_gauge_alloc_total", "prometheus_gauge_alloc_count", "counter",std::vector<String>{}, GaugeCounter::LabelsMap{});
        allocCount.increment(1);

        return counterGauge(name, desc, "gauge", requiredLabels, labelsMap);
    };

    void addCollector(const CollectorFunction &fn) {
        this->collectorFunctions.push_back(fn);
    }

    void collect(){
        static auto &collectCount = counterGauge("prometheus_collectors_total", "prometheus run collectors", "counter", std::vector<String>{}, GaugeCounter::LabelsMap{});
        
        for (const auto &fn: this->collectorFunctions){
            collectCount.increment();
            fn(*this);
        }
    };

    String represent() const {
        String rv;
        for (const auto &metric: metrics) {
            rv += metric.second.represent();
        }
        return rv;
    };

    String collectAndRepresent() {
        this->collect();
        return this->represent();        
    };
};

struct CommonCollectors {
    static void collectEspInfo(Registry& registry) {
        static auto &vcc = registry.gauge("esp_vcc", "Voltage reading if ADC is set to read VCC voltage");
        static auto &freeHeap = registry.gauge("esp_free_heap", "Free heap");
        static auto &chipId = registry.gauge("esp_chip_id", "ESP chip id");
        static auto &bootVersion = registry.gauge("esp_boot_version", "");
        static auto &bootMode = registry.gauge("esp_boot_mode", "");
        static auto &cpuFreqMhz = registry.gauge("esp_cpu_freq_mhz", "");
        static auto &flashChipId = registry.gauge("esp_flash_chip_id", "");
        static auto &flashChipRealSize = registry.gauge("esp_flash_chip_real_size", "");
        static auto &flashChipSize = registry.gauge("esp_flash_chip_size", "");
        static auto &flashChipSpeed = registry.gauge("esp_flash_chip_speed", "");
        static auto &flashChipSizeByChipId = registry.gauge("esp_flash_chip_size_by_chip_id", "");
        static auto &resetReason = registry.gauge("esp_reset_reason", "");
        static auto &sketchSize = registry.gauge("esp_sketch_size", "");
        static auto &sketchFreeSpace = registry.gauge("esp_sketch_free_space", "");
        static auto &espCycleCount = registry.counter("esp_cycle_total", "");
      
        vcc.set(ESP.getVcc()/1024.0);
        freeHeap.set(ESP.getFreeHeap());
        chipId.set(ESP.getChipId());
        bootVersion.set(ESP.getBootVersion());
        bootMode.set(ESP.getBootMode());
        cpuFreqMhz.set(ESP.getCpuFreqMHz());
        flashChipId.set(ESP.getFlashChipId());
        flashChipRealSize.set(ESP.getFlashChipRealSize());
        flashChipSize.set(ESP.getFlashChipSize());
        flashChipSpeed.set(ESP.getFlashChipSpeed());
        flashChipSizeByChipId.set(ESP.getFlashChipSizeByChipId());
        resetReason.set(ESP.getResetInfoPtr()->reason);
        sketchSize.set(ESP.getSketchSize());
        sketchFreeSpace.set(ESP.getFreeSketchSpace());
        espCycleCount.set(ESP.getCycleCount());
    };
};

#endif /* THIN_PROMETHEUS_H */