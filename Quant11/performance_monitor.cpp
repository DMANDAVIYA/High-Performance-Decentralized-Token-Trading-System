#include "performance_monitor.h"
#include <json/json.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

PerformanceMonitor::PerformanceMonitor() : m_start_time(std::chrono::steady_clock::now()) {}

void PerformanceMonitor::record_latency(const std::string& operation, uint64_t latency_us) {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    auto& metrics = m_metrics[operation];
    
    metrics.total_count++;
    metrics.total_latency += latency_us;
    
    uint64_t current_min = metrics.min_latency.load();
    while (latency_us < current_min && 
           !metrics.min_latency.compare_exchange_weak(current_min, latency_us)) {}
    
    uint64_t current_max = metrics.max_latency.load();
    while (latency_us > current_max && 
           !metrics.max_latency.compare_exchange_weak(current_max, latency_us)) {}
    
    {
        std::lock_guard<std::mutex> samples_lock(metrics.samples_mutex);
        metrics.latency_samples.push_back(latency_us);
    }
}

void PerformanceMonitor::start_operation(const std::string& operation) {
    // Implementation for tracking operation start time
    // This would be used for more detailed timing analysis
}

void PerformanceMonitor::end_operation(const std::string& operation) {
    // Implementation for tracking operation end time
    // This would be used for more detailed timing analysis
}

double PerformanceMonitor::get_average_latency(const std::string& operation) const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    auto it = m_metrics.find(operation);
    if (it != m_metrics.end() && it->second.total_count > 0) {
        return static_cast<double>(it->second.total_latency) / it->second.total_count;
    }
    return 0.0;
}

uint64_t PerformanceMonitor::get_min_latency(const std::string& operation) const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    auto it = m_metrics.find(operation);
    if (it != m_metrics.end()) {
        return it->second.min_latency;
    }
    return 0;
}

uint64_t PerformanceMonitor::get_max_latency(const std::string& operation) const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    auto it = m_metrics.find(operation);
    if (it != m_metrics.end()) {
        return it->second.max_latency;
    }
    return 0;
}

uint64_t PerformanceMonitor::get_total_operations(const std::string& operation) const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    auto it = m_metrics.find(operation);
    if (it != m_metrics.end()) {
        return it->second.total_count;
    }
    return 0;
}

double PerformanceMonitor::get_operations_per_second(const std::string& operation) const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    auto it = m_metrics.find(operation);
    if (it != m_metrics.end()) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_time).count();
        if (duration > 0) {
            return static_cast<double>(it->second.total_count) / duration;
        }
    }
    return 0.0;
}

double PerformanceMonitor::get_latency_percentile(const std::string& operation, double percentile) const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    auto it = m_metrics.find(operation);
    if (it != m_metrics.end()) {
        std::lock_guard<std::mutex> samples_lock(it->second.samples_mutex);
        auto& samples = it->second.latency_samples;
        if (!samples.empty()) {
            std::vector<uint64_t> sorted_samples = samples;
            std::sort(sorted_samples.begin(), sorted_samples.end());
            size_t index = static_cast<size_t>(percentile * (sorted_samples.size() - 1));
            return static_cast<double>(sorted_samples[index]);
        }
    }
    return 0.0;
}

double PerformanceMonitor::get_latency_standard_deviation(const std::string& operation) const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    auto it = m_metrics.find(operation);
    if (it != m_metrics.end()) {
        std::lock_guard<std::mutex> samples_lock(it->second.samples_mutex);
        auto& samples = it->second.latency_samples;
        if (!samples.empty()) {
            double mean = static_cast<double>(it->second.total_latency) / it->second.total_count;
            double sum_squares = 0.0;
            for (uint64_t latency : samples) {
                double diff = static_cast<double>(latency) - mean;
                sum_squares += diff * diff;
            }
            return std::sqrt(sum_squares / samples.size());
        }
    }
    return 0.0;
}

void PerformanceMonitor::print_metrics() const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    std::cout << "\nPerformance Metrics:\n";
    std::cout << "===================\n\n";
    
    for (const auto& pair : m_metrics) {
        const auto& operation = pair.first;
        std::cout << operation << ":\n";
        std::cout << "  Total Operations: " << get_total_operations(operation) << "\n";
        std::cout << "  Operations/sec: " << std::fixed << std::setprecision(2) 
                  << get_operations_per_second(operation) << "\n";
        std::cout << "  Average Latency: " << std::fixed << std::setprecision(2) 
                  << get_average_latency(operation) << " μs\n";
        std::cout << "  Min Latency: " << get_min_latency(operation) << " μs\n";
        std::cout << "  Max Latency: " << get_max_latency(operation) << " μs\n";
        std::cout << "  P50 Latency: " << std::fixed << std::setprecision(2) 
                  << get_latency_percentile(operation, 0.5) << " μs\n";
        std::cout << "  P95 Latency: " << std::fixed << std::setprecision(2) 
                  << get_latency_percentile(operation, 0.95) << " μs\n";
        std::cout << "  P99 Latency: " << std::fixed << std::setprecision(2) 
                  << get_latency_percentile(operation, 0.99) << " μs\n";
        std::cout << "  Std Dev: " << std::fixed << std::setprecision(2) 
                  << get_latency_standard_deviation(operation) << " μs\n\n";
    }
}

std::string PerformanceMonitor::get_metrics_json() const {
    std::lock_guard<std::mutex> lock(m_metrics_mutex);
    Json::Value root;
    
    for (const auto& pair : m_metrics) {
        const auto& operation = pair.first;
        Json::Value metrics;
        metrics["total_operations"] = Json::Value::UInt64(get_total_operations(operation));
        metrics["operations_per_second"] = get_operations_per_second(operation);
        metrics["average_latency"] = get_average_latency(operation);
        metrics["min_latency"] = Json::Value::UInt64(get_min_latency(operation));
        metrics["max_latency"] = Json::Value::UInt64(get_max_latency(operation));
        metrics["p50_latency"] = get_latency_percentile(operation, 0.5);
        metrics["p95_latency"] = get_latency_percentile(operation, 0.95);
        metrics["p99_latency"] = get_latency_percentile(operation, 0.99);
        metrics["standard_deviation"] = get_latency_standard_deviation(operation);
        
        root[operation] = metrics;
    }
    
    Json::StreamWriterBuilder writer;
    return Json::writeString(writer, root);
} 