#include <sstream>
#include <fstream>
#include <iostream>
#include "StompProtocol.h"

// -------------------- Constructor --------------------
StompProtocol::StompProtocol() : is_logged_in(false), subscription_id_counter(1),
                                 receipt_id_counter(1), should_terminate(false),
                                 username(), subId_to_channel(), channel_to_subId(),
                                 game_reports(), pending_receipts(), handler(nullptr) {}



// -------------------- Public Methods --------------------
//shouldTerminate()
bool StompProtocol::shouldTerminate() {
    return should_terminate;
}

//setHandler()
void StompProtocol::setHandler(ConnectionHandler* handler_ptr) {
    handler = handler_ptr;
}
    
//getHandler()
ConnectionHandler* StompProtocol::getHandler() {
    return handler;
}

//processKeyboardCommand()
void StompProtocol::processKeyboardCommand(const std::string& input) {
    std::istringstream stream(input);
    std::string command;
    stream >> command; //extracting first word in input
    
    std::vector<std::string> args;
    std::string arg;
    while (stream >> arg) { //while there arguments left in input, insert the to args vector 
        args.push_back(arg);
    }

    if (command == "login") {
        processLoginCommand(args);
    } else if (command == "join") {
        processJoinCommand(args);
    } else if (command == "exit") {
        processExitCommand(args);
    } else if (command == "report") {
        processReportCommand(args);
    } else if (command == "summary") {
        processSummaryCommand(args);
    } else if (command == "logout") {
        processLogoutCommand();
    }
}

//processServerFrame()
void StompProtocol::processServerFrame(const std::string& frame) {
    std::string command = extractCommand(frame);
    std::map<std::string, std::string> headers = extractHeaders(frame);
    std::string body = extractBody(frame);

    if (command == "CONNECTED") {
        handleConnectedFrame();
    } else if (command == "MESSAGE") {
        handleMessageFrame(headers, body);
    } else if (command == "RECEIPT") {
        handleReceiptFrame(headers);
    } else if (command == "ERROR") {
        handleErrorFrame(headers, body);
    }
}



// -------------------- Private Helper Methods --------------------
// -------- Frame-building methods --------
//buildConnectFrame()
std::string StompProtocol::buildConnectFrame(const std::string& username, const std::string& password) {
    return "CONNECT\naccept-version:1.2\nhost:stomp.cs.bgu.ac.il\nlogin:" + username +
    "\npasscode:" + password + "\n\n";
}

//buildSubscribeFrame()
std::string StompProtocol::buildSubscribeFrame(const std::string& destination, int subscriptionId, int receiptId) {
    return "SUBSCRIBE\ndestination:" + destination + "\nid:" + 
    std::to_string(subscriptionId) + "\nreceipt:" + std::to_string(receiptId) + "\n\n";
}

//buildUnsubscribeFrame()
std::string StompProtocol::buildUnsubscribeFrame(int subscriptionId, int receiptId) {
    return "UNSUBSCRIBE\nid:" + std::to_string(subscriptionId) + "\nreceipt:" +
    std::to_string(receiptId) + "\n\n";
}

//buildSendFrame()
std::string StompProtocol::buildSendFrame(const std::string& destination, const std::string& body) {
    return "SEND\ndestination:" + destination + "\n\n" + body;
}

//buildDisconnectFrame()
std::string StompProtocol::buildDisconnectFrame(int receiptId) {
    return "DISCONNECT\nreceipt:" + std::to_string(receiptId) + "\n\n";
}



// -------- Frame-parsing methods --------
//extractCommand()
std::string StompProtocol::extractCommand(const std::string& frame) {
    return frame.substr(0, frame.find('\n'));
}

//extractHeaders()
std::map<std::string, std::string> StompProtocol::extractHeaders(const std::string& frame) {
    std::map<std::string, std::string> headers_and_values;
    std::istringstream stream(frame);
    std::string line;

    std::getline(stream, line); //skipping command line
    while (std::getline(stream, line) && !line.empty()) { //read frame-stream until empty line
        size_t separator_index = line.find(':');
        if (separator_index == std::string::npos)
            continue;

        std::string header = line.substr(0, separator_index);
        std::string value = line.substr(separator_index + 1);
        headers_and_values[header] = value;
    }
    return headers_and_values;
}

//extractBody()
std::string StompProtocol::extractBody(const std::string& frame) {
    std::istringstream stream(frame);
    std::string line;

    while (std::getline(stream, line) && !line.empty()) {}
    std::ostringstream out;
    out << stream.rdbuf();

    return out.str();
}



// -------- Server-frame-handler methods --------
//handleConnectedFrame()
void StompProtocol::handleConnectedFrame() {
    is_logged_in = true;
    std::cout << "Login successful" << std::endl;
}

