observer TokenMonitor {
  TIME TokenPassingTimeout;
  states {Resume, Verify, Duplicate, Lost};
  void setup () {
    TokenPassingTimeout = TTRT + TTRT;
  };
  perform;
};

observer FairnessMonitor {
  TIME MaxDelay;
  void setup (TIME md) { MaxDelay = md; };
  states {Resume, CheckDelay};
  perform;
};
