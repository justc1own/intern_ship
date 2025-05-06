#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace std;

class TimeUtils {
public:
    static int timeToMinutes(const string &timeStr) {
        size_t colonPos = timeStr.find(':');
        int hours = stoi(timeStr.substr(0, colonPos));
        int minutes = stoi(timeStr.substr(colonPos + 1));
        return hours * 60 + minutes;
    }

    static string minutesToTime(int minutes) {
        int hours = minutes / 60;
        minutes %= 60;
        ostringstream oss;
        oss << setw(2) << setfill('0') << hours << ":" 
            << setw(2) << setfill('0') << minutes;
        return oss.str();
    }
};

class Table {
private:
    int number;
    bool isOccupied;
    string currentClient;
    int startTime;
    int revenue;
    int totalTime;

public:
    Table(int num) : number(num), isOccupied(false), currentClient(""), 
                    startTime(0), revenue(0), totalTime(0) {}

    void occupy(const string &client, int time) {
        isOccupied = true;
        currentClient = client;
        startTime = time;
    }

    void release(int time, int rent) {
        if (!isOccupied) return;
        
        int timeSpent = time - startTime;
        int hours = (timeSpent + 59) / 60;
        revenue += hours * rent;
        totalTime += timeSpent;
        
        isOccupied = false;
        currentClient = "";
    }

    void forceRelease(int time, int rent) {
        if (isOccupied) {
            release(time, rent);
        }
    }

    bool getIsOccupied() const { return isOccupied; }
    int getNumber() const { return number; }
    int getRevenue() const { return revenue; }
    int getTotalTime() const { return totalTime; }
    string getCurrentClient() const { return currentClient; }
};

class Client {
private:
    string name;
    int arrivalTime;
    int tableNumber;

public:
    Client(const string &s, int time) : name(s), arrivalTime(time), tableNumber(-1) {}
    
    Client(const Client& other) 
        : name(other.name), 
          arrivalTime(other.arrivalTime), 
          tableNumber(other.tableNumber) {}

    Client(Client&& other) noexcept 
        : name(std::move(other.name)),
          arrivalTime(other.arrivalTime),
          tableNumber(other.tableNumber) {
        other.arrivalTime = 0;
        other.tableNumber = -1;
    }

    Client& operator=(const Client& other) {
        if (this != &other) {
            name = other.name;
            arrivalTime = other.arrivalTime;
            tableNumber = other.tableNumber;
        }
        return *this;
    }

    Client& operator=(Client&& other) noexcept {
        if (this != &other) {
            name = std::move(other.name);
            arrivalTime = other.arrivalTime;
            tableNumber = other.tableNumber;
            
            other.arrivalTime = 0;
            other.tableNumber = -1;
        }
        return *this;
    }

    Client() = default;

    void setTable(int table) { tableNumber = table; }
    void leaveTable() { tableNumber = -1; }

    string getName() const { return name; }
    int getArrivalTime() const { return arrivalTime; }
    int getTableNumber() const { return tableNumber; }
    bool isSeated() const { return tableNumber != -1; }
};

class Event {
private:
    int time;
    int id;
    vector<string> body;

public:
    Event(int t, int i, const vector<string> &b) : time(t), id(i), body(b) {}

    int getTime() const { return time; }
    int getId() const { return id; }
    vector<string> getBody() const { return body; }

    string toString() const {
        ostringstream oss;
        oss << TimeUtils::minutesToTime(time) << " " << id;
        for (const auto &token : body) {
            oss << " " << token;
        }
        return oss.str();
    }
};

class ComputerClub {
private:
    int openTime;
    int closeTime;
    int rent;
    //int freeTables;
    vector<Table> tables;
    
    map<string, Client> clients;
    queue<string> waitingQueue;
    vector<Event> outputEvents;

    bool isClientInClub(const string &clientName) const {
        return clients.find(clientName) != clients.end();
    }

    bool isTableNumberValid(int tableNumber) const {
        return tableNumber >= 1 && tableNumber <= tables.size();
    }

    bool hasFreeTables() const {
        for (const auto &table : tables) {
            if (!table.getIsOccupied()) return true;
        }
        return false;
    }

    void addErrorEvent(int time, const string &error) {
        outputEvents.emplace_back(time, 13, vector<string>{error});
    }

    void processClientArrived(const Event &event) {
        string clientName = event.getBody()[0];
        
        if (isClientInClub(clientName)) {
            addErrorEvent(event.getTime(), "YouShallNotPass");
        } else if (event.getTime() < openTime || event.getTime() >= closeTime) {
            addErrorEvent(event.getTime(), "NotOpenYet");
        } else {
            clients.emplace(clientName, Client(clientName, event.getTime()));
        }
    }

    void processClientSat(const Event &event) {
        string clientName = event.getBody()[0];
        int tableNumber = stoi(event.getBody()[1]);

        if (!isClientInClub(clientName)) {
            addErrorEvent(event.getTime(), "ClientUnknown");
        } else if (!isTableNumberValid(tableNumber)) {
            addErrorEvent(event.getTime(), "PlaceIsBusy");
        } else if (tables[tableNumber - 1].getIsOccupied()) {
            addErrorEvent(event.getTime(), "PlaceIsBusy");
        } else {
            Client &client = clients[clientName];
            
            if (client.isSeated()) {
                int prevTable = client.getTableNumber() - 1;
                tables[prevTable].forceRelease(event.getTime(), rent);
            }

            tables[tableNumber - 1].occupy(clientName, event.getTime());
            client.setTable(tableNumber);
        }
    }