//handleMessageFrame()
void StompProtocol::handleMessageFrame(const std::map<std::string, std::string>& headers, const std::string& body) {
    std::string game_name = headers.at("destination");
    std::pair<Event, std::string> body_and_user = parseEventFromBody(body);
    std::string sender = body_and_user.second;
    game_reports[game_name][sender].push_back(body_and_user.first);
}

//handleReceiptFrame()
void StompProtocol::handleReceiptFrame(const std::map<std::string, std::string>& headers) {
    std::string receipt_id = headers.at("receipt-id");
    int rec_id = std::stoi(receipt_id);
    
    std::string to_print = pending_receipts.at(rec_id);
    if (to_print == "logout") {
        should_terminate = true;
    }
    
    std::cout << to_print << std::endl;
    pending_receipts.erase(rec_id);
}

//handleErrorFrame()
void StompProtocol::handleErrorFrame(const std::map<std::string, std::string>& headers, const std::string& body) {
    std::string description = headers.at("message");

    std::cout << description << std::endl;
    if (!body.empty()) {
        std::cout << body << std::endl;
    }

    should_terminate = true;
}



// -------- Keyboard-command-processors methods --------
//processLoginCommand()
void StompProtocol::processLoginCommand(const std::vector<std::string>& args) {
    if (is_logged_in) {
        std::cout << "The client is already logged in, log out before trying again" << std::endl;
        return;
    }

    std::string username = args.at(1); //skipping 0 since main is passing with port
    std::string password = args.at(2);

    this->username = username;

    std::string frame_to_send = buildConnectFrame(username, password);
    handler->sendFrameAscii(frame_to_send, '\0');
}

//processJoinCommand()
void StompProtocol::processJoinCommand(const std::vector<std::string>& args) {
    std::string game_name = args.at(0);
    int sub_id = nextSubscriptionId();
    int receipt_id = nextReceiptId();

    std::string frame_to_send = buildSubscribeFrame(game_name, sub_id, receipt_id);
    handler->sendFrameAscii(frame_to_send, '\0');

    subId_to_channel[sub_id] = game_name;
    channel_to_subId[game_name] = sub_id;
    pending_receipts[receipt_id] = "Joined channel " + game_name;
}

//processExitCommand()
void StompProtocol::processExitCommand(const std::vector<std::string>& args) {
    std::string game_name = args.at(0);
    int sub_id = channel_to_subId[game_name];
    int receipt_id = nextReceiptId();

    std::string frame_to_send = buildUnsubscribeFrame(sub_id, receipt_id);
    handler->sendFrameAscii(frame_to_send, '\0');
    
    subId_to_channel.erase(sub_id);
    channel_to_subId.erase(game_name);
    pending_receipts[receipt_id] = "Exited channel " + game_name;
}

//processReportCommand()
void StompProtocol::processReportCommand(const std::vector<std::string>& args) {
    struct names_and_events names_and_events = parseEventsFile(args[0]);
    std::string team_a = names_and_events.team_a_name;
    std::string team_b = names_and_events.team_b_name;
    std::vector<Event> events = names_and_events.events;

    std::string game_name = team_a + "_" + team_b;
    for (size_t i = 0 ; i < events.size() ; i++) {
        std::string body = formatEventBody(events[i], args[0]);

        std::string frame_to_send = buildSendFrame(game_name, body);
        handler->sendFrameAscii(frame_to_send, '\0');
    }
}

//processSummaryCommand
void StompProtocol::processSummaryCommand(const std::vector<std::string>& args) {
    std::string game_name = args[0];
    std::string user = args[1];
    std::string file_path = args[2];

    //check if data exists
    if (game_reports.find(game_name) == game_reports.end() ||
        game_reports[game_name].find(user) == game_reports[game_name].end()) {
        std::cout << "No reports found" << std::endl;
        return;
    }

    std::vector<Event>& events = game_reports[game_name][user];
    if (events.empty()) return;

    //get team names from first event
    std::string team_a = events[0].get_team_a_name();
    std::string team_b = events[0].get_team_b_name();

    //aggregate stats (std::map keeps lexicographic order)
    std::map<std::string, std::string> general_stats;
    std::map<std::string, std::string> team_a_stats;
    std::map<std::string, std::string> team_b_stats;

    for (const Event& event : events) {
        for (const auto& pair : event.get_game_updates()) {
            general_stats[pair.first] = pair.second;
        }
        for (const auto& pair : event.get_team_a_updates()) {
            team_a_stats[pair.first] = pair.second;
        }
        for (const auto& pair : event.get_team_b_updates()) {
            team_b_stats[pair.first] = pair.second;
        }
    }

    //write to file
    std::ofstream out_file(file_path);
    
    out_file << team_a << " vs " << team_b << "\n";
    out_file << "Game stats:\n";
    
    out_file << "General stats:\n";
    for (const auto& pair : general_stats) {
        out_file << pair.first << ": " << pair.second << "\n";
    }

    out_file << team_a << " stats:\n";
    for (const auto& pair : team_a_stats) {
        out_file << pair.first << ": " << pair.second << "\n";
    }

    out_file << team_b << " stats:\n";
    for (const auto& pair : team_b_stats) {
        out_file << pair.first << ": " << pair.second << "\n";
    }
    
    out_file << "Game event reports:\n";
    for (const Event& event : events) {
        out_file << event.get_time() << " - " << event.get_name() << ":\n";
        out_file << event.get_discription() << "\n";
    }

    out_file.close();
}
    
