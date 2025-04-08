#include <iostream>
#include <string>
#include <chrono>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

using namespace std;
using namespace std::chrono;

// Cross-platform socket close function
void closeSocket(int socket) {
    #ifdef _WIN32
        closesocket(socket);
    #else
        close(socket);
    #endif
}

class TypingClient {
private:
    int clientSocket;
    string serverIP;
    int serverPort;
    
    // Calculate words per minute
    double calculateWPM(const string& text, double timeInSeconds) {
        // Standard: 5 characters = 1 word
        return (text.length() / 5.0) / (timeInSeconds / 60.0);
    }
    
    // Calculate accuracy percentage
    double calculateAccuracy(const string& original, const string& typed) {
        int correctChars = 0;
        int minLength = min(original.length(), typed.length());
        
        for (int i = 0; i < minLength; i++) {
            if (original[i] == typed[i]) {
                correctChars++;
            }
        }
        
        return (double)correctChars / original.length() * 100.0;
    }

public:
    TypingClient(const string& ip, int port) : serverIP(ip), serverPort(port) {
        #ifdef _WIN32
            // Initialize Winsock
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                cerr << "WSAStartup failed" << endl;
                exit(1);
            }
        #endif

        // Create socket
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            cerr << "Error creating socket" << endl;
            exit(1);
        }
    }
    
    bool connectToServer() {
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        
        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            cerr << "Invalid address or address not supported" << endl;
            return false;
        }
        
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Connection failed" << endl;
            return false;
        }
        
        cout << "Connected to server at " << serverIP << ":" << serverPort << endl;
        return true;
    }
    
    void startGame() {
        char buffer[1024] = {0};
        
        cout << "Waiting for another player to join..." << endl;
        
        // Wait for START message from server
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        
        if (bytesRead <= 0) {
            cerr << "Server disconnected" << endl;
            return;
        }
        
        string message(buffer);
        
        if (message.substr(0, 6) != "START|") {
            cerr << "Unexpected message from server" << endl;
            return;
        }
        
        // Extract text to type
        string textToType = message.substr(6);
        
        cout << "\n=== Typing Test Started ===\n" << endl;
        cout << "Type the following text:" << endl;
        cout << textToType << endl;
        cout << "\nType the text and press Enter when finished." << endl;
        
        cin.ignore(); // Wait for user to press Enter
        
        // Start timing
        auto startTime = high_resolution_clock::now();
        
        // Get user input
        string userInput;
        getline(cin, userInput);
        
        // Stop timing
        auto endTime = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(endTime - startTime);
        double seconds = duration.count() / 1000.0;
        
        // Calculate results
        double wpm = calculateWPM(userInput, seconds);
        double accuracy = calculateAccuracy(textToType, userInput);
        
        // Display local results
        cout << "\n=== Your Results ===\n" << endl;
        cout << "Time: " << seconds << " seconds" << endl;
        cout << "WPM: " << wpm << endl;
        cout << "Accuracy: " << accuracy << "%" << endl;
        
        // Send results to server
        string resultMsg = "FINISH|" + to_string(wpm) + "|" + to_string(accuracy);
        send(clientSocket, resultMsg.c_str(), resultMsg.length(), 0);
        
        cout << "\nWaiting for other player to finish..." << endl;
        
        // Wait for results from server
        memset(buffer, 0, sizeof(buffer));
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        
        if (bytesRead <= 0) {
            cerr << "Server disconnected" << endl;
            return;
        }
        
        message = string(buffer);
        
        if (message.substr(0, 8) == "RESULTS|") {
            cout << "\n=== Final Results ===\n" << endl;
            
            // Parse results
            string resultsStr = message.substr(8);
            size_t pos = 0;
            while ((pos = resultsStr.find("|")) != string::npos) {
                string playerResult = resultsStr.substr(0, pos);
                resultsStr.erase(0, pos + 1);
                
                if (playerResult.empty()) continue;
                
                // Parse player result (id,wpm,accuracy)
                size_t commaPos1 = playerResult.find(",");
                size_t commaPos2 = playerResult.find(",", commaPos1 + 1);
                
                int playerId = stoi(playerResult.substr(0, commaPos1));
                double playerWpm = stod(playerResult.substr(commaPos1 + 1, commaPos2 - commaPos1 - 1));
                double playerAccuracy = stod(playerResult.substr(commaPos2 + 1));
                
                cout << "Player " << playerId + 1 << ":" << endl;
                cout << "  WPM: " << playerWpm << endl;
                cout << "  Accuracy: " << playerAccuracy << "%" << endl;
                cout << endl;
            }
        }
    cout << "Press Enter to quit..." << endl;
    string dummy;
    getline(cin, dummy);
    }
    
    ~TypingClient() {
        closeSocket(clientSocket);
        
        #ifdef _WIN32
            WSACleanup();
        #endif
    }
};

int main(int argc, char* argv[]) {
    string serverIP;
    int serverPort = 8080; // Default port

    cout << "Enter the server's IP address: ";
    cin >> serverIP;

    TypingClient client(serverIP, serverPort);
    if (client.connectToServer()) {
        client.startGame();
    }

    return 0;
}
