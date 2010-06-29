import java.io.*;
import java.lang.*;
import java.net.*;
import java.awt.*;
import java.util.*;
import java.applet.Applet;
import java.awt.event.*;
class AlertArea extends TextArea {
  final static int ADICTINITSIZE = 64; // Initial size of the dictionary
  final static int STRBFINITSIZE = 48; // Initial size of a string buffer
  final static int NROWS = 24; // The number of visible rows
  final static int NCOLS = 40; // Visible columns
  final static int NLINES = 64; // The number of text lines
  final static int X_ERROR = 1; // Alert types
  final static int X_WARNING = 2;
  final static int X_BADCMD = 3;
  final static Color BACKGROUNDN = Color.gray; // Normal
  final static Color BACKGROUNDA = Color.red; // Alarm
  final static Color FOREGROUND = Color.black; // Text
  // Alert types (borrowed from operator.h)
  final static int ALERT = 0;
  final static int OVERRIDE = 1;
  int LPos [] = new int [NLINES]; // Line endings for scrolling
  boolean AlarmBackground; // Tells the background type
  AlertDictionaryItem ADict [] = new AlertDictionaryItem [ADICTINITSIZE];
  OperatorWindow OP; // Where we come from
  Unit LastAlerted;
  AlertArea (OperatorWindow op) {
    super ("", NROWS, NCOLS);
    int i;
    OP = op;
    for (i = 0; i < NLINES; i++) LPos [i] = -1; // No lines
    setBackground (BACKGROUNDN);
    setForeground (FOREGROUND);
    AlarmBackground = false;
    LastAlerted = null;
  }
  void displayLine (String s) {
    int i, loc, sp;
    // The line goes at the end of the area
    // System.out.println ("Alert: "+s);  // +++
    for (loc = NLINES-1; (loc >= 0) && (LPos [loc] < 0); loc--);
    loc++;
    if (loc == NLINES) {
      // Scroll the first line out
      (this).replaceRange ("", 0, sp = LPos [0]);
      for (i = 0; i < NLINES-1; i++) LPos [i] = LPos [i+1] - sp;
      loc = NLINES-1;
    }
    sp = (loc == 0) ? 0 : LPos [loc-1];
    (this).append (s);
    // We assume that the text doesn't have '\n' at the end
    (this).append ("\n");
    LPos [loc] = sp + s.length () + 1;
  }
  public synchronized void update (CommandBuffer CB) {
    int aix, ix, nl, ur;
    StringBuffer s;
    Unit u;
    AlertDictionaryItem Aux [], ae;
    if (CB.Command == CommandBuffer.CMD_GETMAP) {
      // A new alert map entry
      ix = CB.getShort ();
      if (ix >= ADict.length) {
        // The dictionary must grow
        for (nl = ADict.length * 2; nl <= ix; nl += nl);
        Aux = new AlertDictionaryItem [nl];
        for (nl = 0; nl < ADict.length; nl++) Aux [nl] = ADict [nl];
        ADict = Aux;
        Aux = null;
      }
      if (ADict [ix] == null)
        // Create a new entry
        ADict [ix] = new AlertDictionaryItem ();
      // Fill the entry
      ADict [ix] . UId = CB.getShort ();
      ADict [ix] . UType = CB.getByte ();
      ADict [ix] . AType = CB.getByte ();
      ADict [ix] . Id = CB.getString ();
      // No further action -- just an internal update
    } else if (CB.Command == CommandBuffer.CMD_ALERT) {
      // An alert: send it to the display area
      aix = CB.getShort (); // Alert index
      if (aix < ADict.length && aix >= 0)
        ae = ADict [aix];
      else
        ae = null;
      ur = CB.getByte (); // Urgency
      if (ur == X_ERROR) {
        if (!AlarmBackground) {
          AlarmBackground = true;
          setBackground (BACKGROUNDA);
        }
      } else {
        if (AlarmBackground) {
          AlarmBackground = false;
          setBackground (BACKGROUNDN);
        }
      }
      s = new StringBuffer (STRBFINITSIZE);
      s.append ('(');
      if (ur != X_BADCMD) {
        // This in fact comes from an object
        if (ae != null)
          s.append (ae.Id);
        else
          s.append ("??");
      } else
        s.append ("OP");
      s.append (") ");
      s.append (CB.getString ());
      displayLine (s.toString ());
    }
    // Not your command -- ignore it
  }
  public void localAlert (String s) {
    if (AlarmBackground) {
      AlarmBackground = false;
      setBackground (BACKGROUNDN);
    }
    displayLine (s);
  }
  public synchronized AlertDictionaryItem getOverride (Unit u, int indx)
                                      throws ArrayIndexOutOfBoundsException {
    AlertDictionaryItem adi;
    if (indx >= ADict.length) {
      throw new ArrayIndexOutOfBoundsException ();
    } else {
      adi = ADict [indx];
      if (adi == null || adi.AType != OVERRIDE || adi.UId != u.UId ||
        adi.UType != u.UType) return null;
      return adi;
    }
  }
}
class AlertDictionaryItem {
  String Id; // Textual description
  int AType, // Alert type (e.g., override)
      UId, // Owning unit Id
      UType; // Owning unit type
}
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
class Dangle {
  Dangle next; // Dangling connection waiting for a
  Unit SU; // matching successor to arrive
  int NUId, NUType, NDUId, NDUType;
}
class ConvPanel extends Panel {
  final static int UNITSINITSIZE = 64; // Initial size of Units array
  final static int SEGMENT = 0; // Unit types
  final static int DIVERTER = 1;
  final static int SSEGMENT = 2;
  final static int SDIVERTER = 3;
  final static int SOURCE = 4;
  final static int SINK = 5;
  final static int MERGER = 6;
  final static int SMERGER = 7;
  final static double ARROWWIDTH = 5.0; // Arrow size
  final static double ARROWHEIGHT = 7.0;
  final static int MOVETOLERANCE = 5; // This is in pixels
  final static Color BGRCOLOR = new Color (102,205,170);
  final static Color SEGMENTCOLOR = Color.cyan;
  final static Color DIVERTERCOLOR = Color.magenta;
  final static Color SOURCECOLOR = Color.yellow;
  final static Color SINKCOLOR = Color.orange;
  final static Color MERGERCOLOR = Color.pink;
  final static Color SELECTCOLOR = Color.green;
  final static Color ALERTCOLOR = Color.red;
  final static Color OUTEDGECOLOR = Color.black;
  final static Color DIVERTEDGECOLOR = Color.black;
  Dangle DL = null; // Dangling connection list
  Unit Units [];
  int NUnits; // The current number of units
  OperatorWindow OP; // Where we come from
  Image OffScreenImage;
  Dimension OffScreenSize;
  Graphics OffScreenGraphics;
  FontMetrics CurrentFontMetrics;
  boolean NeedRedo = false;
  Unit PickUnit;
  int PickX, PickY, PickDX, PickDY; // For mouse actions
  int UnitHeight;
  int LevelDepth; // Used by distanceFromTheRight
  int LevelBegin [], LevelEnd [], LevelMap [];
  ConvPanel (OperatorWindow op) {
    OP = op;
    Units = new Unit [UNITSINITSIZE];
    NUnits = 0;
    OffScreenImage = null;
    LevelBegin = LevelEnd = LevelMap = null;
    addMouseListener (new LocalMouseAdapter ());
    addMouseMotionListener (new LocalMouseMotionAdapter ());
  }
  void markHeads () {
    Unit u;
    int i;
    for (i = 0; i < NUnits; i++) {
      Units [i] . Head = true;
      Units [i] . x = -1;
    }
    for (i = 0; i < NUnits; i++) {
      u = Units [i];
      if (u.NU != null) {
        if (u.NU.Head) {
          u.NU.Head = false;
        }
      }
      if (u.NDU != null) {
        if (u.NDU.Head) {
          u.NDU.Head = false;
        }
      }
    }
  }
  int distanceFromTheRight (Unit cu, int level) {
    // Calculates horizontal locations (unscaled)
    int fd, sd;
    if (cu.NU != null) {
       if (cu.NU.x < 0)
         fd = distanceFromTheRight (cu.NU, level);
       else
         fd = cu.NU.x;
    } else
      fd = 0;
    if (cu.NDU != null) {
      LevelDepth++;
      sd = distanceFromTheRight (cu.NDU, LevelDepth);
      if (sd > fd) fd = sd;
    }
    cu.x = 1 + fd;
    cu.y = level;
    return cu.x;
  }
  void markLevels (Unit cu) {
    LevelBegin [cu.y] = cu.x;
    while (true) {
      if (cu.NDU != null) {
        LevelBegin [cu.NDU.y] = cu.NDU.x;
        markLevels (cu.NDU);
      }
      if (cu.NU == null || cu.NU.y != cu.y) {
        LevelEnd [cu.y] = cu.x;
        return;
      }
      cu = cu.NU;
    }
  }
  void mergeLevels () {
    int i, k, l, nlevels;
    for (i = 0; i < LevelDepth; i++) LevelMap [i] = i; // Identical mapping
    nlevels = 0;
    for (i = 0; i < LevelDepth; i++) {
      // We are looking at level i trying to squeeze it into one of already
      // compressed levels
      for (k = 0; k < nlevels; k++) {
        for (l = 0; l < LevelDepth; l++) {
          if (LevelMap [l] == k) {
            // Check if the present level doesn't overlap with any fragment
            // of this level
            if ((LevelBegin [i] >= LevelEnd [l] &&
                 LevelBegin [i] <= LevelBegin [l]) ||
                (LevelBegin [l] >= LevelEnd [i] &&
                 LevelBegin [l] <= LevelBegin [i]) ) break;
          }
        }
        if (l == LevelDepth) {
          // The level can be merged with k
          LevelMap [i] = k;
          break;
        }
      }
      if (k == nlevels) {
        LevelMap [i] = nlevels;
        nlevels++;
      }
    }
    LevelDepth = nlevels;
  }
  void rescaleHorizontal (int MaxCx) {
    int i;
    Unit cu;
    if (MaxCx < 2) MaxCx = 2; // Avoid division by zero
    for (i = 0; i < NUnits; i++) {
      cu = Units [i];
      cu.x = ((OffScreenSize.width - 30) * (MaxCx - cu.x)) / (MaxCx - 1) + 15;
    }
  }
  void rescaleVertical (int NLevels) {
    int i;
    Unit u;
    for (i = 0; i < NUnits; i++) {
      u = Units [i];
      u.y = 8 + ((u.y + 1) * OffScreenSize.height) / (NLevels + 1);
    }
  }
  void paintArrow (Graphics g, int x1, int y1, int x2, int y2, Color c) {
    double x, y, xb, yb, dx, dy, l, sina, cosa;
    int xc, yc, xd, yd;
    g.setColor (c);
    dx = x2 - x1;
    dy = y1 - y2;
    l = Math.sqrt((double) (dx * dx + dy * dy));
    if (l == 0.0) return; // Arrow length is zero
    sina = dy / l;
    cosa = dx / l;
    xb = x2 * cosa - y2 * sina;
    yb = x2 * sina + y2 * cosa;
    x = xb - ARROWHEIGHT;
    y = yb - ARROWWIDTH / 2.0;
    xc = (int) (x * cosa + y * sina + .5);
    yc = (int) (-x * sina + y * cosa + .5);
    y = yb + ARROWWIDTH / 2.0;
    xd = (int) (x * cosa + y * sina + .5);
    yd = (int) (-x * sina + y * cosa + .5);
    g.drawLine (x1, y1, x2, y2);
    g.drawLine (xc, yc, x2, y2);
    g.drawLine (xd, yd, x2, y2);
  }
  public synchronized void update (Graphics g) { // The painting function
    int i, x, y;
    Unit u, v;
    Color c;
    setupGraphics ();
    OffScreenGraphics.setColor (BGRCOLOR);
    OffScreenGraphics.fillRect (0, 0, OffScreenSize.width,
                                                         OffScreenSize.height);
    // Nodes first
    for (i = 0; i < NUnits; i++) {
      u = Units [i];
      x = u.x;
      y = u.y;
      if (u == OP.AE.LastAlerted) {
        c = ALERTCOLOR;
      } else if (u.Picked || u == PickUnit) {
        c = SELECTCOLOR;
      } else {
        switch (u.UType) {
          case SEGMENT:
          case SSEGMENT:
            c = SEGMENTCOLOR;
            break;
          case DIVERTER:
          case SDIVERTER:
            c = DIVERTERCOLOR;
            break;
          case MERGER:
          case SMERGER:
            c = MERGERCOLOR;
            break;
          case SOURCE:
            c = SOURCECOLOR;
            break;
          case SINK:
            c = SINKCOLOR;
            break;
          default:
            // This is just in case, we won't get here
            c = Color.white;
        }
      }
      OffScreenGraphics.setColor (c);
      OffScreenGraphics.fillRect (x - u.width/2, y - UnitHeight/2,
                                                          u.width, UnitHeight);
      OffScreenGraphics.setColor (Color.black);
      OffScreenGraphics.drawRect (x - u.width/2, y - UnitHeight/2,
                                                      u.width-1, UnitHeight-1);
      OffScreenGraphics.drawString (u.Label, x - (u.width-10)/2,
                       (y-(UnitHeight-4)/2) + CurrentFontMetrics.getAscent ());
    }
    // Now for the edges
    for (i = 0; i < NUnits; i++) {
      u = Units [i];
      if ((v = u.NU) != null)
        // This is the straightforward "out" edge
        paintArrow (OffScreenGraphics, u.x+u.width/2, u.y,
                                              v.x-v.width/2, v.y, OUTEDGECOLOR);
      if ((v = u.NDU) != null)
        // This is a divert edge
        paintArrow (OffScreenGraphics, u.x,
         (v.y > u.y) ? u.y + UnitHeight/2 : u.y - UnitHeight/2,
          v.x - v.width/2, v.y, DIVERTEDGECOLOR);
    }
    g.drawImage (OffScreenImage, 0, 0, null);
  }
  void redoUnits () {
    // Update the sizes and coordinates of all units for display
    int i, NHeads, SW, SH, MaxCx;
    Unit u;
    UnitHeight = CurrentFontMetrics.getHeight() + 4;
    for (i = 0; i < NUnits; i++) {
      u = Units [i];
      u.width = CurrentFontMetrics.stringWidth (u.Label) + 10;
    }
    // Now for the tricky part
    markHeads (); // Identify sources
    LevelDepth = 0;
    MaxCx = 0;
    for (i = 0; i < NUnits; i++) if (Units [i].Head) {
      // Identify threads
      distanceFromTheRight (Units [i], LevelDepth);
      if (Units [i].x > MaxCx) MaxCx = Units [i].x;
      // Note that LevelDepth can be increased by distanceFromTheRight
      LevelDepth++;
    }
    if (LevelBegin == null || LevelBegin.length != LevelDepth) {
      LevelBegin = new int [LevelDepth];
      LevelEnd = new int [LevelDepth];
      LevelMap = new int [LevelDepth];
    }
    for (i = 0; i < NUnits; i++) if (Units [i].Head) markLevels (Units [i]);
    mergeLevels ();
    // Remap levels
    for (i = 0; i < NUnits; i++) Units [i].y = LevelMap [Units [i].y];
    rescaleHorizontal (MaxCx);
    // Rescale vertical positions
    rescaleVertical (LevelDepth);
    // Now we are ready to display the graph
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
    if (NeedRedo) {
      redoUnits ();
      NeedRedo = false;
    }
  }
  class LocalMouseAdapter extends MouseAdapter {
  // ------------- //
  // Mouse actions //
  // ------------- //
    public void mousePressed (MouseEvent e) {
      int x = e.getX (), y = e.getY ();
      double bestdist, dist;
      int i;
      Unit u;
      PickUnit = null;
      bestdist = Double.MAX_VALUE;
        for (i = 0 ; i < NUnits ; i++) {
        u = Units [i];
        if (u.x - u.width/2 <= x && u.x + u.width/2 >= x &&
                          u.y - UnitHeight/2 <= y && u.y + UnitHeight/2 >= y ) {
          // Now check the distance to the center to select the best of
          // multiple overlapping candidates
          dist = (u.x - x) * (u.x - x) + (u.y - y) * (u.y - y);
          if (dist < bestdist) {
            PickUnit = u;
            bestdist = dist;
          }
        }
      }
      if (PickUnit != null) {
        PickX = PickUnit.x;
        PickY = PickUnit.y;
        PickDX = PickX - x;
        PickDY = PickY - y;
        repaint();
      }
    }
    public void mouseReleased (MouseEvent e) {
      int x = e.getX (), y = e.getY ();
      int dx, dy;
      if (PickUnit != null) {
        x += PickDX;
        y += PickDY;
        // Determine how far we have moved
        dx = PickX - x; if (dx < 0) dx = - dx;
        dy = PickY - y; if (dy < 0) dy = - dy;
        if (dx < MOVETOLERANCE && dy < MOVETOLERANCE) {
          // This was a simple click -- reverse the "picked" status
          if (OP.AE.LastAlerted == PickUnit) {
            // But don't change it if the unit was "alerted" -- then just
            // clear the "alerted" status
            OP.AE.LastAlerted = null;
          } else {
            PickUnit.Picked = PickUnit.Picked ? false : true;
            OP.selectedToMenu (PickUnit); // Update menus
          }
        }
        if (x > 0 && x < OffScreenSize.width &&
                                          y > 0 && y < OffScreenSize.height) {
          PickUnit.x = x;
          PickUnit.y = y;
        }
        PickUnit = null;
        repaint();
      }
    }
  }
  class LocalMouseMotionAdapter extends MouseMotionAdapter {
    public void mouseDragged (MouseEvent e) {
      int x = e.getX (), y = e.getY ();
      if (PickUnit != null) {
        x += PickDX;
        y += PickDY;
        if (x > 0 && x < OffScreenSize.width &&
                             y > 0 && y < OffScreenSize.height) {
          PickUnit.x = x;
          PickUnit.y = y;
        }
        repaint();
      }
    }
  }
  void addDangle (Unit u, int nuid, int nutype, int nduid, int ndutype) {
    Dangle d = new Dangle ();
    d.NUId = nuid;
    d.NUType = nutype;
    d.NDUId = nduid;
    d.NDUType = ndutype;
    d.SU = u;
    d.next = DL;
    DL = d;
  }
  void tryDangle (Unit u) {
    Dangle dp, dc;
    for (dp = null, dc = DL; dc != null; ) {
      if (dc.NUId == u.UId && dc.NUType == u.UType) {
        if (dc.SU.NU == null) dc.SU.NU = u;
        dc.NUId = -1;
      }
      if (dc.NDUId == u.UId && dc.NDUType == u.UType) {
        if (dc.SU.NDU == null) dc.SU.NDU = u;
        dc.NDUId = -1;
      }
      if (dc.NUId == -1 && dc.NDUId == -1) {
        // The entry can be deleted
        if (dp == null)
          DL = dc.next;
        else
          dp.next = dc.next;
      } else { dp = dc; }
      dc = dc.next;
    }
  }
  Unit locateUnit (int uid, int utype) {
    int i;
    Unit u;
    for (i = 0; i < NUnits; i++) {
      u = Units [i];
      if (u.UId == uid && u.UType == utype) return u;
    }
    return null;
  }
  int addUnit (Unit v) {
    int i;
    Unit U[];
    if (NUnits == Units.length) {
      // No room in the array -- grow it
      U = Units;
      Units = new Unit [NUnits + NUnits];
      for (i = 0; i < NUnits; i++) Units [i] = U [i];
      U = null;
    }
    Units [i = NUnits++] = v;
    return i;
  }
  void update (CommandBuffer CB) {
    int uid, utype, nuid, nutype, nduid, ndutype;
    Unit u, su;
    boolean incomplete;
    StringBuffer s;
    if (CB.Command != CommandBuffer.CMD_GETGRAPH) return;
    // This is your command
    uid = CB.getShort ();
    utype = CB.getByte ();
    // Check if this unit is not defined already
    if ((u = locateUnit (uid, utype)) != null) return;
    u = new Unit ();
    u.UId = uid;
    u.UType = utype;
    u.Picked = false;
    u.NU = u.NDU = null;
    incomplete = false;
    nuid = nutype = nduid = ndutype = -1;
    if (utype != SINK) {
      // There is a successor
      nuid = CB.getShort ();
      nutype = CB.getByte ();
      // Locate the successor
      if ((su = locateUnit (nuid, nutype)) != null)
        u.NU = su;
      else
        incomplete = true;
      if (utype == SDIVERTER) {
        // There is a second successor
        nduid = CB.getShort ();
        ndutype = CB.getByte ();
        if ((su = locateUnit (nduid, ndutype)) != null)
          u.NDU = su;
        else
          incomplete = true;
      }
    }
    if (incomplete) addDangle (u, nuid, nutype, nduid, ndutype);
    // Construct the unit label
    switch (utype) {
      case SINK:
        s = new StringBuffer ("@");
        break;
      case SEGMENT:
      case SSEGMENT:
        s = new StringBuffer ("S");
        break;
      case DIVERTER:
      case SDIVERTER:
        s = new StringBuffer ("D");
        break;
      case MERGER:
      case SMERGER:
        s = new StringBuffer ("M");
        break;
      case SOURCE:
        s = new StringBuffer (">");
        break;
      default:
        s = new StringBuffer ("?");
    }
    s.append (uid);
    u.Label = s.toString ();
    addUnit (u);
    // Try to resolve dangling pointers
    tryDangle (u);
    NeedRedo = true;
  }
  public void display () {
    setupGraphics ();
    repaint();
  }
}
class MenuListEntry implements ActionListener {
  MenuItem TheItem; // Pointer to the item
  Unit U; // Owning unit
  int OI; // Override index
  MenuListEntry next; // For linking these things together
  OperatorWindow OW;
  public void actionPerformed (ActionEvent e) {
    OW.ActionMenu = this;
    OW.dispatchEvent (e);
  }
  MenuListEntry (OperatorWindow ow) {
    OW = ow;
  }
  public void activateMenu (MenuItem mi) {
    TheItem = mi;
    TheItem.addActionListener (this);
    TheItem.addActionListener (OW);
  }
}
class OperatorWindow extends Frame implements ActionListener {
  MenuBar Menus;
  Menu DisplayMenu, CancelMenu, OverrideMenu;
  AlertArea AE;
  ConvPanel Graph;
  Panel DisplayPanel;
  Socket sock = null;
  CommandBuffer CB;
  StatusWindow StatusWindowList = null;
  boolean SimulatorRunning = false;
  MenuListEntry DisplayMenuList = null,
                CancelMenuList = null,
                OverrideMenuList = null,
                WaitingForDialog = null;
  MenuListEntry ActionMenu = null;
  OutputStream OS; // Output stream to the simulator
  OverrideDialog OV;
  Conveyor OP;
  // We need these because there is no parameter passing by reference
  // in java. Well, we could have done it another way, but who cares
  // really.
  final static int DISPLAYMENULIST = 0; // Menu list ordinals
  final static int CANCELMENULIST = 1;
  final static int OVERRIDEMENULIST = 2;
  final static int REQUESTBUFFERSIZE = 32; // Requests tend to be short
  final static int OWWIDTH = 900; // Operator window size
  final static int OWHEIGHT = 490;
  final static int MAINLOOPDELAY = 600; // Main loop delay
  byte RequestBuffer [] = new byte [REQUESTBUFFERSIZE];
  int RBOffset;
  synchronized void displayStatusWindows () {
    StatusWindow w;
    for (w = StatusWindowList; w != null; w = w.next) {
      w.SD.setupGraphics ();
      w.SD.repaint ();
    }
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
  public void dump (int length) {
    // For debugging only
    int i;
    char c1, c2;
    System.out.println ("+++Block sent: "+length);
    for (i = 0; i < length; i++) {
      c1 = (char) ((RequestBuffer [i] >> 4) & 0xf);
      c2 = (char) (RequestBuffer [i] & 0xf);
      if (c1 > 9) c1 += 'a' - 10; else c1 += '0';
      if (c2 > 9) c2 += 'a' - 10; else c2 += '0';
      System.out.print (c1);
      System.out.print (c2);
      System.out.print (" (");
      if ((c1 = (char)(RequestBuffer [i])) < 0x20 || c1 > 0x7f) c1 = ' ';
      System.out.print (c1);
      System.out.print (") ");
      if (i != 0 && (i % 10 == 0) && i < length-1) System.out.print ('\n');
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
  void doDisplay (MenuListEntry m) {
    // Shuffle the menus
    displayedToMenu (m.U);
    // Prepare the display request
    RBOffset = 0;
    outByte (CommandBuffer.CMD_PERIODIC);
    outShort (3);
    outShort (m.U.UId);
    outByte (m.U.UType);
    if (!sendRequest ()) {
      System.out.println ("SIDE disconnected");
      OP.CommandThread = null;
    }
  }
  public synchronized void doCancel (MenuListEntry m, StatusWindow s) {
    StatusWindow ss;
    if (m == null) {
      // Called from window's action
      if (s == null) return; // Can't really happen
      for (m = CancelMenuList; m != null; m = m.next)
        if (s.SD.UId == m.U.UId && s.SD.UType == m.U.UType) break;
      if (m == null) return; // Shouldn't happen
    }
    undisplayedFromMenu (m.U);
    // Remove and deallocate the window thread
    for (s = StatusWindowList, ss = null; s != null; ss = s, s = s.next)
      if (s.SD.UId == m.U.UId && s.SD.UType == m.U.UType) break;
    if (s != null) {
      // The window actually exists
      if (ss == null)
        StatusWindowList = s.next;
      else
        ss.next = s.next;
      s.SD.destruct ();
    }
    RBOffset = 0;
    outByte (CommandBuffer.CMD_UNPERIODIC);
    outShort (3);
    outShort (m.U.UId);
    outByte (m.U.UType);
    if (!sendRequest ()) {
      System.out.println ("SIDE disconnected");
      OP.CommandThread = null;
    }
  }
  void doOverride (MenuListEntry m) {
    if (WaitingForDialog == null) {
      WaitingForDialog = m;
      OV.Proceed = OV.Cancel = false;
      OV.show (); OV.show (); // Make sure the dialog shows up
    }
  }
  void sendOverride (MenuListEntry m) {
    RBOffset = 0;
    outByte (CommandBuffer.CMD_OVERRIDE);
    outShort (6);
    outShort (m.OI);
    outShort (OV.Action);
    outShort (OV.Value);
    if (!sendRequest ()) {
      System.out.println ("SIDE disconnected");
      OP.CommandThread = null;
    }
  }
  void doGetMap () {
    RBOffset = 0;
    outByte (CommandBuffer.CMD_GETMAP);
    outShort (0);
    if (!sendRequest ()) {
      System.out.println ("SIDE disconnected");
      OP.CommandThread = null;
    }
    RBOffset = 0;
    outByte (CommandBuffer.CMD_GETGRAPH);
    outShort (0);
    if (!sendRequest ()) {
      System.out.println ("SIDE disconnected");
      OP.CommandThread = null;
    }
  }
  void doStart () {
    RBOffset = 0;
    outByte (CommandBuffer.CMD_START);
    outShort (0);
    if (!sendRequest ()) {
      System.out.println ("SIDE disconnected");
      OP.CommandThread = null;
    }
  }
  synchronized void doStop (boolean disconnect) {
    if (OP.ReadThread != null) {
      RBOffset = 0;
      if (disconnect)
        outByte (CommandBuffer.CMD_DISCONNECT);
      else
        outByte (CommandBuffer.CMD_STOP);
      outShort (0);
      if (!sendRequest ()) {
        System.out.println ("SIDE disconnected");
      }
    }
  }
  synchronized void cleanStatusWindows () {
    StatusWindow sw;
    for (sw = StatusWindowList; sw != null; sw = sw.next) sw.SD.destruct ();
    OV.dispose ();
  }
  synchronized public void selectedToMenu (Unit u) {
    // Called when a Unit becomes selected/unselected
    if (u.Picked) {
      if (!withinMenuList (u, CANCELMENULIST))
        addMenuList (u, DISPLAYMENULIST);
      addMenuList (u, OVERRIDEMENULIST);
    } else {
      deleteMenuList (u, DISPLAYMENULIST);
      deleteMenuList (u, OVERRIDEMENULIST);
    }
  }
  synchronized public void displayedToMenu (Unit u) {
    // Called when a unit is displayed
    deleteMenuList (u, DISPLAYMENULIST);
    addMenuList (u, CANCELMENULIST);
  }
  synchronized public void undisplayedFromMenu (Unit u) {
    // Called when a unit display is cancelled
    deleteMenuList (u, CANCELMENULIST);
    selectedToMenu (u);
  }
  MenuListEntry menuList (int ord) {
    switch (ord) {
      case DISPLAYMENULIST: return DisplayMenuList;
      case CANCELMENULIST: return CancelMenuList;
      case OVERRIDEMENULIST: return OverrideMenuList;
    }
    return null;
  }
  synchronized boolean withinMenuList (Unit u, int ord) {
    MenuListEntry m = menuList (ord);
    while (m != null) {
      if (m.U == u) return true;
      m = m.next;
    }
    return false;
  }
  synchronized void addMenuList (Unit u, int ord) {
    MenuListEntry ML = menuList (ord);
    MenuListEntry nm, p, q;
    AlertDictionaryItem adi;
    int i;
    if (withinMenuList (u, ord)) return; // Present there already
    if (ord != OVERRIDEMENULIST) {
      // Process Display or Cancel menu; Override is trickier
      nm = new MenuListEntry (this);
      // New items get added at the end
      nm.next = null;
      if (ML == null) {
        // The list is empty
        if (ord == DISPLAYMENULIST)
          // No reference arguments in java -- sorry
          DisplayMenuList = nm;
        else
          CancelMenuList = nm;
        ML = nm;
      } else {
        for (p = ML; p.next != null; p = p.next);
        p.next = nm;
      }
      nm.U = u;
      // Create the new menu item
      nm.activateMenu (new MenuItem (u.Label));
      if (ord == DISPLAYMENULIST)
        DisplayMenu.add (nm.TheItem);
      else
        CancelMenu.add (nm.TheItem);
    } else {
      // We get here to process the override case
      p = q = null;
      for (i = 0; ; i++) {
        // Go through the alert dictionary and locate all overrides
        // belonging to the given unit
        try {
          adi = AE.getOverride (u, i);
        } catch (ArrayIndexOutOfBoundsException e) {
          // We get this when i gets out of bound for the dictionary
          break;
        }
        if (adi == null) continue;
        // We have a new override to add to the menu
        nm = new MenuListEntry (this);
        nm.next = null;
        if (p == null) // Link up the new entry
          p = nm;
        else
          q.next = nm;
        q = nm;
        nm.U = u;
        nm.OI = i;
        nm.activateMenu (new MenuItem (adi.Id));
        OverrideMenu.add (nm.TheItem);
      }
      if (p != null) {
        // Find the end of the existing list
        if (OverrideMenuList == null) {
          OverrideMenuList = p;
        } else {
          for (q = OverrideMenuList; q.next != null; q = q.next);
          q.next = p;
        }
      }
    }
  }
  synchronized void deleteMenuList (Unit u, int ord) {
    MenuListEntry ML = menuList (ord);
    MenuListEntry p, q;
    // Override menus are now treated in the same way
    for (p = ML, q = null; p != null; p = p.next) {
      // Locate the entry on the list
      if (p.U == u) {
        // Remove from the menu
        switch (ord) {
          case DISPLAYMENULIST:
            DisplayMenu.remove (p.TheItem);
            break;
          case CANCELMENULIST:
            CancelMenu.remove (p.TheItem);
            break;
          case OVERRIDEMENULIST:
            OverrideMenu.remove (p.TheItem);
            break;
        }
        // And from the list
        if (q == null) {
          // We are at the very beginning
          switch (ord) {
            case DISPLAYMENULIST:
              DisplayMenuList = p.next;
              break;
            case CANCELMENULIST:
              CancelMenuList = p.next;
              break;
            case OVERRIDEMENULIST:
              OverrideMenuList = p.next;
              break;
          }
        } else {
          q.next = p.next;
        }
      } else {
          q = p;
      }
    }
  }
  synchronized void updateStatus () {
    int uid, utype;
    StatusWindow s;
    StatusDisplay x;
    Unit u;
    if (CB.Command != CommandBuffer.CMD_UPDATE) return; // Not ours
    uid = CB.getShort ();
    utype = CB.getByte ();
    // Find the window for this unit
    for (s = StatusWindowList; s != null; s = s.next)
      if (s.SD.UId == uid && s.SD.UType == utype) break;
    if (s == null) {
      // The window doesn't exist
      if ((u = Graph.locateUnit (uid, utype)) == null) return;
      // Check if the unit is on the Cancel menu -- this means that we have
      // actually requested to display it
      if (!withinMenuList (u, CANCELMENULIST)) return;
      // This is for the first time -- create the window
      x = new StatusDisplay (u, Graph);
      s = new StatusWindow ();
      s.SD = x;
      s.next = StatusWindowList;
      StatusWindowList = s;
    }
    s.SD.update (CB);
  }
  void processBlock () {
    // Reply distributor
    // CB.dump ();      // +++
    switch (CB.Command) {
      case CommandBuffer.CMD_UPDATE:
        updateStatus ();
        break;
      case CommandBuffer.CMD_ALERT:
      case CommandBuffer.CMD_GETMAP:
        AE . update (CB);
        break;
      case CommandBuffer.CMD_GETGRAPH:
        Graph . update (CB);
        break;
      default:
        AE . localAlert ("Illegal command "+CB.Command+" from SIDE");
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
          if ((sock = new Socket (hoststr, port)) == null) {
            AE.localAlert ("Failed to connect to host "+hoststr);
            return null;
          }
        } catch (UnknownHostException e) {
          AE.localAlert ("Host " + hoststr + " not found");
          return null;
        }
      } catch (IOException e) {
        AE.localAlert ("Simulator doesn't respond on "+hoststr+" " +port);
        return null;
      }
    } catch (Exception e) {
      AE.localAlert ("Security violation on socket for host "+hoststr);
      return null;
    }
    try {
      is = sock.getInputStream ();
    } catch (IOException e) {
      AE.localAlert ("Cannot get socket's input stream");
    }
    try {
      OS = sock.getOutputStream ();
    } catch (IOException e) {
      AE.localAlert ("Cannot get socket's output stream");
    }
    return is;
  }
  MenuListEntry menuAction (MenuListEntry m, MenuListEntry list) {
    // Locates the menu triggering the action
    while (list != null) {
      if (m == list) return list;
      list = list.next;
    }
    return null;
  }
  public void actionPerformed (ActionEvent e) {
    MenuListEntry m, tm;
    tm = ActionMenu;
    ActionMenu = null;
    // Go through menus
    if ((m = menuAction (tm, DisplayMenuList)) != null) {
      doDisplay (m);
      return;
    }
    if ((m = menuAction (tm, CancelMenuList)) != null) {
      doCancel (m, null);
      return;
    }
    if ((m = menuAction (tm, OverrideMenuList)) != null) {
      doOverride (m);
      return;
    }
  }
  class LocalWindowAdapter extends WindowAdapter {
    public void windowClosing (WindowEvent ev) {
      // To intercept destroy event
      OP.Quit = true;
      OP.Disconnect = true;
    }
  }
  OperatorWindow (Conveyor op) {
    InputStream ss;
    OP = op;
    setLayout (new BorderLayout ());
    DisplayPanel = new Panel ();
    DisplayMenu = new Menu ("Display");
    CancelMenu = new Menu ("Cancel");
    OverrideMenu = new Menu ("Override");
    Graph = new ConvPanel (this);
    AE = new AlertArea (this);
    DisplayPanel . setLayout (new GridLayout (1, 2));
    DisplayPanel . add (Graph);
    DisplayPanel . add (AE);
    add ("Center", DisplayPanel);
    Menus = new MenuBar ();
    Menus.add (DisplayMenu);
    Menus.add (CancelMenu);
    Menus.add (OverrideMenu);
    setMenuBar(Menus);
    OV = new OverrideDialog ();
    this.pack ();
    this.show ();
    (this).setSize (OWWIDTH, OWHEIGHT);
    validate ();
    ss = openSocket ();
    if (ss == null) {
      // Connect failed
      try {
        Thread.sleep (2000);
      } catch (InterruptedException e) { }
      CB = null;
      (this).setVisible (false);
    } else {
      CB = new CommandBuffer (ss);
      doGetMap (); // This should be called only once
    }
    addWindowListener (new LocalWindowAdapter ());
  }
  void runRead () {
    // This function interprets stuff arriving from the SIDE program
    while (OP.ReadThread == Thread.currentThread ()) {
      if (CB.readBlock () < 0) {
        System.out.println ("SIDE disconnected");
        OP.ReadThread = null;
        return;
      }
      processBlock ();
    }
  }
  void runCommand () {
    // Display event loop
    while (OP.CommandThread == Thread.currentThread ()) {
      if (OP.Quit) {
        doStop (OP.Disconnect);
        OP.Quit = false;
      }
      Graph.display ();
      displayStatusWindows ();
      if (WaitingForDialog != null) {
        if (OV.waitForReply ()) {
          if (!OV.Cancel) sendOverride (WaitingForDialog);
          WaitingForDialog = null;
        }
      }
      try {
        Thread.sleep(MAINLOOPDELAY);
      } catch (InterruptedException e) {
        break;
      }
    }
  }
}
class OverrideDialog extends Frame implements ActionListener {
  // We don't really want this to be a formal Dialog, because there is some
  // controversy as to whether an applet running within its browser's window
  // can have a dialog. We will be keeping this window around only making it
  // visible/invisible as needed, which seems to be making more sense in this
  // particular case anyway.
  final static int DIALOGWIDTH = 588;
  final static int DIALOGHEIGHT = 80;
  // Actions (borrowed from sensor.h)
  final static int OVR_MOTOR_CNTRL = 1;
  final static int OVR_SET_COUNT = 2;
  final static int OVR_CLEAR = 3;
  final static int OVR_HOLD = 254;
  final static int OVR_RESUME = 255;
  Button ProceedButton, CancelButton;
  TextField ValueField;
  Choice ActionChoice;
  Panel MyPanel;
  boolean Proceed, // Proceed flag (for waiting only)
          Cancel; // Cancel flag
  int Action, Value;
  OverrideDialog () {
    setLayout (new GridLayout (1,1)); // Just one item -- the panel
    MyPanel = new Panel ();
    MyPanel.setLayout (new GridLayout (1, 4));
    ProceedButton = new Button ("Apply");
    CancelButton = new Button ("Cancel");
    ValueField = new TextField ("    0");
    ActionChoice = new Choice ();
    // Create action choice items
    ActionChoice.addItem ("Resume");
    ActionChoice.addItem ("Hold");
    ActionChoice.addItem ("Motor ON");
    ActionChoice.addItem ("Motor OFF");
    ActionChoice.addItem ("Clear");
    ActionChoice.addItem ("Set count");
    MyPanel.add (ProceedButton);
    MyPanel.add (CancelButton);
    MyPanel.add (ActionChoice);
    MyPanel.add (ValueField);
    this.setTitle ("Override dialog");
    this.add (MyPanel);
    this.pack ();
    (this).setSize (DIALOGWIDTH, DIALOGHEIGHT);
    // Do not show it yet
    ProceedButton.addActionListener (this);
    CancelButton.addActionListener (this);
    addWindowListener (new LocalWindowAdapter ());
  }
  synchronized public boolean waitForReply () {
    String s;
    if (!Proceed) return false;
    if (!Cancel) {
      // Set the values of Value and Action
      s = (ValueField.getText ()).trim ();
      try {
        Value = Integer . valueOf (s) . intValue ();
      } catch (NumberFormatException e) {
        Value = 0;
      }
      Action = ActionChoice.getSelectedIndex ();
      // Convert action to standard value
      switch (Action) {
        case 0:
          Action = OVR_RESUME;
          break;
        case 1:
          Action = OVR_HOLD;
          break;
        case 2:
          Action = OVR_MOTOR_CNTRL;
          Value = StatusDisplay.MOTOR_ON;
          break;
        case 3:
          Action = OVR_MOTOR_CNTRL;
          Value = StatusDisplay.MOTOR_OFF;
          break;
        case 4:
          Action = OVR_CLEAR;
          break;
        case 5:
          Action = OVR_SET_COUNT;
          break;
        default:
          Action = OVR_RESUME; // This cannot happen (or so I think)
      }
    }
    (this).setVisible (false);
    return true;
  }
  class LocalWindowAdapter extends WindowAdapter {
    public void windowClosing (WindowEvent ev) {
      // To intercept destroy event
      Cancel = Proceed = true;
    }
  }
  public void actionPerformed (ActionEvent e) {
    if (e.getActionCommand () == ProceedButton.getLabel ()) {
      Proceed = true;
    } else if (e.getActionCommand () == CancelButton.getLabel ()) {
      Cancel = true;
      Proceed = true;
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
      MTracker.waitForAll ();
    } catch (InterruptedException e) {
    }
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
class BoxDesc {
  int distance, length, type, aux;
  // Note: this is used not only for boxes, but also for some other stuff,
  // like box counts per traffic pattern
}
class StatusDisplay extends Frame {
  final static int SEGWIDTH = 900; // Window default sizes
  final static int SEGHIGHT = 100;
  final static int MERWIDTH = 600;
  final static int MERHIGHT = 100;
  final static int DIVWIDTH = 174;
  final static int DIVHIGHT = 100;
  final static int SRCWIDTH = 360;
  final static int SRCHIGHT = 128;
  final static int SNKWIDTH = 200;
  final static int SNKHIGHT = 128;
  // Windows'95
  //final static int TOFFSET      =  0;    // Top margin
  //final static int BOFFSET      = 40;    // Bottom margin
  // X (Unix)
  final static int TOFFSET = 40; // Top margin
  final static int BOFFSET = 0; // Bottom margin
  final static int INITBXSIZE = 64; // Initial size of the boxes array
  final static int SEGENTRYSIZE = 10; // Borrowed from operator.h
  final static int SNENTRYSIZE = 8; // Sink update entry
  final static int SRENTRYSIZE = 16; // Source update entry
  final static int SENSOR_ON = 0; // Borrowed from sensor.h
  final static int SENSOR_OFF = 1;
  final static int MOTOR_ON = 1;
  final static int MOTOR_OFF = 0;
  // Fraction of the segment window used for top and bottom stripes
  // representing sensor/actuator status and belt legend
  final static double SEGSTRIPRATIO = 0.30;
  final static double CCOLSIZERATIO = 0.7;
  final static Color STATUSON = Color.red;
  final static Color STATUSOFF = new Color (238, 216, 174);
  final static Color SEGBOTTOM = Color.yellow;
  final static Color SEGBELT = Color.lightGray;
  final static Color SSTATHDR = Color.yellow; // Sink/Source status hdr
  final static Color SSTATLINE = Color.lightGray; // and line
  // Assignment of colors to box types (mod 6)
  final static Color BOXTYPES [] = {Color.blue, Color.cyan, Color.green,
                                      Color.magenta, Color.orange, Color.pink};
  ConvPanel Graph;
  int UId, UType;
  Unit U;
  Image OffScreenImage; // These are the same as for Graph
  Dimension OffScreenSize;
  Graphics OffScreenGraphics;
  FontMetrics CurrentFontMetrics;
  int NBoxes; // The number of boxes on the belt
  double EELength; // End-to-end length
  boolean S1, S12, M1, S2; // Sensor and motor status
  BoxDesc Boxes [];
  StatusDisplay (Unit u, ConvPanel graph) {
    int wi, hi;
    U = u;
    UId = u.UId;
    UType = u.UType;
    Graph = graph;
    // Setup the window
    this.pack();
    this.setTitle (U.Label);
    Boxes = null;
    S1 = S12 = M1 = S2 = false;
    // Determine the window size
    switch (UType) {
      case ConvPanel.SEGMENT:
      case ConvPanel.SSEGMENT:
        wi = SEGWIDTH;
        hi = SEGHIGHT;
        // Allocate the array for boxes
        NBoxes = 0;
        Boxes = new BoxDesc [INITBXSIZE];
        break;
      case ConvPanel.MERGER:
      case ConvPanel.SMERGER:
        wi = MERWIDTH;
        hi = MERHIGHT;
        NBoxes = 0;
        Boxes = new BoxDesc [INITBXSIZE];
        break;
      case ConvPanel.DIVERTER:
      case ConvPanel.SDIVERTER:
        wi = DIVWIDTH;
        hi = DIVHIGHT;
        break;
      case ConvPanel.SOURCE:
        wi = SRCWIDTH;
        hi = SRCHIGHT;
        NBoxes = 0;
        // Will use this to store traffic status
        Boxes = new BoxDesc [INITBXSIZE/8];
        break;
      case ConvPanel.SINK:
        wi = SNKWIDTH;
        hi = SNKHIGHT;
        NBoxes = 0;
        // Will use this to store traffic status
        Boxes = new BoxDesc [INITBXSIZE/8];
        break;
      default:
        wi = 0; hi = 0; // Will never get here
    }
    (this).setSize (wi, hi);
    this.show ();
    OffScreenImage = null;
    addWindowListener (new LocalWindowAdapter ());
  }
  public void destruct () {
    this.dispose ();
  }
  class LocalWindowAdapter extends WindowAdapter {
    public void windowClosing (WindowEvent ev) {
      // To intercept destroy event
      StatusWindow s;
      for (s = Graph.OP.StatusWindowList; s != null; s = s.next)
        if (s.SD == StatusDisplay.this) break;
      if (s != null) Graph.OP.doCancel (null, s);
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
    }
  }
  public synchronized void update (CommandBuffer CB) {
    switch (UType) {
      // Well, perhaps we need a separate subtype for each Unit type,
      // but let's keep this simple for a while as we still don't know
      // how many types we need
      case ConvPanel.SEGMENT:
      case ConvPanel.SSEGMENT:
        updateSegment (CB, false);
        break;
      case ConvPanel.DIVERTER:
      case ConvPanel.SDIVERTER:
        updateDiverter (CB);
        break;
      case ConvPanel.MERGER:
      case ConvPanel.SMERGER:
        // Merger looks almost the same as segment
        updateSegment (CB, true);
        break;
      case ConvPanel.SOURCE:
        updateSource (CB);
        break;
      case ConvPanel.SINK:
        updateSink (CB);
        break;
    }
    repaint ();
  }
  void updateSegment (CommandBuffer CB, boolean Merger) {
    // Update segment status
    BoxDesc tmp [], b;
    int i, eet;
    M1 = (CB.getByte () != MOTOR_OFF); // Motor and sensors
    S1 = (CB.getByte () != SENSOR_OFF);
    if (Merger)
      S12 = (CB.getByte () != SENSOR_OFF);
    S2 = (CB.getByte () != SENSOR_OFF);
    eet = CB.getLong ();
    EELength = (double) eet; // We need this to be double
    // Now get the boxes
    for (NBoxes = 0; CB.more () >= SEGENTRYSIZE; NBoxes++) {
      if (NBoxes == Boxes.length) {
        // We have exhausted the array
        tmp = Boxes;
        Boxes = new BoxDesc [tmp.length + tmp.length];
        for (i = 0; i < tmp.length; i++) Boxes [i] = tmp [i];
        tmp = null;
      }
      if ((b = Boxes [NBoxes]) == null)
        b = Boxes [NBoxes] = new BoxDesc ();
      b.distance = CB.getLong ();
      b.length = CB.getLong ();
      b.type = CB.getShort ();
    }
  }
  void updateDiverter (CommandBuffer CB) {
    // Update diverter status
    M1 = (CB.getByte () != MOTOR_OFF); // Motor and sensors
    S1 = (CB.getByte () == SENSOR_ON);
    S2 = (CB.getByte () == SENSOR_ON);
  }
  void updateSource (CommandBuffer CB) {
    // Update source status
    BoxDesc tmp [], b;
    int i;
    // Get the queue sizes
    for (NBoxes = 0; CB.more () >= SRENTRYSIZE; NBoxes++) {
      if (NBoxes == Boxes.length) {
        // We have exhausted the array
        tmp = Boxes;
        Boxes = new BoxDesc [tmp.length + tmp.length];
        for (i = 0; i < tmp.length; i++) Boxes [i] = tmp [i];
        tmp = null;
      }
      if ((b = Boxes [NBoxes]) == null)
        b = Boxes [NBoxes] = new BoxDesc ();
      b.distance = CB.getLong (); // Xmitted boxes
      b.length = CB.getLong (); // Xmitted centimetres
      b.type = CB.getLong (); // Queued boxes
      b.aux = CB.getLong (); // Queued centimetres
    }
  }
  void updateSink (CommandBuffer CB) {
    // Update sink status
    BoxDesc tmp [], b;
    int i;
    // Get the queue sizes
    for (NBoxes = 0; CB.more () >= SNENTRYSIZE; NBoxes++) {
      if (NBoxes == Boxes.length) {
        // We have exhausted the array
        tmp = Boxes;
        Boxes = new BoxDesc [tmp.length + tmp.length];
        for (i = 0; i < tmp.length; i++) Boxes [i] = tmp [i];
        tmp = null;
      }
      if ((b = Boxes [NBoxes]) == null)
        b = Boxes [NBoxes] = new BoxDesc ();
      b.distance = CB.getLong ();
      b.length = CB.getLong ();
    }
  }
  public synchronized void update (Graphics g) { // The painting function
    setupGraphics ();
    OffScreenGraphics.setColor (getBackground ());
    OffScreenGraphics.fillRect (0, 0, OffScreenSize.width,
                                                          OffScreenSize.height);
    switch (UType) {
      case ConvPanel.SEGMENT:
      case ConvPanel.SSEGMENT:
        paintSegment (OffScreenGraphics, false);
        break;
      case ConvPanel.MERGER:
      case ConvPanel.SMERGER:
        // Merger looks almost the same as segment
        paintSegment (OffScreenGraphics, true);
        break;
      case ConvPanel.DIVERTER:
      case ConvPanel.SDIVERTER:
        paintDiverter (OffScreenGraphics);
        break;
      case ConvPanel.SOURCE:
        paintSource (OffScreenGraphics);
        break;
      case ConvPanel.SINK:
        paintSink (OffScreenGraphics);
    }
    g.drawImage (OffScreenImage, 0, 0, null);
  }
  void paintSegment (Graphics g, boolean Merger) {
    int i, dst, th, xw, x0, y0, x1, y1, xs, ys, xd, yd, tw, as;
    String s;
    double dd;
    x0 = 0;
    y0 = TOFFSET;
    x1 = OffScreenSize.width;
    y1 = OffScreenSize.height - BOFFSET;
    if (y1 < y0 + 6) return; // Don't bother
    // Sensor/actuator status
    th = CurrentFontMetrics.getHeight ();
    as = CurrentFontMetrics.getAscent ();
    xd = (x1 - x0) / (Merger ? 4 : 3);
    yd = (int) (SEGSTRIPRATIO * (y1 - y0));
    ys = y0;
    xs = x0;
    g.setColor (S1 ? STATUSON : STATUSOFF);
    g.fillRect (xs, ys, xd, yd);
    g.setColor (Color.black);
    g.drawRect (xs, ys, xd, yd);
    if (Merger) {
      // Here goes the second entry sensor
      xs += xd;
      xd = (x1 - xs) / 3;
      g.setColor (S12 ? STATUSON : STATUSOFF);
      g.fillRect (xs, ys, xd, yd);
      g.setColor (Color.black);
      g.drawRect (xs, ys, xd, yd);
    }
    xs += xd;
    xd = (x1 - xs) / 2;
    g.setColor (M1 ? STATUSON : STATUSOFF);
    g.fillRect (xs, ys, xd, yd);
    g.setColor (Color.black);
    g.drawRect (xs, ys, xd, yd);
    xs += xd;
    xd = x1 - xs;
    g.setColor (S2 ? STATUSON : STATUSOFF);
    g.fillRect (xs, ys, xd, yd);
    g.setColor (Color.black);
    g.drawRect (xs, ys, xd, yd);
    if (th < yd) {
      // We can afford to insert a description of what we are painting
      xs = x0;
      if (Merger) {
        xd = (x1 - x0) / 4;
        s = "In1";
      } else {
        xd = (x1 - x0) / 3;
        s = "In";
      }
      tw = CurrentFontMetrics.stringWidth (s);
      if (tw < xd)
        g.drawString (s, xs + (xd - tw)/2, ys + yd);
      if (Merger) {
        xs += xd;
        xd = (x1 - xs) / 3;
        s = "In2";
        tw = CurrentFontMetrics.stringWidth (s);
        if (tw < xd)
          g.drawString (s, xs + (xd - tw)/2, ys + yd);
      }
      xs += xd;
      xd = (x1 - xs) / 2;
      s = "Motor";
      tw = CurrentFontMetrics.stringWidth (s);
      if (tw < xd)
        g.drawString (s, xs + (xd - tw)/2, ys + yd);
      xs += xd;
      xd = x1 - xs;
      s = "Out";
      tw = CurrentFontMetrics.stringWidth (s);
      if (tw < xd)
        g.drawString (s, xs + (xd - tw)/2, ys + yd);
    }
    ys += yd; // This is the belt area
    yd = (int) ((y1 - y0) * (1.0 - SEGSTRIPRATIO - SEGSTRIPRATIO));
    xs = x0;
    xd = (x1 - x0);
    g.setColor (SEGBELT); // Belt background
    g.fillRect (xs, ys, xd, yd);
    g.setColor (Color.black);
    g.drawRect (xs, ys, xd, yd);
    dd = ((double)xd) / EELength;
    for (i = 0; i < NBoxes; i++) { // Draw the boxes
      // Pixel distance of the box center from the left end of the belt
      dst = (int)((EELength - (double) (Boxes [i] . distance)) * dd + 0.5);
      // Pixel width of the box
      xw = (int)((double)(Boxes [i].length) * dd + 0.5);
      if (dst < xw/2) {
        xw -= (xw/2 - dst);
        dst = 0;
      } else
        dst -= xw/2;
      if (xw <= 0) xw = 2; // Make it at least 2 pixels wide
      g.setColor (BOXTYPES [Boxes [i].type % BOXTYPES.length]);
      g.fillRect (xs + dst, ys, xw, yd);
      g.setColor (Color.black);
      g.drawRect (xs + dst, ys, xw, yd);
    }
    // Now for the bottom row -- the legend
    ys += yd;
    xs = x0;
    yd = y1 - ys;
    xd = (x1 - x0);
    g.setColor (SEGBOTTOM);
    g.fillRect (xs, ys, xd, yd);
    g.setColor (Color.black);
    g.drawRect (xs, ys, xd, yd);
    s = "Length: "+(EELength/1000000.0)+"m";
    tw = CurrentFontMetrics.stringWidth (s);
    if ((dst = xd - tw) > 0) {
      g.drawString (s, xs + (xd - tw)/2, xw = ys + as);
      if (dst > CurrentFontMetrics.stringWidth ("F  E") + 8) {
        g.drawString ("F", xs + 4, xw);
        g.drawString ("E", xs + xd - CurrentFontMetrics.stringWidth ("E") - 4,
          xw);
      }
    }
  }
  void paintDiverter (Graphics g) {
    int th, x0, y0, x1, y1, xs, ys, xd, yd, tw, as;
    String s;
    x0 = 0;
    y0 = TOFFSET;
    x1 = OffScreenSize.width;
    y1 = OffScreenSize.height-BOFFSET;
    // Sensor/actuator status
    th = CurrentFontMetrics.getHeight ();
    as = CurrentFontMetrics.getAscent ();
    xd = (x1 - x0) / 3;
    yd = y1 - y0;
    ys = y0;
    xs = x0;
    g.setColor (S1 ? STATUSON : STATUSOFF);
    g.fillRect (xs, ys, xd, yd);
    g.setColor (Color.black);
    g.drawRect (xs, ys, xd, yd);
    xs += xd;
    xd = (x1 - xs) / 2;
    g.setColor (S2 ? STATUSON : STATUSOFF);
    g.fillRect (xs, ys, xd, yd);
    g.setColor (Color.black);
    g.drawRect (xs, ys, xd, yd);
    xs += xd;
    xd = x1 - xs;
    g.setColor (M1 ? STATUSON : STATUSOFF);
    g.fillRect (xs, ys, xd, yd);
    g.setColor (Color.black);
    g.drawRect (xs, ys, xd, yd);
    if (th < yd) {
      // We can afford to insert a legend
      xs = x0;
      xd = (x1 - x0) / 3;
      s = "Full";
      tw = CurrentFontMetrics.stringWidth (s);
      if (tw < xd)
        g.drawString (s, xs + (xd - tw)/2, ys + yd - as);
      xs += xd;
      xd = (x1 - xs) / 2;
      s = "Reader";
      tw = CurrentFontMetrics.stringWidth (s);
      if (tw < xd)
        g.drawString (s, xs + (xd - tw)/2, ys + yd - as);
      xs += xd;
      xd = x1 - xs;
      s = "Motor";
      tw = CurrentFontMetrics.stringWidth (s);
      if (tw < xd)
        g.drawString (s, xs + (xd - tw)/2, ys + yd - as);
    }
  }
  void paintSource (Graphics g) {
    int nr, th, x0, y0, x1, y1, xs, ys, yd, ytoff, tw, as, i;
    int TWidth, NWidth, CWidth, RWidth;
    BoxDesc b;
    String s;
    // This is going to look like a table
    x0 = 0;
    y0 = TOFFSET;
    x1 = OffScreenSize.width;
    y1 = OffScreenSize.height-BOFFSET;
    th = CurrentFontMetrics.getHeight ();
    as = CurrentFontMetrics.getAscent ();
    // Determine the maximum number of text rows that can fit the window
    nr = (y1 - y0 - 2) / (th + 4) - 1; // One reserved for the header
    if (nr > NBoxes) nr = NBoxes; // The number we need
    if (nr == 0) return; // Don't bother
    // Calculate the row width
    RWidth = (y1 - y0) / (nr + 1);
    // Calculate the width of the columns
    s = "T";
    if (nr > 9) s = "T0";
    TWidth = CurrentFontMetrics.stringWidth (s) + 12;
    NWidth = (x1 - x0 - TWidth) / 2;
    if (NWidth < 42) return; // Don't bother
    CWidth = (int) ((double) NWidth / (1.0 + CCOLSIZERATIO));
    NWidth -= CWidth;
    xs = x0;
    ys = y0;
    // Display the headers
    yd = (y1 - y0) - nr * RWidth;
    g.setColor (SSTATHDR);
    g.fillRect (x0, y0, x1, yd);
    g.setColor (Color.black);
    s = "T";
    tw = CurrentFontMetrics.stringWidth (s);
    g.drawString (s, xs + (TWidth - tw)/2, ys + yd - as);
    xs += TWidth;
    s = "XmB";
    tw = CurrentFontMetrics.stringWidth (s);
    g.drawString (s, xs + (NWidth - tw)/2, ys + yd - as);
    xs += NWidth;
    s = "XmBCm";
    tw = CurrentFontMetrics.stringWidth (s);
    g.drawString (s, xs + (CWidth - tw)/2, ys + yd - as);
    xs += CWidth;
    s = "QuB";
    tw = CurrentFontMetrics.stringWidth (s);
    g.drawString (s, xs + (NWidth - tw)/2, ys + yd - as);
    xs += NWidth;
    s = "QuBCm";
    tw = CurrentFontMetrics.stringWidth (s);
    g.drawString (s, xs + (CWidth - tw)/2, ys + yd - as);
    ys += yd;
    g.setColor (SSTATLINE);
    g.fillRect (x0+TWidth, ys+1, x1 - TWidth - x0, y1 - ys - 1);
    g.setColor (Color.black);
    g.drawLine (x0, ys-1, x1, ys-1);
    ytoff = (RWidth - th) / 2 + as; // Text offset (vertical)
    // Now go through the traffic patterns
    for (i = 0; i < NBoxes; i++) {
      // Also draw horizontal lines
      g.setColor (Color.black);
      g.drawLine (x0, ys, x1, ys);
      b = Boxes [i];
      xs = x0;
      g.setColor (BOXTYPES [i % BOXTYPES.length]);
      g.fillRect (x0, ys+1, TWidth, RWidth-1);
      g.setColor (Color.black);
      s = String.valueOf (i); // Traffic pattern number
      tw = CurrentFontMetrics.stringWidth (s);
      // All numbers are right aligned
      g.drawString (s, xs + TWidth - tw - 4, ys + ytoff);
      xs += TWidth;
      s = String.valueOf (b.distance); // Xmitted boxes
      tw = CurrentFontMetrics.stringWidth (s);
      g.drawString (s, xs + NWidth - tw - 4, ys + ytoff);
      xs += NWidth;
      s = String.valueOf (b.length); // Xmitted centimetres
      tw = CurrentFontMetrics.stringWidth (s);
      g.drawString (s, xs + CWidth - tw - 4, ys + ytoff);
      xs += CWidth;
      s = String.valueOf (b.type); // Queued boxes
      tw = CurrentFontMetrics.stringWidth (s);
      g.drawString (s, xs + NWidth - tw - 4, ys + ytoff);
      xs += NWidth;
      s = String.valueOf (b.aux); // Queued centimetres
      tw = CurrentFontMetrics.stringWidth (s);
      g.drawString (s, xs + CWidth - tw - 8, ys + ytoff);
      ys += RWidth;
    }
    // Draw the last horizontal line
    g.drawLine (x0, ys-1, x1, ys-1);
    // And draw the vertical lines
    xs = x0+TWidth;
    g.drawLine (xs, y0, xs, y1);
    g.drawLine (xs-1, y0, xs-1, y1);
    xs += NWidth; g.drawLine (xs, y0, xs, y1);
    xs += CWidth; g.drawLine (xs, y0, xs, y1);
    xs += NWidth; g.drawLine (xs, y0, xs, y1);
    xs = x1-1; g.drawLine (xs, y0, xs, y1);
  }
  void paintSink (Graphics g) {
    int nr, th, x0, y0, x1, y1, xs, ys, yd, ytoff, tw, as, i;
    int TWidth, NWidth, CWidth, RWidth;
    BoxDesc b;
    String s;
    // Another table
    x0 = 0;
    y0 = TOFFSET;
    x1 = OffScreenSize.width;
    y1 = OffScreenSize.height-BOFFSET;
    th = CurrentFontMetrics.getHeight ();
    as = CurrentFontMetrics.getAscent ();
    // Determine the maximum number of text rows that can fit the window
    nr = (y1 - y0 - 2) / (th + 4) - 1; // One reserved for the header
    if (nr > NBoxes) nr = NBoxes; // The number we need
    if (nr == 0) return; // Don't bother
    // Calculate the row width
    RWidth = (y1 - y0) / (nr + 1);
    // Calculate the width of the columns
    s = "T";
    if (nr > 9) s = "T0";
    TWidth = CurrentFontMetrics.stringWidth (s) + 12;
    NWidth = x1 - x0 - TWidth;
    if (NWidth < 42) return; // Don't bother
    CWidth = (int) ((double) NWidth / (1.0 + CCOLSIZERATIO));
    NWidth -= CWidth;
    xs = x0;
    ys = y0;
    // Display the headers
    yd = (y1 - y0) - nr * RWidth;
    g.setColor (SSTATHDR);
    g.fillRect (x0, y0, x1, yd);
    g.setColor (Color.black);
    s = "T";
    tw = CurrentFontMetrics.stringWidth (s);
    g.drawString (s, xs + (TWidth - tw)/2, ys + yd - as);
    xs += TWidth;
    s = "RcB";
    tw = CurrentFontMetrics.stringWidth (s);
    g.drawString (s, xs + (NWidth - tw)/2, ys + yd - as);
    xs += NWidth;
    s = "RcBCm";
    tw = CurrentFontMetrics.stringWidth (s);
    g.drawString (s, xs + (CWidth - tw)/2, ys + yd - as);
    ys += yd;
    g.setColor (SSTATLINE);
    g.fillRect (x0+TWidth, ys+1, x1 - TWidth - x0, y1 - ys - 1);
    g.setColor (Color.black);
    g.drawLine (x0, ys-1, x1, ys-1);
    ytoff = (RWidth - th) / 2 + as; // Text offset (vertical)
    // Now go through the traffic patterns
    for (i = 0; i < NBoxes; i++) {
      // Also draw horizontal lines
      g.setColor (Color.black);
      g.drawLine (x0, ys, x1, ys);
      b = Boxes [i];
      xs = x0;
      g.setColor (BOXTYPES [i % BOXTYPES.length]);
      g.fillRect (x0, ys+1, TWidth, RWidth-1);
      g.setColor (Color.black);
      s = String.valueOf (i); // Traffic pattern number
      tw = CurrentFontMetrics.stringWidth (s);
      // All numbers are right aligned
      g.drawString (s, xs + TWidth - tw - 4, ys + ytoff);
      xs += TWidth;
      s = String.valueOf (b.distance); // Received boxes
      tw = CurrentFontMetrics.stringWidth (s);
      g.drawString (s, xs + NWidth - tw - 4, ys + ytoff);
      xs += NWidth;
      s = String.valueOf (b.length); // Received centimetres
      tw = CurrentFontMetrics.stringWidth (s);
      g.drawString (s, xs + CWidth - tw - 8, ys + ytoff);
      ys += RWidth;
    }
    // Draw the last horizontal line
    g.drawLine (x0, ys-1, x1, ys-1);
    // And draw the vertical lines
    xs = x0+TWidth;
    g.drawLine (xs, y0, xs, y1);
    g.drawLine (xs-1, y0, xs-1, y1);
    xs += NWidth; g.drawLine (xs, y0, xs, y1);
    xs = x1-1; g.drawLine (xs, y0, xs, y1);
  }
}
class StatusWindow {
  StatusDisplay SD; // The window
  StatusWindow next;
}
class Unit {
  int x, y; // Center coordinates
  int UId, UType; // Unit Id and Type
  int width; // This depends on Label, height is global
  Unit NU, NDU; // Successors
  String Label; // Textual name
  boolean Head, // True if no predecessor (Source)
          Picked; // For mouse actions
}
public class Conveyor extends Applet implements Runnable {
  final static int NEYES = 3; // How many eyes we have
  RootPanel RP;
  Thread Animator = null;
  OperatorWindow OW;
  Thread ReadThread = null, CommandThread = null;
  boolean Quit = false, Disconnect = false;
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
    Disconnect = Quit = false;
    if (OW != null) return; // Things seem to be doing fine
    OW = new OperatorWindow (this);
    if (OW.CB == null) {
      OW = null;
      return; // We have failed
    }
    OW.doStart ();
    if (CommandThread == null) {
      CommandThread = new Thread (this);
      CommandThread . start ();
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
          Quit = Disconnect = true;
          break;
        case 3:
          Quit = true;
// @@@ TERMINATE @@@ //
// Remove the next line to enable the Terminate button
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
          Thread.sleep (300);
          RP.CTime += 300;
        } catch (InterruptedException e) {
          RP.CTime += 300;
        }
      }
      return;
    }
    if (OW == null) return;
    if (ReadThread == ct) {
      OW.runRead ();
      CommandThread = null;
    } else if (CommandThread == ct) {
      OW.runCommand ();
      if (ReadThread != null) {
        ReadThread.stop ();
        ReadThread = null;
      }
      // We are done with everything -- CommandThread exits second (well,
      // most of the time)
      OW.cleanStatusWindows ();
      System.out.println ("Master thread exit");
      try { OW.sock.close (); } catch (IOException e) { }
      // OW.dispose ();
      (OW).setVisible (false);
      OW = null;
    }
  }
  public void destroy () {
    Quit = true;
    Disconnect = true;
    while (OW != null) {
      try {
        Thread.sleep(100);
      } catch (InterruptedException e) {
        continue;
      }
    }
  }
}
