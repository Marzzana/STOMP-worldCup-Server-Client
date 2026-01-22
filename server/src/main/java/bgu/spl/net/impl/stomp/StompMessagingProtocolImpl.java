package bgu.spl.net.impl.stomp;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.impl.data.Database;
import bgu.spl.net.impl.data.LoginStatus;


public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {
    // ================ Fields ================
    private int connectionId;
    private ConnectionsImpl<String> connections;
    private boolean shouldTerminate;
    private String username;
    private static AtomicInteger messageIdCounter = new AtomicInteger(1);


    // ================ Constructors ================
    public StompMessagingProtocolImpl() {
        shouldTerminate = false;
        username = null;
    }


    // ================ Interface Methods ================
    @Override
    public void start(int connectionId, Connections<String> connections) {
        if (connections == null)
            throw new IllegalArgumentException("Connections is null!");
        
        this.connectionId = connectionId;
        this.connections = (ConnectionsImpl<String>) connections;
    }
    
    @Override
    public void process(String message) {
        if (message == null)
            throw new IllegalArgumentException("Message is null!");
        
        String command = extractCommand(message);
        Map<String,String> headers = extractHeaders(message);
        String body = extractBody(message);

        switch (command) {
            case "CONNECT":
                handleConnect(headers);
                break;
            case "DISCONNECT":
                handleDisconnect(headers);
                break;
            case "SUBSCRIBE":
                handleSubscribe(headers);
                break;
            case "UNSUBSCRIBE":
                handleUnsubscribe(headers);
                break;
            case "SEND":
                handleSend(headers, body);
                break;
            default:
                String receiptId = headers.get("receipt");
                connections.send(connectionId, buildErrorFrame("Unknown command", receiptId));
                shouldTerminate = true;
        }
    }
	
    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }
    

    // ================ Helper Methods ================
    // ========== Frame-Parsing Methods ==========
    //extractCommand
    private String extractCommand(String frame) {
        if (frame == null)
            throw new IllegalArgumentException("Frame is null!");
        
        return frame.substring(0, frame.indexOf('\n'));
    }

    //extractHeaderes
    private Map<String,String> extractHeaders(String frame) {
        if (frame == null)
            throw new IllegalArgumentException("Frame is null!");
        
        Map<String,String> headersAndValues = new HashMap<>();
        
        String[] lines = frame.split("\n");
        int i = 1;
        String currentLine = lines[i];
        while (i < lines.length && !currentLine.isEmpty()) {
            int separatorIndex  = currentLine.indexOf(':');
            
            String header = currentLine.substring(0, separatorIndex );
            String value = currentLine.substring(separatorIndex + 1);

            headersAndValues.put(header, value);
            i++;
            if (i < lines.length) {
                currentLine = lines[i];
            } else {
                break;
            }
            
        }
        return headersAndValues;
    }

    //extractBody
    private String extractBody(String frame) {
        if (frame == null)
            throw new IllegalArgumentException("Frame is null!");

        String[] lines = frame.split("\n");
        int blankLineIndex = 0;
        for (int i = 1 ; i < lines.length ; i++) {
            String currentLine = lines[i];
            if (currentLine.isEmpty()) {
                blankLineIndex = i;
                break;
            }
        }
        String[] lineOfBody = Arrays.copyOfRange(lines, blankLineIndex + 1, lines.length);

        String body = String.join("\n", lineOfBody);
        return body;   
    }


    // ========== Frame-Response Methods ==========
    //buildConnectedFrame
    private String buildConnectedFrame() {
        return "CONNECTED\nversion:1.2\n\n";
    }

    //buildReceiptFrame
    private String buildReceiptFrame(String receiptId) {
        if (receiptId == null)
            throw new IllegalArgumentException("Receipt Id is null!");

        return "RECEIPT\nreceipt-id:" + receiptId + "\n\n";
    }

    //buildErrorFrame
    private String buildErrorFrame(String description, String receiptId) {
        if (description == null)
            throw new IllegalArgumentException("Message is null!");
        //allowing receiptId to be null since not all error messages will have ids

        String errorFrame = "ERROR\n";
        if (receiptId != null) {
            errorFrame += "receipt-id:" + receiptId + "\n";
        }
        errorFrame += "message:" + description + "\n\n";

        return errorFrame;
    }

    //buildMessageFrame
    private String buildMessageFrame(String dest, int subId, int msgId, String body) {
        if (dest == null)
            throw new IllegalArgumentException("Destination is null!");
        if (body == null)
            throw new IllegalArgumentException("Message is null!");

        return "MESSAGE\n" + "subscription:" + subId + "\nmessage-id:" + msgId +
               "\ndestination:" + dest + "\n\n" + body;
    }


    // ========== Handler Methods ==========
    //handleConnect
    private void handleConnect(Map<String,String> headers) {
        if (headers == null)
            throw new IllegalArgumentException("Headers is null!");
        
        String username = headers.get("login");
        String password = headers.get("passcode");
        String receiptId = headers.get("receipt");

        if (username == null || password == null) {
            connections.send(connectionId, buildErrorFrame("Missing login or passcode", receiptId));
            shouldTerminate = true;
            return;
        }
        if (this.username != null) {
            connections.send(connectionId, buildErrorFrame("Client already logged in", receiptId));
            shouldTerminate = true;
            return;
        }

        LoginStatus status = Database.getInstance().login(connectionId, username, password);
        switch (status) {
            case ADDED_NEW_USER:
            case LOGGED_IN_SUCCESSFULLY:
                this.username = username;
                connections.send(connectionId, buildConnectedFrame());
                break;
            case WRONG_PASSWORD:
                connections.send(connectionId, buildErrorFrame("User exists, wrong password",receiptId));
                shouldTerminate = true;
                break;
            case ALREADY_LOGGED_IN:
                connections.send(connectionId, buildErrorFrame("This user is logged in elsewhere", receiptId));
                shouldTerminate = true;
                break;
            case CLIENT_ALREADY_CONNECTED:
                connections.send(connectionId,  buildErrorFrame("This connectionId already logged in", receiptId));
                shouldTerminate = true;
                break;
            default:
                shouldTerminate = true;
                throw new IllegalStateException("Something unexpected happend");
        }
    }

    //handleSubscribe
    private void handleSubscribe(Map<String,String> headers) {
        if (headers == null)
            throw new IllegalArgumentException("Headers is null!");
    
        String destination = headers.get("destination");
        String subscriptionId = headers.get("id");
        String receiptId = headers.get("receipt");

        if (destination == null) {
            connections.send(connectionId, buildErrorFrame("No destination!", receiptId));
            shouldTerminate = true;
            return;
        }
        if (subscriptionId == null) {
            connections.send(connectionId, buildErrorFrame("No Subscription id!", receiptId));
            shouldTerminate = true;
            return;
        }
        if (username == null) {
            connections.send(connectionId, buildErrorFrame("Username isn't logged-in!", receiptId));
            shouldTerminate = true;
            return;
        }

        int subId = Integer.valueOf(subscriptionId);
        connections.subscribe(connectionId, destination, subId);
        if (receiptId != null) {
            connections.send(connectionId, buildReceiptFrame(receiptId));
        }
    }

    //handleUnsubscribe
    private void handleUnsubscribe(Map<String,String> headers) {
        if (headers == null)
            throw new IllegalArgumentException("Headers is null!");
        
        String subscriptionId = headers.get("id");
        String receiptId = headers.get("receipt");
        
        if (username == null) {
            connections.send(connectionId, buildErrorFrame("Username isn't logged-in!", receiptId));
            shouldTerminate = true;
            return;
        }
        if (subscriptionId == null) {
            connections.send(connectionId, buildErrorFrame("No Subscription id!", receiptId));
            shouldTerminate = true;
            return;
        }

        int subId = Integer.valueOf(subscriptionId);
        String channel = connections.unsubscribe(connectionId, subId);
        
        if (channel == null) {
            connections.send(connectionId, buildErrorFrame("Channel doesn't exist!", receiptId));
            shouldTerminate = true;
            return;
        }
        if (receiptId != null) {
            connections.send(connectionId, buildReceiptFrame(receiptId));
        }
    }

    //handleSend
    private void handleSend(Map<String,String> headers, String body) {
        if (headers == null)
            throw new IllegalArgumentException("Headers is null!");
        if (body == null)
            throw new IllegalArgumentException("Message-body is null!");
        
        String destination = headers.get("destination");
        String receiptId = headers.get("receipt");

        if (username == null) {
            connections.send(connectionId, buildErrorFrame("Username isn't logged-in!", receiptId));
            shouldTerminate = true;
            return;
        }
        if (destination == null) {
            connections.send(connectionId, buildErrorFrame("No destination!", receiptId));
            shouldTerminate = true;
            return;
        }

        Map<Integer,Integer> subs = connections.getChannelSubscribers(destination);

        if (subs == null || subs.get(connectionId) == null) { //no subscribers to send to (neither you client is subbed)
            connections.send(connectionId, buildErrorFrame("You arent subbed to channel!", receiptId));
            shouldTerminate = true;
            return; 
        }
        
        for (Integer connId : subs.keySet()) {
            int subId = subs.get(connId);
            int msgId = messageIdCounter.getAndIncrement();
            String message = buildMessageFrame(destination, subId, msgId, body);

            connections.send(connId, message);
        }

        // Extract user and source from body for file tracking
        String reportUser = null;
        String sourceFile = null;
        String[] bodyLines = body.split("\n");
        for (String line : bodyLines) {
            if (line.startsWith("user: ")) {
                reportUser = line.substring(6);
            } else if (line.startsWith("source: ")) {
                sourceFile = line.substring(8);
            }
        }
        if (reportUser != null && sourceFile != null) {
            Database.getInstance().trackFileUpload(reportUser, sourceFile, destination);
        }

        if (receiptId != null) {
            connections.send(connectionId, buildReceiptFrame(receiptId));
        }
    }

    //handleDisconnect
    private void handleDisconnect(Map<String,String> headers) {
        if (headers == null)
            throw new IllegalArgumentException("Headers is null!");

        String receiptId = headers.get("receipt");
        
        if (receiptId != null) {
            connections.send(connectionId, buildReceiptFrame(receiptId));
        }
        
        connections.disconnect(connectionId);
        shouldTerminate = true;
        Database.getInstance().logout(connectionId);
    }
}
