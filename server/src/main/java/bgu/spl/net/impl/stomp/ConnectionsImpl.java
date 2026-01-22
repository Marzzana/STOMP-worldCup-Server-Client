package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionHandler;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;


public class ConnectionsImpl<T> implements Connections<T> {
    // ================ Fields ================
    private final ConcurrentHashMap<Integer, ConnectionHandler<T>> handlers;
    private final ConcurrentHashMap<String, ConcurrentHashMap<Integer, Integer>> channelSubscribers; //channel -> (connId -> subId)
    private final ConcurrentHashMap<Integer, ConcurrentHashMap<Integer, String>> connectionSubscriptions; //connId -> (subId -> channel)
    private final AtomicInteger connectionIdCounter;


    // ================ Constructors ================
    public ConnectionsImpl() {
        handlers = new ConcurrentHashMap<>();
        channelSubscribers = new ConcurrentHashMap<>();
        connectionSubscriptions = new ConcurrentHashMap<>();
        connectionIdCounter = new AtomicInteger(1);
    }


    // ================ Interface Methods ================
    @Override
    public boolean send(int connectionId, T msg) {
        if (msg == null)
            throw new IllegalArgumentException("Message is null!");
         
        ConnectionHandler<T> thisHandler = handlers.get(connectionId);
        if (thisHandler == null)
            return false;
        
        thisHandler.send(msg);
        return true;
    }
    
    @Override
    public void send(String channel, T msg) {
        //Send identical msg to all channel subscribers (STOMP wont use)
        if (channel == null)
            throw new IllegalArgumentException("Channel is null!");
        if (msg == null)
            throw new IllegalArgumentException("Message is null!");

        ConcurrentHashMap<Integer, Integer> recipients = channelSubscribers.get(channel);
        if (recipients == null)
            return; //swallowing when messaging to channel with no subscribers 
        
        for (Integer connectionId : recipients.keySet()) {
            send(connectionId, msg);            
        }
    }

    @Override
    public void disconnect(int connectionId) {
        //Remove handler, clean all subscriptions for this connection
        handlers.remove(connectionId);
        
        ConcurrentHashMap<Integer, String> mySubscriptions = connectionSubscriptions.get(connectionId);
        if (mySubscriptions == null) //nothing to unsub from
            return;
        
        for (String channel : mySubscriptions.values()) { //unsub every channel im subbed to
            ConcurrentHashMap<Integer, Integer> currentChannelSubs = channelSubscribers.get(channel);
            if (currentChannelSubs == null) //nothing to unsub from
                continue;
            currentChannelSubs.remove(connectionId); //unsub          
        }
        
        connectionSubscriptions.remove(connectionId);
    }


    // ================ Additional Methods ================
    public int registerHandler(ConnectionHandler<T> handler) {
        //Generating new connectionId, stores handler returns connectionId
        if (handler == null)
            throw new IllegalArgumentException("Handler is null!");
        
        int connectionId = connectionIdCounter.getAndIncrement();
        handlers.put(connectionId, handler);
        connectionSubscriptions.put(connectionId, new ConcurrentHashMap<Integer, String>());
    
        return connectionId;
    }

    public void subscribe(int connectionId, String channel, int subscriptionId) {
        //Adding connectionId to both maps
        if (!handlers.containsKey(connectionId))
            throw new IllegalArgumentException("Connection doesn't exist!");
        if (channel == null)
            throw new IllegalArgumentException("Channel is null!");

        channelSubscribers.computeIfAbsent(channel, ignored -> new ConcurrentHashMap<>()).put(connectionId, subscriptionId);
        connectionSubscriptions.get(connectionId).put(subscriptionId, channel);
    }

    public String unsubscribe(int connectionId, int subscriptionId) {
        //Remove from both maps, returns channel name
        ConcurrentHashMap<Integer, String> mySubscriptions = connectionSubscriptions.get(connectionId);
        if (mySubscriptions == null) //nothing to unsub from
            return null;
        
        String channel = mySubscriptions.remove(subscriptionId); 
        if (channel == null) //not subscribing channel - nothing to unsub from
            return null;

        channelSubscribers.get(channel).remove(connectionId);
        
        return channel;
    }

    public Map<Integer,Integer> getChannelSubscribers(String channel) {
        if (channel == null)
            throw new IllegalArgumentException("Channel is null!");
        
        return channelSubscribers.get(channel);
    }

}
