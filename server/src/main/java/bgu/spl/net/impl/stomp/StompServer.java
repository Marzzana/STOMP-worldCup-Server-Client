package bgu.spl.net.impl.stomp;

import bgu.spl.net.impl.data.Database;
import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        int port = Integer.valueOf(args[0]);
        String serverType = args[1];
        ConnectionsImpl<String> connections = new ConnectionsImpl<>();

        Runtime.getRuntime().addShutdownHook(
            new Thread(() -> { Database.getInstance().printReport(); }));

        if (serverType.equals("tpc")) {
            Server.threadPerClient(port,
                StompMessagingProtocolImpl::new,
                StompEncoderDecoderImpl::new,
                connections).serve();
        } else if (serverType.equals("reactor")) {
            Server.reactor(
            Runtime.getRuntime().availableProcessors(),
            port,
            StompMessagingProtocolImpl::new,
            StompEncoderDecoderImpl::new,
            connections).serve();
        }
    }
}
