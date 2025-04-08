#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <ctime>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
#endif

using namespace std;

// Cross-platform socket close function
void closeSocket(int socket) {
    #ifdef _WIN32
        closesocket(socket);
    #else
        close(socket);
    #endif
}

// Function to get the local machine's IP address
string getLocalIPAddress() {
    string localIP = "127.0.0.1"; // Default to localhost in case of failure

    // Create a temporary socket
    int tempSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (tempSocket == -1) {
        cerr << "Failed to create socket for IP detection." << endl;
        return localIP;
    }

    struct sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(80); // Arbitrary port
    inet_pton(AF_INET, "8.8.8.8", &remoteAddr.sin_addr); // Google's public DNS

    // Connect to Google's DNS server
    if (connect(tempSocket, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr)) == -1) {
        cerr << "Failed to connect for IP detection." << endl;
        closeSocket(tempSocket);
        return localIP;
    }

    // Get the local address used for the connection
    struct sockaddr_in localAddr;
    socklen_t addrLen = sizeof(localAddr);
    if (getsockname(tempSocket, (struct sockaddr*)&localAddr, &addrLen) == -1) {
        cerr << "Failed to get local IP address." << endl;
        closeSocket(tempSocket);
        return localIP;
    }

    char ipBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &localAddr.sin_addr, ipBuffer, sizeof(ipBuffer));
    localIP = string(ipBuffer);

    closeSocket(tempSocket);
    return localIP;
}

// Game state
struct PlayerResult {
    int id;
    double wpm;
    double accuracy;
    bool finished;
};

class TypingServer {
private:
    int serverSocket;
    vector<int> clientSockets;
    vector<PlayerResult> results;
    string typingText;
    mutex resultsMutex;
    bool gameStarted;
    int playersFinished;

    // Sample texts
    vector<string> sampleTexts = {
        "The quick brown fox jumped over a sleepy dog lying in the golden sunlight.",
        "She walked quietly through the forest, listening to birds chirping and leaves rustling overhead.",
        "Time passed slowly as the storm raged outside the small wooden cabin in the hills.",
        "He typed rapidly, his fingers dancing across the keyboard like a pianist in a concert.",
        "They packed their bags and left for the mountains before the sun could rise.",
        "The stars shimmered brightly in the night sky, illuminating the path through the trees.",
        "A good book can transport you to magical places beyond your wildest imagination.",
        "The coffee shop buzzed with life, full of laughter, conversation, and the smell of espresso.",
        "She smiled at the child playing with a red balloon on the busy street corner.",
        "Learning new skills requires patience, practice, and the willingness to make many small mistakes.",
        "The train arrived late, screeching to a stop at the nearly empty platform.",
        "His heart pounded as he stood in front of the crowd, preparing to speak.",
        "The lighthouse stood tall, casting light across the choppy sea during the stormy night.",
        "A rainbow stretched across the sky after the heavy afternoon rain had finally passed.",
        "She tied her shoes tightly and began her morning run through the quiet neighborhood.",
        "He watched the sunset paint the clouds orange, pink, and purple above the ocean.",
        "The wind blew strongly, carrying the scent of rain and freshly cut grass.",
        "The old clock ticked loudly in the room, counting the seconds in perfect rhythm.",
        "She opened the envelope slowly, hands trembling with anticipation and a hint of fear.",
        "Their footsteps echoed through the vast, empty hall of the ancient abandoned building.",
        "The little girl clutched her teddy bear tightly as the thunder rolled outside.",
        "He found peace in the silence of early mornings before the world woke up.",
        "A paper airplane soared across the room and landed perfectly on the teacher's desk.",
        "The streetlamp flickered, casting shadows that danced across the pavement and nearby brick walls.",
        "Her voice trembled slightly as she read the poem aloud to her classmates.",
        "The fire crackled gently in the fireplace, warming the cold room with golden light.",
        "He scribbled down ideas on a napkin while waiting for his coffee to brew.",
        "The car sped down the highway, headlights cutting through the thick evening fog.",
        "She carefully folded the letter and placed it back inside the dusty old box.",
        "The cat stared intently at the moving shadow beneath the couch, ready to pounce.",
        "He laughed loudly, wiping tears from his eyes after hearing the joke again.",
        "A leaf drifted slowly to the ground, carried by the gentle autumn breeze.",
        "They ran through the rain, shoes soaked and clothes clinging to their skin.",
        "The museum was filled with relics from forgotten times and distant civilizations.",
        "She listened closely to the whisper of the wind through the open window.",
        "He opened the book and began reading as the train rolled steadily forward.",
        "The storm passed, leaving behind puddles, broken branches, and a deep sense of calm.",
        "The bakery smelled of fresh bread, warm butter, and sweet cinnamon rolls.",
        "They watched the stars together, lying on the grass and talking about dreams.",
        "He picked up the guitar and strummed a tune that filled the quiet room.",
        "The computer screen glowed softly, displaying lines of code scrolling endlessly.",
        "She held his hand tightly, afraid to let go in the unfamiliar crowd.",
        "The city lights flickered in the distance as the plane descended through the clouds.",
        "A dog barked in the distance, breaking the silence of the peaceful evening.",
        "The boat rocked gently in the harbor, tied securely to the wooden dock.",
        "The classroom buzzed with excitement as the final bell rang for summer vacation.",
        "The movie ended, but the characters stayed in her mind long after the credits.",
        "He took a deep breath and dove into the clear, cold water of the lake.",
        "She looked up from her book and smiled at the sound of laughter nearby.",
        "The candle burned slowly, its flame swaying with every breath of air inside the room."
    };

public:
    TypingServer(int port) : gameStarted(false), playersFinished(0) {
        #ifdef _WIN32
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                cerr << "WSAStartup failed" << endl;
                exit(1);
            }
        #endif

