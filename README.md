## High Performance Decentralized Token Trading System
A high-performance order execution and management system for trading using C++. This system provides real-time market data streaming, order management, and performance monitoring capabilities.

## Environment
- GCC Version: 6.3.0 (MinGW.org GCC-6.3.0-1)
- Operating System: Windows 10
- Compiler: MinGW

## Project Structure

```
Quant11/
├── Qaunt11/
│   ├── api_credentials.h/cpp      # API key and secret management
│   ├── api_manager.h/cpp          # Main API interface
│   ├── performance_monitor.h      # Performance metrics tracking
│   ├── token_manager.h/cpp        # Authentication token management
│   ├── web_socket_client.h/cpp    # WebSocket client for market data
│   └── web_socket_server.h        # WebSocket server for data distribution
├── build/
│   ├── api_key.txt               # API key storage
│   ├── api_secret.txt            # API secret storage
│   ├── access_token.txt          # Access token storage
│   └── refresh_token.txt         # Refresh token storage
└── api.env                       # Environment configuration
```

## Core Components

### 1. API Credentials Management (`api_credentials.h/cpp`)
- Manages API key and secret for authentication
- Supports environment variables and file-based configuration
- Implements secure credential storage and retrieval
- Validates API credentials before use

### 2. Token Management (`token_manager.h/cpp`)
- Handles authentication token lifecycle
- Manages access and refresh tokens
- Implements token expiration checking
- Provides token refresh functionality
- Stores tokens securely in files

### 3. API Manager (`api_manager.h/cpp`)
- Main interface for API interactions
- Implements REST API calls for:
  - Order placement and management
  - Position management
  - Account information retrieval
- Handles API response parsing and error management

### 4. WebSocket Client (`web_socket_client.h/cpp`)
- Establishes real-time connection
- Subscribes to market data channels:
  - Order book updates
  - Trade information
  - Ticker updates
- Implements message handling and parsing
- Manages connection state and reconnection

### 5. WebSocket Server (`web_socket_server.h`)
- Distributes market data to connected clients
- Manages client connections and subscriptions
- Implements broadcast functionality
- Tracks connection metrics

### 6. Performance Monitor (`performance_monitor.h`)
- Tracks system performance metrics:
  - Latency measurements
  - Operation counts
  - Statistical analysis (min, max, avg, std dev)
  - Percentile calculations (p95, p99)
- Provides JSON export of metrics
- Thread-safe implementation

## Dependencies

- C++11
- Boost C++ Libraries (for WebSocket)
- OpenSSL (for secure connections)
- JSON for Modern C++ (nlohmann/json)
- CMake (build system)

## Requirements

### System Requirements
- Windows 10
- MinGW GCC 6.3.0
- CMake 3.15 or later


### API Requirements
- Deribit API
- API access credentials
- Internet connection

## Building the Project

1. Install dependencies:
```bash
vcpkg install boost:x64-windows
vcpkg install openssl:x64-windows
vcpkg install nlohmann-json:x64-windows
```

2. Configure with CMake:
```bash
mkdir build
cd build
cmake ..
```

3. Build the project:
```bash
cmake --build . --config Release
```

## Configuration

1. Create `api.env` file with your credentials:
```
API_KEY=your_api_key
SECRET_KEY=your_secret_key
```

2. Set up token files:
- `build/access_token.txt`
- `build/refresh_token.txt`

## Usage

1. Initialize the API manager:
```cpp
ApiManager api_manager;
```

2. Connect to WebSocket:
```cpp
WebSocketClient ws_client;
ws_client.connect();
```

3. Subscribe to market data:
```cpp
ws_client.subscribe_to_orderbook("BTC-PERPETUAL");
```

4. Place orders:
```cpp
Order order;
order.instrument_name = "BTC-PERPETUAL";
order.amount = 122.0;
order.price = 52300.0;
order.type = OrderType::LIMIT;
order.side = OrderSide::SELL;

api_manager.place_order(order);
```

## Performance Monitoring

The system includes comprehensive performance monitoring:

```cpp
PerformanceMonitor monitor;

// Record latency
monitor.record_latency("order_placement", latency_ms);

// Get metrics
auto metrics = monitor.get_metrics("order_placement");

// Export metrics
monitor.export_metrics_to_file("performance_metrics.json");
```

## Security Features

- Secure credential storage
- Token-based authentication
- Encrypted WebSocket connections
- Environment variable support
- File-based configuration

## Error Handling

- Comprehensive error checking
- Exception handling
- Connection retry logic
- Token refresh mechanism
- Logging system

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request
