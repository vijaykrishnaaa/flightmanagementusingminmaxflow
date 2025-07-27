#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <map>

using namespace std;

class PushRelabel {
public:
    struct Edge {
        int to, capacity, flow, reverse;
    };

    int n;
    vector<vector<Edge>> adj;
    vector<int> height, excess;
    map<int, string> flightNames;
    map<int, int> assignedPassengers;
    map<int, int> remainingSeats;
    map<int, bool> flightStatus;
    map<int, string> nodeDescriptions;
    map<int, int> waitingList;
    map<int, int> refundedPassengers;

    PushRelabel(int vertices) : n(vertices), adj(vertices), height(vertices), excess(vertices) {}

    void addEdge(int u, int v, int capacity) {
        adj[u].push_back({v, capacity, 0, (int)adj[v].size()});
        adj[v].push_back({u, 0, 0, (int)adj[u].size() - 1});
    }

    void push(int u, Edge &e) {
        int flowToPush = min(excess[u], e.capacity - e.flow);
        if (flowToPush > 0) {
            e.flow += flowToPush;
            adj[e.to][e.reverse].flow -= flowToPush;
            excess[u] -= flowToPush;
            excess[e.to] += flowToPush;
        }
    }

    void relabel(int u) {
        int minHeight = INT_MAX;
        for (Edge &e : adj[u]) {
            if (e.capacity - e.flow > 0) {
                minHeight = min(minHeight, height[e.to]);
            }
        }
        if (minHeight < INT_MAX)
            height[u] = minHeight + 1;
    }

    void pushRelabel(int source, int sink) {
        fill(height.begin(), height.end(), 0);
        fill(excess.begin(), excess.end(), 0);
        height[source] = n;
        queue<int> q;

        for (Edge &e : adj[source]) {
            int v = e.to;
            int flowToPush = e.capacity;
            if (flowToPush > 0) {
                e.flow = flowToPush;
                adj[v][e.reverse].flow = -flowToPush;
                excess[v] += flowToPush;
                excess[source] -= flowToPush;
                if (v != sink) q.push(v);
            }
        }

        while (!q.empty()) {
            int u = q.front();
            q.pop();

            if (excess[u] > 0) {
                bool pushed = false;
                for (Edge &e : adj[u]) {
                    if (e.capacity - e.flow > 0 && height[u] == height[e.to] + 1) {
                        push(u, e);
                        if (excess[e.to] > 0 && e.to != source && e.to != sink) q.push(e.to);
                        pushed = true;
                        break;
                    }
                }

                if (!pushed) {
                    relabel(u);
                    q.push(u);
                }
            }
        }
    }

    void recordPassengerAllocation() {
        for (int flight = 1; flight <= 3; flight++) {
            if (!flightStatus[flight]) continue;
            
            for (Edge &e : adj[flight]) {
                if (e.to == flight + 6) {  // Edge to capacity node
                    assignedPassengers[flight] = e.flow;
                    remainingSeats[flight + 6] = e.capacity - e.flow;
                    break;
                }
            }
        }
    }

    void reallocatePassengers(int canceledFlight) {
        int passengersToReallocate = assignedPassengers[canceledFlight];
        assignedPassengers[canceledFlight] = 0;
        remainingSeats[canceledFlight + 6] = 0;

        // Update graph edges for canceled flight
        for (Edge &e : adj[canceledFlight]) {
            if (e.to == canceledFlight + 6) {
                e.flow = 0;
                adj[e.to][e.reverse].flow = 0;
                break;
            }
        }

        cout << "\nReallocating passengers from canceled flight " << flightNames[canceledFlight] << "...\n";

        vector<pair<int, int>> availableFlights;
        for (int flight = 1; flight <= 3; flight++) {
            if (flight != canceledFlight && flightStatus[flight]) {
                int availableSeats = remainingSeats[flight + 6];
                if (availableSeats > 0) {
                    availableFlights.push_back({availableSeats, flight});
                }
            }
        }

        sort(availableFlights.rbegin(), availableFlights.rend());

        for (auto &flight : availableFlights) {
            int availableSeats = flight.first;
            int flightNum = flight.second;
            
            if (passengersToReallocate == 0) break;
            int assigned = min(availableSeats, passengersToReallocate);
            
            assignedPassengers[flightNum] += assigned;
            remainingSeats[flightNum + 6] -= assigned;
            
            // Update graph edges for target flight
            for (Edge &e : adj[flightNum]) {
                if (e.to == flightNum + 6) {
                    e.flow = assignedPassengers[flightNum];
                    adj[e.to][e.reverse].flow = -assignedPassengers[flightNum];
                    break;
                }
            }
            
            passengersToReallocate -= assigned;
            cout << "Moved " << assigned << " passengers to " << flightNames[flightNum] << endl;
        }

        if (passengersToReallocate > 0) {
            cout << "Could not reallocate " << passengersToReallocate << " passengers\n";
            cout << "Options:\n";
            cout << "1. Add to waiting list\n";
            cout << "2. Refund passengers\n";
            cout << "Enter your choice (1 or 2): ";
            
            int choice;
            cin >> choice;
            
            if (choice == 1) {
                waitingList[canceledFlight] += passengersToReallocate;
                cout << passengersToReallocate << " passengers added to waiting list\n";
            } else {
                refundedPassengers[canceledFlight] += passengersToReallocate;
                cout << passengersToReallocate << " passengers refunded\n";
            }
        }
    }