//processLogoutCommand()
void StompProtocol::processLogoutCommand() {
    int receipt_id = nextReceiptId();

    std::string frame_to_send = buildDisconnectFrame(receipt_id);
    handler->sendFrameAscii(frame_to_send, '\0');

    pending_receipts[receipt_id] = "logout";
}



// -------- Utility methods --------
//nextSubscriptionId()
int StompProtocol::nextSubscriptionId() {
    return subscription_id_counter++;
}

//nextReceiptId()
int StompProtocol::nextReceiptId() {
    return receipt_id_counter++;
}

//formatEventBody()
std::string StompProtocol::formatEventBody(const Event& event, const std::string& filename) {
    std::string user = username;
    std::string team_a = event.get_team_a_name();
    std::string team_b = event.get_team_b_name();
    std::string event_name = event.get_name();
    int time = event.get_time();
    const auto& general_updates = event.get_game_updates();
    const auto& team_a_updates = event.get_team_a_updates();
    const auto& team_b_updates = event.get_team_b_updates();
    std::string description = event.get_discription();

    std::string body = "user: " + user + "\nsource: " + filename + "\nteam a: " + team_a +
        "\nteam b: " + team_b + "\nevent name: " + event_name + "\ntime: " +
        std::to_string(time) + "\ngeneral game updates:\n";

    for (const auto& pair : general_updates) {
        body += pair.first + ": " + pair.second + "\n";
    }
    body += "team a updates:\n";
    for (const auto& pair : team_a_updates) {
        body += pair.first + ": " + pair.second + "\n";
    }    
    body += "team b updates:\n"; 
    for (const auto& pair : team_b_updates) {
        body += pair.first + ": " + pair.second + "\n";
    } 
    body += "description:\n" + description;

    return body;
}

//parseEventFromBody()
std::pair<Event, std::string> StompProtocol::parseEventFromBody(const std::string& body) {
    std::istringstream stream(body);
    std::string line;

    std::getline(stream, line); //reading username
    std::string username = line.substr(6); //6=chars until username

    std::getline(stream, line); //skipping source line

    std::getline(stream, line); //reading team_a name
    std::string team_a = line.substr(8); //8=chars until team_a name
    std::getline(stream, line); //reading team_b name
    std::string team_b = line.substr(8);

    std::getline(stream, line); //reading event name
    std::string event_name = line.substr(12);
    
    std::getline(stream, line); //reading time
    std::string time_str = line.substr(6);
    int time = std::stoi(time_str);

    //reading general_updates
    std::getline(stream, line); 
    std::map<std::string, std::string> general_updates;
    while (std::getline(stream, line) && (line != "team a updates:")) {
        size_t separator_index = line.find(':');
        if (separator_index == std::string::npos)
            continue;
        general_updates[line.substr(0, separator_index)] = line.substr(separator_index + 2);
    }
    //reading team_a_updates
    std::map<std::string, std::string> team_a_updates;
    while (std::getline(stream, line) && (line != "team b updates:")) {
        size_t separator_index = line.find(':');
        if (separator_index == std::string::npos)
            continue;
        team_a_updates[line.substr(0, separator_index)] = line.substr(separator_index + 2);
    }
    //reading team_b_updates
    std::map<std::string, std::string> team_b_updates;
    while (std::getline(stream, line) && (line != "description:")) {
        size_t separator_index = line.find(':');
        if (separator_index == std::string::npos)
            continue;
        team_b_updates[line.substr(0, separator_index)] = line.substr(separator_index + 2);
    }
    //reading description
    std::ostringstream out;
    out << stream.rdbuf(); //out now contains description
    
    return std::make_pair(Event(team_a, team_b, event_name, time, general_updates,
        team_a_updates, team_b_updates, out.str()), username);
}
