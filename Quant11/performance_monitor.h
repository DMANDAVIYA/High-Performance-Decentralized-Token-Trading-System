#pragma once

#define _GLIBCXX_USE_CXX11_ABI 1
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <mutex>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <limits>
#include <thread>

class PerformanceMonitor {
public:
    struct Metrics {
        size_t total_count = 0;
        double total_latency = 0.0;
        double min_latency = std::numeric_limits<double>::max();
        double max_latency = 0.0;
        std::vector<double> latency_samples;
        static constexpr size_t MAX_SAMPLES = 1000;
    };

    void record_latency(const std::string& operation, double latency_ms) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& metrics = m_metrics[operation];
        metrics.total_count++;
        metrics.total_latency += latency_ms;
        metrics.min_latency = std::min(metrics.min_latency, latency_ms);
        metrics.max_latency = std::max(metrics.max_latency, latency_ms);
        
        if (metrics.latency_samples.size() < Metrics::MAX_SAMPLES) {
            metrics.latency_samples.push_back(latency_ms);
        } else {
            metrics.latency_samples[metrics.total_count % Metrics::MAX_SAMPLES] = latency_ms;
        }
    }

    Metrics get_metrics(const std::string& operation) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_metrics.find(operation);
        return it != m_metrics.end() ? it->second : Metrics();
    }

    void clear_metrics(const std::string& operation) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_metrics.erase(operation);
    }

    void clear_all_metrics() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_metrics.clear();
    }

    std::string get_metrics_json() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string json = "{\n";
        for (const auto& pair : m_metrics) {
            const auto& operation = pair.first;
            const auto& metrics = pair.second;
            json += "  \"" + operation + "\": {\n";
            json += "    \"total_count\": " + std::to_string(metrics.total_count) + ",\n";
            json += "    \"avg_latency\": " + std::to_string(metrics.total_latency / metrics.total_count) + ",\n";
            json += "    \"min_latency\": " + std::to_string(metrics.min_latency) + ",\n";
            json += "    \"max_latency\": " + std::to_string(metrics.max_latency) + ",\n";
            
            if (!metrics.latency_samples.empty()) {
                double mean = metrics.total_latency / metrics.total_count;
                double variance = 0.0;
                for (double sample : metrics.latency_samples) {
                    variance += std::pow(sample - mean, 2);
                }
                variance /= metrics.latency_samples.size();
                
                json += "    \"std_dev\": " + std::to_string(std::sqrt(variance)) + ",\n";
                json += "    \"p95\": " + std::to_string(calculate_percentile(metrics.latency_samples, 95)) + ",\n";
                json += "    \"p99\": " + std::to_string(calculate_percentile(metrics.latency_samples, 99)) + "\n";
            } else {
                json += "    \"std_dev\": 0.0,\n";
                json += "    \"p95\": 0.0,\n";
                json += "    \"p99\": 0.0\n";
            }
            
            json += "  },\n";
        }
        if (!m_metrics.empty()) {
            json.pop_back(); // Remove trailing comma
            json.pop_back(); // Remove newline
        }
        json += "\n}\n";
        return json;
    }

    bool export_metrics_to_file(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        file << get_metrics_json();
        return true;
    }

private:
    double calculate_percentile(const std::vector<double>& samples, double percentile) const {
        if (samples.empty()) return 0.0;
        std::vector<double> sorted_samples = samples;
        std::sort(sorted_samples.begin(), sorted_samples.end());
        size_t index = static_cast<size_t>(std::ceil(percentile / 100.0 * sorted_samples.size())) - 1;
        return sorted_samples[index];
    }

    mutable std::mutex m_mutex;
    std::map<std::string, Metrics> m_metrics;
}; 