    void cancelFlight(int flightNode) {
        if (!flightStatus[flightNode]) {
            cout << "\nFlight " << flightNames[flightNode] << " is already canceled.\n";
            return;
        }
        cout << "\nCanceling " << flightNames[flightNode] << "...\n";
        flightStatus[flightNode] = false;
        reallocatePassengers(flightNode);
    }

    void adjustSeats(int flightNode, int newCapacity) {
        if (newCapacity < 0) {
            cout << "Error: Capacity cannot be negative\n";
            return;
        }
        
        int currentPassengers = assignedPassengers[flightNode];
        if (newCapacity < currentPassengers) {
            int overbooked = currentPassengers - newCapacity;
            cout << "Warning: Reducing capacity would overbook by " << overbooked << " passengers\n";
            cout << "Options:\n";
            cout << "1. Keep current passengers and set capacity to " << currentPassengers << "\n";
            cout << "2. Move " << overbooked << " passengers to waiting list\n";
            cout << "3. Refund " << overbooked << " passengers\n";
            cout << "Enter your choice (1-3): ";
            
            int choice;
            cin >> choice;
            
            if (choice == 1) {
                newCapacity = currentPassengers;
            } else if (choice == 2) {
                waitingList[flightNode] += overbooked;
                assignedPassengers[flightNode] = newCapacity;
            } else {
                refundedPassengers[flightNode] += overbooked;
                assignedPassengers[flightNode] = newCapacity;
            }
        }
        
        // Update capacity and remaining seats
        int addedCapacity = newCapacity - (assignedPassengers[flightNode] + remainingSeats[flightNode + 6]);
        remainingSeats[flightNode + 6] += addedCapacity;
        
        // Update the capacity edge in the graph
        for (Edge &e : adj[flightNode]) {
            if (e.to == flightNode + 6) {
                e.capacity = newCapacity;
                adj[e.to][e.reverse].capacity = newCapacity;
                break;
            }
        }
        
        cout << "\nAdjusted " << flightNames[flightNode] << " capacity to " << newCapacity << " seats.\n";
    }

    void processWaitingList(int flightNode) {
        if (waitingList[flightNode] == 0) {
            cout << "No passengers on waiting list for " << flightNames[flightNode] << "\n";
            return;
        }
        
        int availableSeats = remainingSeats[flightNode + 6];
        if (availableSeats <= 0) {
            cout << "No available seats for waiting list passengers\n";
            return;
        }
        
        int toAssign = min(availableSeats, waitingList[flightNode]);
        assignedPassengers[flightNode] += toAssign;
        remainingSeats[flightNode + 6] -= toAssign;
        waitingList[flightNode] -= toAssign;
        
        // Update graph edges
        for (Edge &e : adj[flightNode]) {
            if (e.to == flightNode + 6) {
                e.flow += toAssign;
                adj[e.to][e.reverse].flow -= toAssign;
                break;
            }
        }
        
        cout << "Assigned " << toAssign << " passengers from waiting list to " << flightNames[flightNode] << "\n";
        cout << waitingList[flightNode] << " passengers remain on waiting list\n";
    }

    void refundPassengers(int flightNode, int count) {
        if (count <= 0) return;
        
        if (waitingList[flightNode] >= count) {
            waitingList[flightNode] -= count;
            refundedPassengers[flightNode] += count;
            cout << "Refunded " << count << " passengers from waiting list\n";
        } else {
            int availableToRefund = waitingList[flightNode];
            waitingList[flightNode] = 0;
            refundedPassengers[flightNode] += availableToRefund;
            cout << "Refunded " << availableToRefund << " passengers (only this many were on waiting list)\n";
        }
    }

