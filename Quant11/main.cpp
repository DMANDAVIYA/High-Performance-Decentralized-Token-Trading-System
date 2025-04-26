#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <drogon/drogon.h>
#include <json/json.h>
#include "order_manager.h"
#include "utility_manager.h"
#include "web_socket_client.h"
#include "token_manager.h"
#include "api_manager.h"

using namespace std;

void DisplayMenu()
{
    std::cout << "\nSelect an action:\n";
    std::cout << "1. Place Order\n";
    std::cout << "2. Modify Order\n";
    std::cout << "3. Cancel Order\n";
    std::cout << "4. Get Order Book\n";
    std::cout << "5. View Current Positions\n";
    std::cout << "6. Get Open Orders\n";
    std::cout << "7. Connect to WebSocket and Subscribe\n";
    std::cout << "8. Exit\n";
    std::cout << "Enter your choice: ";
}

int main()
{
    std::signal(SIGINT, UtilityManager::HandleExitSignal);
    std::ios_base::sync_with_stdio(false);

    try
    {
        TokenManager token_manager("access_token.txt", "refresh_token.txt", 2592000);
        OrderManager order_manager(token_manager);
        ApiManager api_manager(token_manager);  // Pass TokenManager to ApiManager
        WebSocketClient ws_client;

        if (!ws_client.connect("wss://test.deribit.com/ws/api/v2")) {
            std::cerr << "Failed to connect to WebSocket server" << std::endl;
            return 1;
        }

        std::string symbol = "BTC-PERPETUAL";
        std::string side = "buy";
        double price = 50000.0;
        double amount = 0.1;

        std::string order_json = api_manager.place_order(symbol, side, price, amount);
        ws_client.send_message(order_json);

        std::string response = ws_client.receive_message();
        std::cout << "Order response: " << response << std::endl;

        while (true)
        {
            if (!ws_client.is_connected()) {
                std::cerr << "WebSocket connection lost" << std::endl;
                break;
            }

            std::string message = ws_client.receive_message();
            std::cout << "Received: " << message << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        ws_client.disconnect();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}