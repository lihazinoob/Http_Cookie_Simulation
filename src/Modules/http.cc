#include <omnetpp.h>
#include "helpers.h"
#include <map>
#include <sstream>
using namespace omnetpp;
using namespace std;

class HTTP : public cSimpleModule {
  private:
    int addr = 0;
    map<long, string> sessions; // clientAddr -> sessionId
    int sessionCounter = 0;
    
    simsignal_t firstTimeRequestSignal;
    simsignal_t returningRequestSignal;
    
  protected:
    void initialize() override { 
        addr = par("address"); 
        firstTimeRequestSignal = registerSignal("firstTimeRequest");
        returningRequestSignal = registerSignal("returningRequest");
    }

    void handleMessage(cMessage *msg) override {
        if (msg->getKind() == HTTP_GET) {
            long clientAddr = SRC(msg);
            string cookie = "";
            
            // Check if request has a cookie
            if (msg->hasPar("cookie")) {
                cookie = msg->par("cookie").stringValue();
            }
            
            bool isFirstTime = cookie.empty();
            
            if (isFirstTime) {
                // First-time request: no cookie
                EV_INFO << "HTTP Server: First-time request from client " << clientAddr << "\n";
                emit(firstTimeRequestSignal, 1);
                
                // Generate a new session ID
                stringstream ss;
                ss << "SESSION_" << clientAddr << "_" << (++sessionCounter);
                string sessionId = ss.str();
                sessions[clientAddr] = sessionId;
                
                // Send HTTP_RESPONSE_SET_COOKIE
                auto *resp = mk("HTTP_RESPONSE_SET_COOKIE", HTTP_RESPONSE_SET_COOKIE, addr, clientAddr);
                resp->addPar("bytes").setLongValue(par("pageSizeBytes").intValue());
                resp->addPar("sessionId").setStringValue(sessionId.c_str());
                
                EV_INFO << "HTTP Server: Setting cookie for client " << clientAddr 
                        << " with sessionId: " << sessionId << "\n";
                
                sendDelayed(resp, SimTime(par("serviceTime").doubleValue()), "ppp$o");
            } else {
                // Returning request: has cookie
                EV_INFO << "HTTP Server: Returning request from client " << clientAddr 
                        << " with cookie: " << cookie << "\n";
                emit(returningRequestSignal, 1);
                
                // Validate the session
                auto it = sessions.find(clientAddr);
                if (it != sessions.end() && it->second == cookie) {
                    EV_INFO << "HTTP Server: Session validated successfully for client " << clientAddr << "\n";
                    
                    // Send personalized/session-aware response
                    auto *resp = mk("HTTP_RESPONSE", HTTP_RESPONSE, addr, clientAddr);
                    resp->addPar("bytes").setLongValue(par("pageSizeBytes").intValue());
                    resp->addPar("sessionAware").setBoolValue(true);
                    
                    sendDelayed(resp, SimTime(par("serviceTime").doubleValue()), "ppp$o");
                } else {
                    EV_WARN << "HTTP Server: Invalid session for client " << clientAddr << "\n";
                }
            }
        } else {
            EV_WARN << "HTTP unexpected kind=" << msg->getKind() << "\n";
        }
        delete msg;
    }
    
    void finish() override {
        EV_INFO << "HTTP Server: Total unique sessions created: " << sessions.size() << "\n";
    }
};
Define_Module(HTTP);