    void processClientWaiting(const Event &event) {
        string clientName = event.getBody()[0];

        if (!isClientInClub(clientName)) {
            addErrorEvent(event.getTime(), "ClientUnknown");
        } else if (hasFreeTables()) {
            addErrorEvent(event.getTime(), "ICanWaitNoLonger!");
        } else if (waitingQueue.size() >= tables.size()) {
            outputEvents.emplace_back(event.getTime(), 11, vector<string>{clientName});
            clients.erase(clientName);
        } else {
            waitingQueue.push(clientName);
        }
    }

    void processClientLeft(const Event &event) {
        string clientName = event.getBody()[0];

        if (!isClientInClub(clientName)) {
            addErrorEvent(event.getTime(), "ClientUnknown");
        } else {
            Client &client = clients[clientName];
            
            if (client.isSeated()) {
                int tableNumber = client.getTableNumber();
                tables[tableNumber - 1].release(event.getTime(), rent);

                // look if there are clients waiting
                if (!waitingQueue.empty()) {
                    string nextClient = waitingQueue.front();
                    waitingQueue.pop();
                    tables[tableNumber - 1].occupy(nextClient, event.getTime());
                    clients[nextClient].setTable(tableNumber);
                    outputEvents.emplace_back(event.getTime(), 12, 
                        vector<string>{nextClient, to_string(tableNumber)});
                }
            }
            
            clients.erase(clientName);
        }
    }

public:
    ComputerClub(int numTables, const string &openStr, const string &closeStr, int rate) 
        : openTime(TimeUtils::timeToMinutes(openStr)),
          closeTime(TimeUtils::timeToMinutes(closeStr)),
          rent(rate) {
        for (int i = 1; i <= numTables; ++i) {
            tables.emplace_back(i);
        }
    }

    void processEvent(const Event &event) {
        outputEvents.push_back(event);

        switch (event.getId()) {
            case 1: processClientArrived(event); break;
            case 2: processClientSat(event); break;
            case 3: processClientWaiting(event); break;
            case 4: processClientLeft(event); break;
        }
    }

    void closeClub() { // at closing time remaining clients leave
        vector<string> remainingClients;
        for (const auto &client : clients) {
            remainingClients.push_back(client.first);
        }
        sort(remainingClients.begin(), remainingClients.end());

        for (const auto &clientName : remainingClients) {
            outputEvents.emplace_back(closeTime, 11, vector<string>{clientName});
            if (clients[clientName].isSeated()) {
                int tableNumber = clients[clientName].getTableNumber();
                tables[tableNumber - 1].forceRelease(closeTime, rent);
            }
        }
    }

    void printResults() const {
        cout << TimeUtils::minutesToTime(openTime) << endl;

        // Sort all events by time and print them
        vector<Event> sortedEvents = outputEvents;
        sort(sortedEvents.begin(), sortedEvents.end(), 
            [](const Event &a, const Event &b) { return (a.getTime() != b.getTime() ? a.getTime() < b.getTime() : a.getId() < b.getId()); });

        for (const auto &event : sortedEvents) {
            cout << event.toString() << endl;
        }

        cout << TimeUtils::minutesToTime(closeTime) << endl;

        for (const auto &table : tables) {
            cout << table.getNumber() << " " << table.getRevenue() << " " 
                 << TimeUtils::minutesToTime(table.getTotalTime()) << endl;
        }
    }
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    ifstream inputFile(argv[1]);
    if (!inputFile) {
        cerr << "Error opening file: " << argv[1] << endl;
        return 1;
    }

    int numTables;
    inputFile >> numTables;

    string openTimeStr, closeTimeStr;
    inputFile >> openTimeStr >> closeTimeStr;

    int rent;
    inputFile >> rent;

    vector<Event> events;
    string line;
    getline(inputFile, line);

    auto isValidTime = [](const string& timeStr) -> bool {
        if (timeStr.size() != 5 || timeStr[2] != ':') return false;
        try {
            int hours = stoi(timeStr.substr(0, 2));
            int minutes = stoi(timeStr.substr(3, 2));
            return hours >= 0 && hours < 24 && minutes >= 0 && minutes < 60;
        } catch (...) {
            return false;
        }
    };
    
    auto isValidClientName = [](const string& name) -> bool {
        return !name.empty() && all_of(name.begin(), name.end(), [](char c) {
            return isalnum(c) || c == '_' || c == '-';
        });
    };
    
    while (getline(inputFile, line)) {
        istringstream iss(line);
        string timeStr;
        int id;
        
        // Проверка формата времени и ID события
        if (!(iss >> timeStr >> id) || !isValidTime(timeStr) || id < 1 || id > 4) {
            cerr << "Error line: " << line << endl;
            return 1;
        }
    
        vector<string> body;
        string token;
        while (iss >> token) {
            body.push_back(token);
        }
    
        bool valid = true;
        switch (id) {
            case 1:
                if (body.size() != 1 || !isValidClientName(body[0])) {
                    valid = false;
                }
                break;
                
            case 2:
                if (body.size() != 2 || !isValidClientName(body[0])) {
                    valid = false;
                } else {
                    try {
                        int table = stoi(body[1]);
                        if (table <= 0) valid = false;
                    } catch (...) {
                        valid = false;
                    }
                }
                break;
                
            case 3:
                if(!isValidClientName(body[0])) {
                    valid = false;
                }
                break;
            case 4: 
                if (body.size() != 1 || !isValidClientName(body[0])) {
                    valid = false;
                }
                break;
        }
    
        if (!valid) {
            cerr << "Error line: " << line << endl;
            return 1;
        }
    
        events.emplace_back(TimeUtils::timeToMinutes(timeStr), id, body);
    }

    ComputerClub club(numTables, openTimeStr, closeTimeStr, rent);

    for (const auto &event : events) {
        club.processEvent(event);
    }

    club.closeClub();

    club.printResults();

    return 0;
}