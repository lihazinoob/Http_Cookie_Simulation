package simple_dns_http;

channel P2P extends ned.DatarateChannel
{
    datarate = 100Mbps;
    delay = 0.2ms;
}

simple Router
{
    parameters:
        string routes = default("");   // e.g. "1:0,2:1,3:2"
        @display("i=device/router");
    gates:
        inout pppg[4];                 // <-- expanded to 4 gates (0..3)
}

simple PC
{
    parameters:
        int address;
        int dnsAddr = default(2);
        string dnsQuery = default("example.com");
        double startAt @unit(s) = default(0.5s);
        @display("i=device/laptop");
    gates:
        inout ppp;
}

simple DNS
{
    parameters:
        int address;
        int answerAddr = default(3);
        @display("i=device/server");
    gates:
        inout ppp;
}

simple HTTP
{
    parameters:
        int address;
        double serviceTime @unit(s) = default(5ms);
        int pageSizeBytes = default(20000);
        @display("i=device/server");
    gates:
        inout ppp;
}

network SimpleNet
{
    submodules:
        r: Router {
            parameters:
                routes = "1:0,4:1,2:2,3:3";
                @display("p=420,200");
        }
        pc: PC {
            parameters:
                address = 1;
                dnsAddr = 2;
                dnsQuery = "example.com";
                startAt = 0.5s;
                @display("p=120,120");
        }
        pc2: PC {
            parameters:
                address = 4;
                dnsAddr = 2;
                dnsQuery = "example.com";
                startAt = 1.0s;
                @display("p=120,280");
        }
        dns: DNS {
            parameters:
                address = 2;
                answerAddr = 3;
            @display("p=720,120");
        }
        http: HTTP {
            parameters:
                address = 3;
                serviceTime = 5ms;
                pageSizeBytes = 20000;
            @display("p=720,280");
        }
    connections:
        pc.ppp   <--> P2P <--> r.pppg[0];
        pc2.ppp  <--> P2P <--> r.pppg[1];
        dns.ppp  <--> P2P <--> r.pppg[2];
        http.ppp <--> P2P <--> r.pppg[3];
}

