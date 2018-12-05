int udtcp_get_udp_port(int sock)
{
    //
    // TODO ADD TIMEOUT
    //
    int portUdpFromServer;
    if (read(sock, &portUdpFromServer, sizeof(int)) < 1)
        return -1;
    return portUdpFromServer;
}