    void printGraph() {
        cout << "\nFlow Network:\n";
        for (int i = 0; i < n; i++) {
            if (!nodeDescriptions[i].empty()) {
                cout << "Node " << i << " (" << nodeDescriptions[i] << "):\n";
            }
            for (auto &e : adj[i]) {
                if (e.capacity > 0) {
                    cout << "  -> Node " << e.to << " | Cap: " << e.capacity 
                         << ", Flow: " << e.flow << "\n";
                }
            }
        }
    }

    void printAllocation() {
        cout << "\nCurrent Allocation:\n";
        for (int flight = 1; flight <= 3; flight++) {
            if (flightStatus[flight]) {
                cout << flightNames[flight] << ": " << assignedPassengers[flight] << " passengers ("
                     << remainingSeats[flight + 6] << " seats remaining)";
                if (waitingList[flight] > 0) {
                    cout << ", " << waitingList[flight] << " on waiting list";
                }
                cout << "\n";
            } else {
                cout << flightNames[flight] << ": CANCELED";
                if (waitingList[flight] > 0) {
                    cout << ", " << waitingList[flight] << " passengers awaiting reallocation";
                }
                cout << "\n";
            }
        }
        
        bool anyRefunds = false;
        for (auto& entry : refundedPassengers) {
            if (entry.second > 0) {
                if (!anyRefunds) {
                    cout << "Refunded Passengers:\n";
                    anyRefunds = true;
                }
                cout << flightNames[entry.first] << ": " << entry.second << " passengers refunded\n";
            }
        }
    }
};

int main() {
    int source = 0, sink = 10;
    PushRelabel pr(11);

    // Describe all nodes
    pr.nodeDescriptions[0] = "SOURCE (All passengers)";
    pr.nodeDescriptions[1] = "Flight 1 (9:00 AM)";
    pr.nodeDescriptions[2] = "Flight 2 (12:00 PM)";
    pr.nodeDescriptions[3] = "Flight 3 (5:00 PM)";
    pr.nodeDescriptions[4] = "Business Class Hub";
    pr.nodeDescriptions[5] = "Economy Class Hub";
    pr.nodeDescriptions[6] = "Connecting Flights Hub";
    pr.nodeDescriptions[7] = "Flight 1 Capacity";
    pr.nodeDescriptions[8] = "Flight 2 Capacity";
    pr.nodeDescriptions[9] = "Flight 3 Capacity";
    pr.nodeDescriptions[10] = "SINK (Completed trips)";

    pr.flightNames[1] = "Flight 1 (9:00 AM)";
    pr.flightNames[2] = "Flight 2 (12:00 PM)";
    pr.flightNames[3] = "Flight 3 (5:00 PM)";

    // Initialize capacities
    pr.remainingSeats[7] = 20;  // Flight 1 capacity
    pr.remainingSeats[8] = 30;  // Flight 2 capacity
    pr.remainingSeats[9] = 25;  // Flight 3 capacity

    pr.flightStatus[1] = pr.flightStatus[2] = pr.flightStatus[3] = true;

    // Passenger sources
    pr.addEdge(source, 4, 15);  // Business class demand
    pr.addEdge(source, 5, 40);  // Economy class demand
    pr.addEdge(source, 6, 10);  // Connecting flight demand

    // Business class to flights
    pr.addEdge(4, 1, 10);  // Business → Flight 1
    pr.addEdge(4, 2, 10);  // Business → Flight 2
    pr.addEdge(4, 3, 10);  // Business → Flight 3

    // Economy class to flights
    pr.addEdge(5, 1, 20);  // Economy → Flight 1
    pr.addEdge(5, 2, 20);  // Economy → Flight 2
    pr.addEdge(5, 3, 20);  // Economy → Flight 3

    // Connecting flights to flights
    pr.addEdge(6, 1, 5);  // Connecting → Flight 1
    pr.addEdge(6, 2, 5);  // Connecting → Flight 2

    // Flight to capacity nodes
    pr.addEdge(1, 7, 20);  // Flight 1 → Capacity
    pr.addEdge(2, 8, 30);  // Flight 2 → Capacity
    pr.addEdge(3, 9, 25);  // Flight 3 → Capacity

    // Capacity to sink
    pr.addEdge(7, sink, 20);
    pr.addEdge(8, sink, 30);
    pr.addEdge(9, sink, 25);

    // Initial allocation
    pr.pushRelabel(source, sink);
    pr.recordPassengerAllocation();
    pr.printGraph();
    pr.printAllocation();

    // Demo cancellation
    pr.cancelFlight(2);
    
    // Adjust capacity
    pr.adjustSeats(3, 30);
    
    // Process waiting list
    pr.processWaitingList(3);
    
    // Recompute allocations
    pr.pushRelabel(source, sink);
    pr.recordPassengerAllocation();
    pr.printAllocation();

    return 0;
}