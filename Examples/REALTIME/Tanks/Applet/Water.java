import java.io.*;
import java.lang.*;
import java.net.*;
import java.awt.*;
import java.util.*;
import java.applet.Applet;
import java.awt.event.*;
class CommandBuffer {
  // No single item arriving from SIDE will be longer than this
  final static int IMAXUPDATESIZE = 4096;
  final static int CMDHDRL = 3; // Block/command header length
  final static int CMD_UPDATE = 0;
  final static int CMD_ALERT = 1;
  final static int CMD_GETMAP = 2;
  final static int CMD_GETGRAPH = 3;
  final static int CMD_GETSTAT = 4;
  final static int CMD_PERIODIC = 5;
  final static int CMD_UNPERIODIC = 6;
  final static int CMD_OVERRIDE = 7;
  final static int CMD_DISCONNECT = 8;
  final static int CMD_START = 254;
  final static int CMD_STOP = 255;
  byte BlockBuffer [];
  InputStream IS;
  boolean ioerror;
  int Command, Offset, Limit;
  public void dump () {
    // For debugging only
    int i;
    char c1, c2;
    System.out.println ("+++Block read: "+Limit);
    for (i = 0; i < Limit; i++) {
      c1 = (char) ((BlockBuffer [i] >> 4) & 0xf);
      c2 = (char) (BlockBuffer [i] & 0xf);
      if (c1 > 9) c1 += 'a' - 10; else c1 += '0';
      if (c2 > 9) c2 += 'a' - 10; else c2 += '0';
      System.out.print (c1);
      System.out.print (c2);
      System.out.print (" (");
      if ((c1 = (char)(BlockBuffer [i])) < 0x20 || c1 > 0x7f ) c1 = ' ';
      System.out.print (c1);
      System.out.print (") ");
      if (i != 0 && (i % 10 == 0) && i < Limit-1) System.out.print ('\n');
    }
    System.out.print ('\n');
  }
  CommandBuffer (InputStream s) {
    IS = s;
    BlockBuffer = new byte [IMAXUPDATESIZE];
    ioerror = false;
  }
  void readChunk (int off, int len) {
    // Reads a chunk of data from the socket. Its length is guaranteed to be
    // len.
    while (len > 0) {
      try {
        int nr = IS . read (BlockBuffer, off, len);
        if (nr == 0) { ioerror = true; return; }
        len -= nr;
        off += nr;
      } catch (IOException e) { ioerror = true; }
    }
  }
  public int readBlock () {
    // Read a block of incoming data from the socket
    readChunk (0, CMDHDRL); // Command header
    if (ioerror) {
      ioerror = false;
      return -1;
    }
    // Determine the length of the remaining portion
    int cmdl = ((((int) (BlockBuffer [1])) & 0xff) << 8) +
                (((int) (BlockBuffer [2])) & 0xff) ;
    if (cmdl > BlockBuffer.length - CMDHDRL) {
      // Need to resize the buffer
      byte tmp [] = BlockBuffer;
      int nl = BlockBuffer.length + BlockBuffer.length;
      while (nl - CMDHDRL < cmdl) nl += nl;
      BlockBuffer = new byte [nl];
      for (nl = 0; nl < CMDHDRL; nl++) BlockBuffer [nl] = tmp [nl];
      tmp = null;
    }
    // And read the remaining portion
    readChunk (CMDHDRL, cmdl);
    if (ioerror) {
      ioerror = false;
      return -1;
    }
    Command = (int) BlockBuffer [0];
    Offset = CMDHDRL;
    Limit = ((((int) (BlockBuffer [1])) & 0xff) << 8) +
             (((int) (BlockBuffer [2])) & 0xff) + CMDHDRL;
    if (Command == CMD_DISCONNECT)
      return -1;
    else
      return cmdl + CMDHDRL; // Return the total length
  }
  public int getShort () {
    int res;
    res = (((int) BlockBuffer [Offset]) << 8) +
          (((int) BlockBuffer [Offset+1]) & 0xff);
    Offset += 2;
    return res;
  }
  public int getLong () {
    int res;
    res = ((((int) BlockBuffer [Offset ]) & 0xff) << 24) +
          ((((int) BlockBuffer [Offset+1]) & 0xff) << 16) +
          ((((int) BlockBuffer [Offset+2]) & 0xff) << 8) +
           (((int) BlockBuffer [Offset+3]) & 0xff);
    Offset += 4;
    return res;
  }
  public int getByte () {
    return ((int) BlockBuffer [Offset++]) & 0xff;
  }
  public String getString (int limit) {
    String s;
    s = new String (BlockBuffer, Offset, limit-Offset);
    Offset = limit;
    return s;
  }
  public int more () {
    return (Limit > Offset) ? Limit - Offset : 0;
  }
  public String getString () { return getString (Limit); }
}
class TankPanel extends Frame {
  final static int TOTALWIDTH = 600;
  final static int TOTALHEIGHT = 400;
  final static double TANKWIDTH = 0.3;
  final static double TANKHEIGHT = 0.6;
  final static double PIPEHEIGHT = 0.06;
  final static double PUMPWIDTH = 0.3;
  final static double PUMPHEIGHT = 0.06;
  final static Color BGRCOLOR = new Color (238,233,191);
  final static Color PIPECOLOR = new Color (122,139,139);
  final static int MAINLOOPDELAY = 200; // Main loop delay
  final static int BLINKTIME = 400; // Blink delay
  final static int REQUESTBUFFERSIZE = 16; // Requests tend to be short
  Image OffScreenImage;
  Dimension OffScreenSize;
  Graphics OffScreenGraphics;
  FontMetrics CurrentFontMetrics;
  Socket Sock = null;
  Tank Tanks [] = null, PickTank = null;
  Pump Pumps [] = null;
  int NTanks = 0, NPumps = 0, PickTankNumber;
  boolean NeedRedo = false, TooSmall = false;
  boolean BlinkON = false;
  long LastBlinkTime = 0;
  Rectangle Pipe;
  OutputStream OS;
  byte RequestBuffer [] = new byte [REQUESTBUFFERSIZE];
  int RBOffset;
  CommandBuffer CB;
  Water OP;
  TankPanel (Water op) {
    InputStream ss;
    OP = op;
    ss = openSocket ();
    if (ss == null) {
      // Connect failed
      try {
        try {
          Thread.sleep (2000);
        } catch (InterruptedException e) { }
      } catch (ThreadDeath e) { }
      CB = null;
      (this).setVisible (false);
      return;
    } else {
      CB = new CommandBuffer (ss);
      doGetParams ();
    }
    Pipe = new Rectangle (0, 0, 1, 1); // These will be reset
    OffScreenImage = null;
    addMouseListener (new LocalMouseAdapter ());
    addMouseMotionListener (new LocalMouseMotionAdapter ());
    addWindowListener (new LocalWindowAdapter ());
    (this).setSize (TOTALWIDTH, TOTALHEIGHT);
    this.show ();
  }
  void outByte (int b) {
    RequestBuffer [RBOffset++] = (byte) b;
  }
  void outShort (int s) {
    RequestBuffer [RBOffset++] = (byte)((s >> 8) & 0xff);
    RequestBuffer [RBOffset++] = (byte)( s & 0xff);
  }
  void outLong (int s) {
    RequestBuffer [RBOffset++] = (byte)((s >> 24) & 0xff);
    RequestBuffer [RBOffset++] = (byte)((s >> 16) & 0xff);
    RequestBuffer [RBOffset++] = (byte)((s >> 8) & 0xff);
    RequestBuffer [RBOffset++] = (byte)( s & 0xff);
  }
  public void dump (int Limit) {
    // For debugging only
    int i;
    char c1, c2;
    System.out.println ("+++Block to write: "+Limit);
    for (i = 0; i < Limit; i++) {
      c1 = (char) ((RequestBuffer [i] >> 4) & 0xf);
      c2 = (char) (RequestBuffer [i] & 0xf);
      if (c1 > 9) c1 += 'a' - 10; else c1 += '0';
      if (c2 > 9) c2 += 'a' - 10; else c2 += '0';
      System.out.print (c1);
      System.out.print (c2);
      System.out.print (" (");
      if ((c1 = (char)(RequestBuffer [i])) < 0x20 || c1 > 0x7f ) c1 = ' ';
      System.out.print (c1);
      System.out.print (") ");
      if (i != 0 && (i % 10 == 0) && i < Limit-1) System.out.print ('\n');
    }
    System.out.print ('\n');
  }
  boolean sendRequest () {
    int length;
    length = ((((int) (RequestBuffer [1])) & 0xff) << 8) +
              (((int) (RequestBuffer [2])) & 0xff);
    // dump (length+3);  // +++
    try {
      OS.write (RequestBuffer, 0, length+3);
      return true;
    } catch (IOException e) {
      return false;
    }
  }
  class LocalWindowAdapter extends WindowAdapter {
    public void windowClosing (WindowEvent ev) {
      // To intercept destroy event
      OP.Disconnect = true;
    }
  }
  InputStream openSocket () {
    // Open a socket to the simulator
    String hoststr, portstr;
    InputStream is;
    hoststr = OP.getParameter("host");
    portstr = OP.getParameter("port");
    if (hoststr == null) hoststr = "legal.cs.ualberta.ca";
    if (portstr == null) portstr = "3345";
    int port = Integer.parseInt(portstr);
    is = null;
    try {
      try {
        try {
          if ((Sock = new Socket (hoststr, port)) == null) {
            System.out.println ("Failed to connect to host "+hoststr);
            return null;
          }
        } catch (UnknownHostException e) {
          System.out.println ("Host " + hoststr + " not found");
          return null;
        }
      } catch (IOException e) {
        System.out.println ("Simulator doesn't respond on "+hoststr+" " +port);
        return null;
      }
    } catch (Exception e) {
      System.out.println ("Security violation on socket for host "+hoststr);
      return null;
    }
    try {
      is = Sock.getInputStream ();
    } catch (IOException e) {
      System.out.println ("Cannot get socket's input stream");
    }
    try {
      OS = Sock.getOutputStream ();
    } catch (IOException e) {
      System.out.println ("Cannot get socket's output stream");
    }
    return is;
  }
  void doGetParams () {
    RBOffset = 0;
    outByte (CommandBuffer.CMD_GETMAP);
    outShort (0);
    if (!sendRequest ()) {
      System.out.println ("SIDE disconnected");
      OP.DisplayThread = null;
    }
  }
  void doWaterRequest (int tank, int level) {
    RBOffset = 0;
    outByte (CommandBuffer.CMD_UPDATE);
    outShort (3);
    outByte ((byte) tank);
    outShort ((short) level);
    if (!sendRequest ()) {
      System.out.println ("SIDE disconnected");
      OP.DisplayThread = null;
    }
  }
  synchronized void setParams () {
    int i, nt = CB.getByte ();
    if (NTanks == 0) {
      NTanks = nt;
      NPumps = NTanks - 1;
      // System.out.println ("NTanks: "+NTanks+" NPumps: "+NPumps);
      Tanks = new Tank [NTanks];
      Pumps = new Pump [NPumps];
      for (i = 0; i < NTanks; i++) Tanks [i] = new Tank ();
      for (i = 0; i < NPumps; i++) Pumps [i] = new Pump ();
      NeedRedo = true;
    }
  }
  void updateStatus () {
    int ns, no;
    while (CB.more () > 0) {
      if ((no = CB.getByte ()) >= 128) {
        // This is a pump status
        no -= 128;
        if ((ns = CB.getByte ()) > 1) ns = -1;
        // System.out.println ("Pump "+no+", status "+ns);
        if (no < NPumps) Pumps [no] . status = ns;
      } else {
        ns = CB.getShort ();
        // System.out.println ("Tank "+no+", level "+ns);
        if (no < NTanks && (PickTank == null || PickTankNumber != no))
                                                       Tanks [no] . level = ns;
      }
    }
  }
  void processBlock () {
    // Reply distributor
    // CB.dump ();      // +++
    switch (CB.Command) {
      case CommandBuffer.CMD_UPDATE:
        updateStatus ();
        break;
      case CommandBuffer.CMD_GETMAP:
        setParams ();
        break;
      default:
        while (CB.more () > 0) CB.getByte ();
    }
  }
  void runRead () {
    while (OP.ReadThread == Thread.currentThread ()) {
      if (CB.readBlock () < 0) {
        System.out.println ("SIDE disconnected");
        OP.ReadThread = null;
        return;
      }
      processBlock ();
    }
  }
  void runDisplay () {
    // Display event loop
    long CTime;
    while (OP.DisplayThread == Thread.currentThread ()) {
      if (OP.Disconnect) {
        OP.Disconnect = false;
        doStop ();
      }
      setupGraphics ();
      repaint ();
      try {
        try {
          Thread.sleep(MAINLOOPDELAY);
        } catch (InterruptedException e) {
          break;
        }
      } catch (ThreadDeath e) { return; }
      if ((CTime = System.currentTimeMillis ()) - LastBlinkTime > BLINKTIME) {
        if (BlinkON) BlinkON = false; else BlinkON = true;
        LastBlinkTime = CTime;
      }
    }
  }
  void setupGraphics () {
    Dimension d = (this).getSize ();
    if ((OffScreenImage == null) || (d.width != OffScreenSize.width) ||
                                          (d.height != OffScreenSize.height)) {
      // Either we are doing this for the first time, or the image has been
      // resized
      OffScreenImage = createImage (d.width, d.height);
      OffScreenSize = d;
      OffScreenGraphics = OffScreenImage.getGraphics ();
      OffScreenGraphics.setFont (getFont ());
      CurrentFontMetrics = OffScreenGraphics.getFontMetrics();
      NeedRedo = true;
    }
    if (NeedRedo && Tanks != null) {
      redoPositions ();
      NeedRedo = false;
    }
  }
  void redoPositions () {
    // Update the sizes and coordinates of all Tanks and pumps
    int i, w, tw, sp, th, pih, puh, pw, x, xs, ys;
    if (NTanks <= 0) {
      TooSmall = true;
      return;
    } else
      TooSmall = false;
    w = OffScreenSize . width / NTanks;
    tw = (int) (w * TANKWIDTH);
    sp = (w - tw) / 2; // Amount of space on each side of a tank
    pw = (int) (w * PUMPWIDTH);
    th = (int) (OffScreenSize . height * TANKHEIGHT);
    pih = (int) (OffScreenSize . height * PIPEHEIGHT);
    puh = (int) (OffScreenSize . height * PUMPHEIGHT);
    if (w < 6 || puh < 6) {
      TooSmall = true;
      return;
    }
    // Margin
    xs = (OffScreenSize . width - (NTanks * (tw + sp + sp)) + sp + sp) / 2;
    ys = (OffScreenSize . height - th - pih) / 2;
    for (x = xs, i = 0; i < NTanks; i++) {
      Tanks [i] . setPosition (x, ys, tw, th);
      x += sp + sp + tw;
    }
    x = xs + tw + (sp + sp - pw) / 2;
    ys += th - puh;
    for (i = 0; i < NPumps; i++) {
      Pumps [i] . setPosition (x, ys, pw, puh);
      x += sp + sp + tw;
    }
    Pipe . height = pih;
    Pipe . width = NTanks * tw + NPumps * (sp + sp);
    Pipe . x = xs;
    Pipe . y = ys + puh;
  }
  public synchronized void update (Graphics g) { // The painting function
    int i;
    setupGraphics ();
    if (Tanks == null || TooSmall) return;
    OffScreenGraphics.setColor (BGRCOLOR);
    OffScreenGraphics.fillRect (0, 0, OffScreenSize.width,
                                                         OffScreenSize.height);
    // The pipe at the bottom
    OffScreenGraphics.setColor (PIPECOLOR);
    OffScreenGraphics.fillRect (Pipe.x, Pipe.y, Pipe.width, Pipe.height);
    OffScreenGraphics.setColor (Color.black);
    OffScreenGraphics.drawRect (Pipe.x, Pipe.y, Pipe.width, Pipe.height);
    for (i = 0; i < NTanks; i++) Tanks [i] . draw (OffScreenGraphics);
    for (i = 0; i < NPumps; i++) Pumps [i] . draw (OffScreenGraphics, BlinkON);
    g.drawImage (OffScreenImage, 0, 0, null);
  }
  class LocalMouseAdapter extends MouseAdapter {
  // ------------- //
  // Mouse actions //
  // ------------- //
    public void mousePressed (MouseEvent e) {
      int x = e.getX (), y = e.getY ();
      int i;
      PickTank = null;
      for (i = 0; i < NTanks; i++) {
        if (Tanks [i] . inWater (x, y)) {
          PickTank = Tanks [PickTankNumber = i];
          return;
        }
      }
    }
    public void mouseReleased (MouseEvent e) {
      int i, x = e.getX (), y = e.getY ();
      if (PickTank != null) {
        PickTank . setWater (y);
        doWaterRequest (PickTankNumber, PickTank . level);
        PickTank = null;
        repaint ();
      }
    }
  }
  class LocalMouseMotionAdapter extends MouseMotionAdapter {
    public void mouseDragged (MouseEvent e) {
      int x = e.getX (), y = e.getY ();
      if (PickTank != null) {
        PickTank . setWater (y);
        repaint ();
      }
    }
  }
  public void display () {
    setupGraphics ();
    repaint();
  }
  synchronized void doStop () {
    if (OP.ReadThread != null) {
      RBOffset = 0;
      outByte (CommandBuffer.CMD_DISCONNECT);
      outShort (0);
      if (!sendRequest ()) {
        System.out.println ("SIDE disconnected");
      }
    }
  }
}
class Pump {
  final static Color ONCOLOR = new Color (255,69,0);
  final static Color OFFCOLOR = new Color (102,102,102);
  final static Color INDCOLOR = new Color (179,238,58);
  final static double ASIZE = 0.8; // Size of the direction indicator
  final static int LEFT = -1;
  final static int RIGHT = 1;
  final static int OFF = 0;
  int x, y; // Coordinates
  int w, h; // Width and height
  Polygon lp, rp;
  int status;
  public Pump () {
    x = y = w = h = 0;
    status = OFF;
    lp = rp = null;
  }
  public void setPosition (int xc, int yc, int wi, int hi) {
    int xt1, xt2, yt1, yt2, ytm;
    x = xc;
    y = yc;
    w = wi;
    h = hi;
    xt1 = w - (int) (w * ASIZE);
    yt1 = h - (int) (h * ASIZE);
    xt2 = x + w - xt1;
    xt1 = x + xt1;
    yt2 = y + h - yt1;
    yt1 = y + yt1;
    ytm = y + h/2;
    lp = new Polygon ();
    rp = new Polygon ();
    lp . addPoint (xt1, ytm);
    lp . addPoint (xt2, yt1);
    lp . addPoint (xt2, yt2);
    rp . addPoint (xt2, ytm);
    rp . addPoint (xt1, yt1);
    rp . addPoint (xt1, yt2);
  }
  public void draw (Graphics g, boolean blink) {
    g.setColor (status != OFF ? ONCOLOR : OFFCOLOR);
    g.fillRect (x, y, w, h);
    g.setColor (Color.black);
    g.drawRect (x, y, w, h);
    if (blink && status != OFF) {
      Polygon p = (status == LEFT) ? lp : rp;
      g.setColor (INDCOLOR);
      g.fillPolygon (p);
      g.setColor (Color.black);
      g.drawPolygon (p);
    }
  }
}
class RootPanel {
  final static int TWIDTH = 313; // Picture width
  final static int THEIGHT = 116; // and height
  final static int EWIDTH = 66; // Eye width
  final static int EHEIGHT = 63; // Eye height
  final static int VINSET = 40; // Vertical inset for the eyes
  final static int HINSET = 20; // Horizontal inset
  final static int EMASK = 3; // Mask for the number of frames
  final static int DELAYMASK = 1023;
  final static String TFILE = "bgr.gif"; // Background picture
  final static String EFILES [] = {
                      "eye1.gif", "eye2.gif", // Animated stages of a button
                      "eye3.gif", "eye4.gif",
                                  "eyeC.gif"}; // Clicked button
  Image Title, Eyes [], CurrEye [];
  int NEyes, NFrames, XCor [], YCor [];
  long CTime, UpdateTime [];
  Image OSI;
  Graphics OSG;
  Random RNG;
  Rectangle CRect [];
  MediaTracker MTracker;
  int MouseEye;
  Applet AP;
  RootPanel (int ne, Applet a) {
    URL url;
    int i, gap, shf;
    MouseEye = -1;
    AP = a;
    CRect = new Rectangle [NEyes = ne];
    Eyes = new Image [(NFrames = EFILES.length-1) + 1];
    CurrEye = new Image [NEyes];
    UpdateTime = new long [NEyes];
    for (i = 0; i < NEyes; i++) UpdateTime [i] = 0;
    XCor = new int [NEyes];
    YCor = new int [NEyes];
    CTime = 0;
    RNG = new Random ();
    try {
      url = new URL (AP.getDocumentBase (), TFILE);
    } catch (MalformedURLException e) {
      url = null;
    }
    Title = AP.getImage (url);
    MTracker = new MediaTracker (AP);
    MTracker.addImage (Title, 0);
    for (i = 0; i <= NFrames; i++) {
      try {
        url = new URL (AP.getDocumentBase (), EFILES [i]);
      } catch (MalformedURLException e) {
        url = null;
      }
      Eyes [i] = AP.getImage (url);
      MTracker.addImage (Eyes [i], 0);
    }
    for (i = 0; i < NEyes; i++) CurrEye [i] = Eyes [0];
    // Calculate positions for the buttons
    gap = ((TWIDTH - HINSET - HINSET) - (EWIDTH * NEyes)) / (NEyes - 1);
    shf = TWIDTH - EWIDTH * NEyes - gap * (NEyes - 1) - HINSET - HINSET;
    shf = (shf > 0) ? HINSET + shf/2 : HINSET;
    for (i = 0; i < NEyes; i++, shf += EWIDTH + gap) {
      XCor [i] = shf;
      YCor [i] = VINSET;
      CRect [i] = new Rectangle (XCor [i], YCor [i], EWIDTH, EHEIGHT);
    }
    OSI = AP.createImage (TWIDTH, THEIGHT);
    OSG = OSI.getGraphics ();
  }
  public boolean click (int x, int y) {
    for (int i = 0; i < NEyes; i++)
      if (CRect [i] . contains (x, y)) {
        CurrEye [MouseEye = i] = Eyes [NFrames]; // Special "clicked" button
        return true;
      }
    return false;
  }
  public int release (int x, int y) {
    int m;
    if ((m = MouseEye) != -1) {
      MouseEye = -1;
      CurrEye [m] = Eyes [0];
      if (CRect [m] . contains (x, y))
        return m + 1;
    }
    return 0;
  }
  public void draw () {
    OSG.drawImage (Title, 0, 0, TWIDTH, THEIGHT, null);
    for (int i = 0; i < NEyes; i++)
      OSG.drawImage (CurrEye [i], XCor [i], YCor [i], EWIDTH, EHEIGHT, null);
  }
  public void waitForImages () {
    try {
      try {
        MTracker.waitForAll ();
      } catch (InterruptedException e) { }
    } catch (ThreadDeath e) { return; }
  }
  public void updateImage () {
    for (int i = 0; i < NEyes; i++) {
      if (i != MouseEye && UpdateTime [i] <= CTime) {
        int rand = RNG.nextInt ();
        int ne = (int)((rand >> 6) & EMASK);
        int nd = (int)((rand >> 10) & DELAYMASK);
        CurrEye [i] = Eyes [ne];
        UpdateTime [i] = CTime + nd;
      }
    }
  }
}
class Tank {
  final static Color TANKFRMCOLOR = Color.black;
  final static Color WATERCOLOR = new Color (142,229,238);
  final static int MAXLEVEL = 32767;
  final static int MOUSETOLERANCE = 7;
  int x, y; // Coordinates
  int w, h; // Width and height
  int level;
  public Tank () {
    x = y = w = h = 0;
    level = 0;
  }
  public void setPosition (int xc, int yc, int wi, int hi) {
    int xt1, xt2, yt1, yt2, ytm;
    x = xc;
    y = yc;
    w = wi;
    h = hi;
  }
  public void draw (Graphics g) {
    int ylev = (h * level) / MAXLEVEL;
    if (ylev > h) ylev = h;
    if (ylev > 0) {
      g.setColor (WATERCOLOR);
      g.fillRect (x, y + h - ylev, w, ylev);
    }
    g.setColor (Color.black);
    g.drawRect (x, y, w, h);
  }
  public boolean inWater (int xc, int yc) {
    int ylev = y + h - ((h * level) / MAXLEVEL);
    return
      (xc >= x) && (xc < x + w) && (yc >= ylev - MOUSETOLERANCE) &&
        (yc < ylev + MOUSETOLERANCE);
  }
  public void setWater (int yc) {
    if (yc < y)
      yc = y;
    else if (yc > y + h)
      yc = y + h;
    level = ((h - (yc - y)) * MAXLEVEL) / h;
  }
}
public class Water extends Applet implements Runnable {
  final static int NEYES = 3; // How many eyes we have
  RootPanel RP;
  Thread Animator = null;
  TankPanel GP;
  Thread ReadThread = null, DisplayThread = null;
  boolean Disconnect = false;
  public void init () {
    RP = new RootPanel (NEYES, this);
    addMouseListener (new LocalMouseAdapter ());
  }
  public synchronized void start () {
    if (Animator == null) {
      Animator = new Thread (this);
      Animator.start ();
    }
  }
  public synchronized void stop () {
    if (Animator != null && Animator.isAlive ())
      Animator.stop ();
    Animator = null;
  }
  synchronized void startup () {
    Disconnect = false;
    if (GP != null) return; // Things seem to be doing fine
    GP = new TankPanel (this);
    if (GP.CB == null) {
      GP = null;
      return; // We have failed
    }
    if (DisplayThread == null) {
      DisplayThread = new Thread (this);
      DisplayThread . start ();
      ReadThread = new Thread (this);
      ReadThread . start ();
    }
  }
  class LocalMouseAdapter extends MouseAdapter {
    public void mousePressed (MouseEvent e) {
      int x = e.getX (), y = e.getY ();
      if (RP.click (x, y)) repaint ();
    }
    public void mouseReleased (MouseEvent e) {
      int x = e.getX (), y = e.getY ();
      switch (RP.release (x, y)) {
        case 1:
          startup ();
          break;
        case 2:
        case 3:
          Disconnect = true;
          break;
        default:
          return;
      }
    }
  }
  public synchronized void update (Graphics g) {
    RP.draw ();
    g.drawImage (RP.OSI, 0, 0, null);
  }
  public void run () {
    // Tell who you are
    Thread ct = Thread.currentThread ();
    if (Animator == ct) {
      ct.setPriority (Thread.MIN_PRIORITY);
      // Wait for the images to be downloaded
      RP.waitForImages ();
      while (Animator == ct) {
        RP.updateImage ();
        // Check update times
        repaint ();
        try {
          try {
            Thread.sleep (300);
            RP.CTime += 300;
          } catch (InterruptedException e) {
            RP.CTime += 300;
          }
        } catch (ThreadDeath e) { return; }
      }
      return;
    }
    if (GP == null) return;
    if (ReadThread == ct) {
      GP.runRead ();
      DisplayThread = null;
    } else if (DisplayThread == ct) {
      GP.runDisplay ();
      if (ReadThread != null) {
        ReadThread.stop ();
        ReadThread = null;
      }
      System.out.println ("Master thread exit");
      try { GP.Sock.close (); } catch (IOException e) { }
      // OW.dispose ();
      (GP).setVisible (false);
      GP = null;
    }
  }
  public void destroy () {
    Disconnect = true;
    while (GP != null) {
      try {
        try {
          Thread.sleep(100);
        } catch (InterruptedException e) {
          continue;
        }
      } catch (ThreadDeath e) { return; }
    }
  }
}
