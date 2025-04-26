cmake_minimum_required(VERSION 3.10)
project(Qaunt11)

# Set C++14 standard for GCC 6.3.0 compatibility
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the directories containing the headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add the directories containing the libraries
link_directories(${CMAKE_SOURCE_DIR}/lib)

# Find the required packages
find_package(websocketpp REQUIRED)
find_package(Jsoncpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# Add the source files
set(SOURCES
    GoQuantOEMSApp/main.cpp 
    GoQuantOEMSApp/api_manager.cpp
    GoQuantOEMSApp/utility_manager.cpp
    GoQuantOEMSApp/web_socket_client.cpp
    GoQuantOEMSApp/token_manager.cpp
    GoQuantOEMSApp/api_credentials.cpp
)

# Add the executable
add_executable(GoQuantOEMS ${SOURCES})

# Link the libraries
target_link_libraries(GoQuantOEMS
    websocketpp
    jsoncpp
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
    ws2_32  # Windows socket library
    crypt32  # Windows crypto library
)

