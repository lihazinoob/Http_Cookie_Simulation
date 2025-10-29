#include <omnetpp.h>
#include "helpers.h"
#include <map>
#include <sstream>
using namespace omnetpp;
using namespace std;

class PC : public cSimpleModule {
  private:
    int addr = 0;
    int dnsAddr = 2;
    string qname;
    cMessage *startEvt = nullptr;
    cMessage *secondRequestEvt = nullptr;
    
    string sessionCookie = ""; // Store the cookie received from server
    long httpServerAddr = -1;
    int requestCount = 0;
    int successfulResponsesWithCookie = 0; // Track responses with valid cookie

  protected:
    void initialize() override {
        addr    = par("address");
        dnsAddr = par("dnsAddr");
        qname   = par("dnsQuery").stdstringValue(); // module param → stdstringValue() OK
        startEvt = new cMessage("start");
        scheduleAt(simTime() + SimTime(par("startAt").doubleValue()), startEvt);
        EV_INFO << "PC" << addr << " will start at " << par("startAt").doubleValue() << "s\n";
    }

    void handleMessage(cMessage *msg) override {
        if (msg->isSelfMessage()) {
            if (strcmp(msg->getName(), "start") == 0) {
                // Step 1: ask DNS
                auto *q = mk("DNS_QUERY", DNS_QUERY, addr, dnsAddr);
                q->addPar("qname").setStringValue(qname.c_str());
                send(q, "ppp$o");
                delete msg;
                return;
            } else if (strcmp(msg->getName(), "secondRequest") == 0) {
                // Send second request with cookie
                EV_INFO << "PC" << addr << " sending HTTP request with cookie: " 
                        << sessionCookie << "\n";
                auto *get = mk("HTTP_GET", HTTP_GET, addr, httpServerAddr);
                get->addPar("path").setStringValue("/");
                if (!sessionCookie.empty()) {
                    get->addPar("cookie").setStringValue(sessionCookie.c_str());
                }
                requestCount++;
                send(get, "ppp$o");
                delete msg;
                return;
            }
        }

        switch (msg->getKind()) {
            case DNS_RESPONSE: {
                httpServerAddr = msg->par("answer").longValue();
                EV_INFO << "PC" << addr << " DNS: "
                        << msg->par("qname").stringValue()  // message param → stringValue()
                        << " -> " << httpServerAddr << "\n";
                // Step 2: HTTP GET to server (first time, no cookie)
                EV_INFO << "PC" << addr << " sending first HTTP request (no cookie)\n";
                auto *get = mk("HTTP_GET", HTTP_GET, addr, httpServerAddr);
                get->addPar("path").setStringValue("/");
                requestCount++;
                send(get, "ppp$o");
                break;
            }
            case HTTP_RESPONSE_SET_COOKIE: {
                // Server sent a Set-Cookie response
                sessionCookie = msg->par("sessionId").stringValue();
                EV_INFO << "PC" << addr << " received Set-Cookie response with sessionId: " 
                        << sessionCookie << " (bytes: " << msg->par("bytes").longValue() << ")\n";
                EV_INFO << "PC" << addr << " stored cookie for future requests\n";
                
                // Schedule a second request after 2 seconds to demonstrate cookie usage
                secondRequestEvt = new cMessage("secondRequest");
                scheduleAt(simTime() + 2.0, secondRequestEvt);
                break;
            }
            case HTTP_RESPONSE: {
                bool sessionAware = msg->hasPar("sessionAware") && msg->par("sessionAware").boolValue();
                EV_INFO << "PC" << addr << " got HTTP response "
                        << msg->par("bytes").longValue() << " bytes";
                if (sessionAware) {
                    successfulResponsesWithCookie++;
                    EV_INFO << " (session-aware/personalized response #" << successfulResponsesWithCookie << ")\n";
                    
                    // Check if we've received 2 successful responses with cookie
                    if (successfulResponsesWithCookie >= 2) {
                        EV_INFO << "PC" << addr << " discarding cookie after " 
                                << successfulResponsesWithCookie << " successful responses\n";
                        sessionCookie = ""; // Discard the cookie
                        successfulResponsesWithCookie = 0; // Reset counter
                        
                        // Schedule a third request without cookie (after 2 seconds)
                        secondRequestEvt = new cMessage("secondRequest");
                        scheduleAt(simTime() + 2.0, secondRequestEvt);
                        EV_INFO << "PC" << addr << " will request a new session cookie\n";
                    } else {
                        // Schedule another request with the cookie (after 2 seconds)
                        secondRequestEvt = new cMessage("secondRequest");
                        scheduleAt(simTime() + 2.0, secondRequestEvt);
                    }
                } else {
                    EV_INFO << "\n";
                }
                break;
            }
            default:
                EV_WARN << "PC" << addr << " unexpected kind=" << msg->getKind() << "\n";
        }
        delete msg;
    }

    void finish() override {
        cancelAndDelete(startEvt);
        startEvt = nullptr;
        if (secondRequestEvt != nullptr && secondRequestEvt->isScheduled()) {
            cancelAndDelete(secondRequestEvt);
            secondRequestEvt = nullptr;
        }
        EV_INFO << "PC" << addr << " finished. Total requests sent: " << requestCount + 1 
                << ", Cookie stored: " << (sessionCookie.empty() ? "No (discarded)" : sessionCookie) << "\n";
    }
};
Define_Module(PC);

