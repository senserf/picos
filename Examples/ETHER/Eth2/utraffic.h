station ClientInterface virtual {
  Packet Buffer;
  Boolean ready (long, long, long);
  void configure ();
};

void initTraffic ();
