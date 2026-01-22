#pragma once

#include <map>
#include <string>
#include <vector>
#include <utility>
#include "ConnectionHandler.h"
#include "event.h"

class StompProtocol {
    private:
    // ==================== Fields ====================
    bool is_logged_in;
    int subscription_id_counter;
    int receipt_id_counter;
    bool should_terminate;
    std::string username;
    std::map<int, std::string> subId_to_channel;
    std::map<std::string, int> channel_to_subId;
    std::map<std::string, std::map<std::string, std::vector<Event>>> game_reports; //game_name->(username->events_reported)
    std::map<int, std::string> pending_receipts; //receipt->its description
    ConnectionHandler* handler;

    // ==================== Helper Methods ====================
    // ======== Frame-building methods ========
    std::string buildConnectFrame(const std::string& username, const std::string& password);
    std::string buildSubscribeFrame(const std::string& destination, int subscriptionId, int receiptId);
    std::string buildUnsubscribeFrame(int subscriptionId, int receiptId);
    std::string buildSendFrame(const std::string& destination, const std::string& body);
    std::string buildDisconnectFrame(int receiptId);
    
    // ======== Frame-parsing methods ========
    std::string extractCommand(const std::string& frame);
    std::map<std::string, std::string> extractHeaders(const std::string& frame);
    std::string extractBody(const std::string& frame);

    // ======== Server-frame-handler methods ========
    void handleConnectedFrame();
    void handleMessageFrame(const std::map<std::string, std::string>& headers, const std::string& body);
    void handleReceiptFrame(const std::map<std::string, std::string>& headers);
    void handleErrorFrame(const std::map<std::string, std::string>& headers, const std::string& body);

    // ======== Keyboard-command-processors methods ========
    void processLoginCommand(const std::vector<std::string>& args);
    void processJoinCommand(const std::vector<std::string>& args);
    void processExitCommand(const std::vector<std::string>& args);
    void processReportCommand(const std::vector<std::string>& args);
    void processSummaryCommand(const std::vector<std::string>& args);
    void processLogoutCommand();
    
    // ======== Utility methods ========
    int nextSubscriptionId();
    int nextReceiptId();
    std::string formatEventBody(const Event& event, const std::string& filename);
    std::pair<Event, std::string> parseEventFromBody(const std::string& body);


    public:
    // ==================== Constructors ====================
    /*NOTE: we deleted the copy constructor and assignment operator since the client will
            have only 2 threads, one thread that is taking care of keyboard input, and the
            other thread that will take care of server communication, therefore we do not
            allow in our program to copy and assign protocols that are already existing to
            other threads.
            in addition, we are leaning on the default destrcutor, since we do not handle any
            memory creating on the heap, the only pointer of the program is the connection
            handler, that the main takes care of creating and deleting, the protocol does not
            own this pointer, therefore we do not need to implement the destructor ourselves */
    StompProtocol();
    StompProtocol(const StompProtocol&) = delete;
    StompProtocol& operator=(const StompProtocol&) = delete;

    // ==================== Public Methods ====================
    bool shouldTerminate();
    void setHandler(ConnectionHandler* handler);
    ConnectionHandler* getHandler();
    void processKeyboardCommand(const std::string& command);
    void processServerFrame(const std::string& frame);
    
};