        // Get the local IP address
        string localIP = getLocalIPAddress();
        cout << "Server will bind to IP: " << localIP << endl;

        // Create socket
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            cerr << "Error creating socket" << endl;
            exit(1);
        }

        // Set socket options
        int opt = 1;
        #ifdef _WIN32
            setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
        #else
            setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        #endif

        // Bind to the retrieved IP and port
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        inet_pton(AF_INET, localIP.c_str(), &serverAddr.sin_addr);
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Error binding socket" << endl;
            exit(1);
        }

        // Listen for connections
        if (listen(serverSocket, 5) < 0) {
            cerr << "Error listening" << endl;
            exit(1);
        }

        cout << "Server started on " << localIP << ":" << port << endl;
        
        // Select a random text for this game
        typingText = sampleTexts[rand() % sampleTexts.size()];
    }

    void acceptConnections() {
        while (clientSockets.size() < 2) {
            struct sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
            if (clientSocket < 0) {
                cerr << "Error accepting connection" << endl;
                continue;
            }

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
            cout << "Client connected from: " << clientIP << endl;
            
            clientSockets.push_back(clientSocket);
            
            // Initialize player result
            PlayerResult newPlayer;
            newPlayer.id = clientSockets.size() - 1;
            newPlayer.wpm = 0;
            newPlayer.accuracy = 0;
            newPlayer.finished = false;
            
            resultsMutex.lock();
            results.push_back(newPlayer);
            resultsMutex.unlock();
            
            // Start a thread to handle this client
            thread clientThread(&TypingServer::handleClient, this, clientSocket, clientSockets.size() - 1);
            clientThread.detach();
        }
        
        // Once we have 2 players, start the game
        startGame();
    }

    void startGame() {
        gameStarted = true;
        cout << "Starting game with 2 players" << endl;
        
        // Send the typing text to all clients
        string startMsg = "START|" + typingText;
        for (int socket : clientSockets) {
            send(socket, startMsg.c_str(), startMsg.length(), 0);
        }
    }

    void handleClient(int clientSocket, int playerId) {
        char buffer[1024] = {0};
        
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            
            if (bytesRead <= 0) {
                cout << "Client " << playerId << " disconnected" << endl;
                break;
            }
            
            string message(buffer);
            
            // Parse message
            if (message.substr(0, 7) == "FINISH|") {
                // Format: FINISH|WPM|ACCURACY
                size_t pos = message.find("|", 7);
                double wpm = stod(message.substr(7, pos - 7));
                double accuracy = stod(message.substr(pos + 1));
                
                // Update player results
                resultsMutex.lock();
                results[playerId].wpm = wpm;
                results[playerId].accuracy = accuracy;
                results[playerId].finished = true;
                playersFinished++;
                resultsMutex.unlock();
                
                cout << "Player " << playerId << " finished with WPM: " << wpm << ", Accuracy: " << accuracy << "%" << endl;
                
                // If all players finished, send results to everyone
                if (playersFinished == 2) {
                    sendResults();
                }
            }
        }
        
        closeSocket(clientSocket);
    }

    void sendResults() {
        string resultsMsg = "RESULTS|";
        
        for (const auto& result : results) {
            resultsMsg += to_string(result.id) + "," + 
                         to_string(result.wpm) + "," + 
                         to_string(result.accuracy) + "|";
        }
        
        // Send results to all clients
        for (int socket : clientSockets) {
            send(socket, resultsMsg.c_str(), resultsMsg.length(), 0);
        }
        
        cout << "Game finished, results sent to all players" << endl;
    }

    ~TypingServer() {
        for (int socket : clientSockets) {
            closeSocket(socket);
        }
        closeSocket(serverSocket);
        
        #ifdef _WIN32
            WSACleanup();
        #endif
    }
};

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    TypingServer server(8080);
    server.acceptConnections();
    
    cout << "\nPress Ctrl+C to stop the server." << endl;
    
    while (true) {
        #ifdef _WIN32
            Sleep(10000); // Sleep for 10 seconds (Windows)
        #else
            sleep(10); // Sleep for 10 seconds (Unix)
        #endif
    }
    
    return 0;
}
