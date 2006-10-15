// --------------------------------------------------------- //
//                                                           //
//             DDDDD       SSSSS     DDDDD                   //
//             D    D     S          D    D                  //
//             D    D     S          D    D                  //
//             D    D      SSSS      D    D                  //
//             D    D          S     D    D                  //
//             D    D          S     D    D                  //
//             DDDDD      SSSSS      DDDDD                   //
//                                                           //
//             P.G. April 1997                               //
//                  May   1997                               //
//                  June  1997 upgrade to version 1.1.1      //
//                                                           //
// --------------------------------------------------------- //
import java.io.*;
import java.lang.*;
import java.net.*;
import java.awt.*;
import java.util.*;
import java.applet.Applet;
import java.awt.event.*;
public class DSD extends Applet implements Runnable {
  // Thread priorities
  final static int PRIORITY_R = Thread.NORM_PRIORITY; // Read thread
  final static int PRIORITY_D = Thread.MAX_PRIORITY; // Display thread
  final static int PRIORITY_B = Thread.MIN_PRIORITY; // Background
  // Banner picture parameters
  final static int NEYES = 2; // How many eyes we have
  final static int EMASK = 3; // Mask for the number of frames
  final static int TWIDTH = 507; // Picture width
  final static int THEIGHT = 245; // and height
  final static int EWIDTH = 66; // Eye width
  final static int EHEIGHT = 63; // Eye height
  final static int VINSET = 90; // Vertical inset for the eyes
  final static int HINSET = 90; // Horizontal inset
  final static String TFILE = "dsdstart.gif";
  final static String EFILES [] = {"eye1.gif", "eye2.gif",
                                     "eye3.gif", "eye4.gif",
                                     "eyeC.gif"};
  final static int MINDELAY = 300;
  final static int DELAYMASK = 1023;
  // Monitor host and port
  String Host; int Port;
  RootWindow RW;
  Thread ReceiverThread = null, DisplayThread = null, AbortThread = null;
  int MouseEye;
  Image Title, Eyes [], CurrEye [];
  int NFrames, XCor [], YCor [];
  long CTime, UpdateTime [];
  Image OSI;
  Graphics OSG;
  Thread Animator = null;
  Random RNG;
  Rectangle StartRect, StopRect;
  MediaTracker MTracker;
  public void init () {
    String portstr;
    URL url;
    int i;
    // Get the TCP/IP parameters
    Host = getParameter ("host");
    portstr = getParameter ("port");
    if (Host == null) Host = "sheerness.cs.ualberta.ca";
    if (portstr == null) portstr = "8482";
    Port = Integer.parseInt (portstr);
    MouseEye = -1;
    Eyes = new Image [(NFrames = EFILES.length - 1) + 1];
    CurrEye = new Image [NEYES];
    UpdateTime = new long [NEYES];
    for (i = 0; i < NEYES; i++) UpdateTime [i] = 0;
    XCor = new int [NEYES];
    YCor = new int [NEYES];
    CTime = 0;
    RNG = new Random ();
    try {
      url = new URL (getDocumentBase (), TFILE);
    } catch (MalformedURLException e) {
      url = null;
    }
    Title = getImage (url);
    MTracker = new MediaTracker (this);
    MTracker.addImage (Title, 0); // Preload everything before displaying
    for (i = 0; i <= NFrames; i++) {
      try {
        url = new URL (getDocumentBase (), EFILES [i]);
      } catch (MalformedURLException e) {
        url = null;
      }
      Eyes [i] = getImage (url);
      MTracker.addImage (Eyes [i], 0);
    }
    for (i = 0; i < NEYES; i++) CurrEye [i] = Eyes [0];
    // Calculate positions for the eyes
    XCor [0] = HINSET;
    YCor [0] = VINSET;
    XCor [1] = TWIDTH - HINSET - EWIDTH;
    YCor [1] = VINSET;
    OSI = createImage (TWIDTH, THEIGHT);
    OSG = OSI.getGraphics ();
    StartRect = new Rectangle (XCor [0], YCor [0], EWIDTH, EHEIGHT);
    StopRect = new Rectangle (XCor [1], YCor [1], EWIDTH, EHEIGHT);
    validate ();
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
  public synchronized void update (Graphics g) {
    int i;
    OSG.drawImage (Title, 0, 0, TWIDTH, THEIGHT, null);
    for (i = 0; i < NEYES; i++)
      OSG.drawImage (CurrEye [i], XCor [i], YCor [i], EWIDTH, EHEIGHT, null);
    g.drawImage (OSI, 0, 0, null);
  }
  class LocalMouseAdapter extends MouseAdapter {
    public void mousePressed (MouseEvent e) {
      int x = e.getX (), y = e.getY ();
      if (StartRect.contains (x, y)) {
        CurrEye [MouseEye = 0] = Eyes [NFrames];
        repaint ();
        return;
      } else if (StopRect.contains (x, y)) {
        CurrEye [MouseEye = 1] = Eyes [NFrames];
        repaint ();
        return;
      }
      return;
    }
    public void mouseReleased (MouseEvent e) {
      int x = e.getX (), y = e.getY ();
      boolean found = false;
      if (StartRect.contains (x, y)) {
        if (MouseEye == 0) {
          startup ();
          found = true;
        }
      } else if (StopRect.contains (x, y)) {
        if (MouseEye == 1) {
          abort ("DSD Terminated");
          found = true;
        }
      }
      if (MouseEye != -1) {
        CurrEye [MouseEye] = Eyes [0];
        MouseEye = -1;
      }
    }
  }
  public void abort (String s) {
    if (RW != null && RW.Alerts != null) {
      // This will be the case, of course
      RW.Alerts.displayLine (s);
      RW.disableMenus ();
      // We need a special thread for this, because abort may be called from
      // a synchronised method of RootWindow
      AbortThread = new Thread (this);
      AbortThread . start ();
    }
  }
  synchronized void startup () {
    if (RW != null) return; // We are already running
    ReceiverThread = null; // Just in case
    RW = new RootWindow (this); // This cannot possibly fail
  }
  public synchronized void stopReceiverThread () {
    if (ReceiverThread != null) {
      ReceiverThread.stop ();
      ReceiverThread = null;
    }
  }
  public synchronized void stopDisplayThread () {
    if (DisplayThread != null) {
      DisplayThread.stop ();
      DisplayThread = null;
    }
  }
  public synchronized void startReceiverThread () {
    if (ReceiverThread != null)
      // This shouldn't really happen
      stopReceiverThread ();
    ReceiverThread = new Thread (this);
    ReceiverThread . start ();
  }
  public synchronized void startDisplayThread () {
    DisplayThread = new Thread (this);
    DisplayThread . start ();
  }
  void terminate () {
    if (RW == null) return;
    stopReceiverThread ();
    stopDisplayThread ();
    // Remove all windows present on the screen
    RW.terminateDisplaySession ();
    RW.deleteDisplayWindows ();
    // Make sure the socket is closed
    RW.SI.closeSocket ();
    (RW).setVisible (false);
    // Deallocate the root window
    RW = null;
  }
  public void run () {
    Thread ct = Thread.currentThread ();
    if (Animator == ct) {
      ct.setPriority (PRIORITY_B);
      try {
        // Wait for the images to be downloaded
        MTracker.waitForAll ();
      } catch (InterruptedException e) {
      }
      while (Animator == ct) {
        // Check update times
        for (int i = 0; i < NEYES; i++) {
          if (i != MouseEye && UpdateTime [i] <= CTime) {
            int rand = RNG.nextInt ();
            int ne = (int)((rand >> 6) & EMASK);
            int nd = (int)((rand >> 10) & DELAYMASK);
            CurrEye [i] = Eyes [ne];
            UpdateTime [i] = CTime + nd;
          }
        }
        repaint ();
        try {
          Thread.sleep (300);
          CTime += 300;
        } catch (InterruptedException e) {
          CTime += 300;
        }
      }
      return;
    }
    if (RW == null) return;
    if (DisplayThread == ct) {
      ct.setPriority (PRIORITY_D);
      RW.runDisplay ();
    } else if (ReceiverThread == ct) {
      ct.setPriority (PRIORITY_R);
      RW.runReceiver ();
    } else if (AbortThread == ct) {
      try {
        // Sleep for 2 seconds to give the user a chance to read the message
        Thread.sleep (2000);
      } catch (InterruptedException e) {
      }
      terminate ();
    }
  }
}
// ------------------------------------------------------------------------- //
class SInterface {
  // This class represents the I/O interface to the simulator. We need it to
  // provide a monitor for synchronizing some critical operations.
  Socket MSocket;
  OutputBuffer OB; InputBuffer IB;
  RootWindow RW;
  DSD Main;
  AlertArea Alerts;
  public SInterface (RootWindow rw) {
    Main = (RW = rw) . Main;
    Alerts = RW.Alerts;
    OB = new OutputBuffer ();
    IB = new InputBuffer ();
    MSocket = null;
  }
  public synchronized boolean openSocket () {
    // This function opens a socket to the monitor and associates it with
    // the buffers
    InputStream is = null;
    String Host = Main.Host; int Port = Main.Port;
    if (MSocket != null) return true;
    try {
      try {
        try {
          if ((MSocket = new Socket (Host, Port)) == null) {
            Alerts.displayLine ("Failed to connect to host "+Host);
            return true;
          }
        } catch (UnknownHostException e) {
          Main.abort ("Host " + Host + " not found");
          MSocket = null;
          return true;
        }
      } catch (IOException e) {
        Main.abort ("Monitor doesn't respond on "+Host+" " +Port);
        MSocket = null;
        return true;
      }
    } catch (Exception e) {
      Main.abort ("Security violation on socket for host "+Host);
      MSocket = null;
      return true;
    }
    OB.connect (MSocket);
    IB.connect (MSocket);
    if (IB.error || OB.error) {
      OB.disconnect ();
      IB.disconnect ();
      // This isn't very likely, I guess
      Main.abort ("Cannot create monitor stream");
      try { MSocket.close (); } catch (Exception e) { }
      MSocket = null;
      return true;
    }
    return false;
  }
  public synchronized void closeSocket () {
    OB.disconnect ();
    IB.disconnect ();
    try { MSocket.close (); } catch (Exception e) { }
    MSocket = null;
  }
}
// ------------------------------------------------------------------------- //
class Taster implements ActionListener {
  // A menu item
  Menu menu;
  MenuItem item;
  int Action;
  DisplayWindow wind;
  Taster next;
  RootWindow TheRoot;
  public Taster (RootWindow RW, Menu m, MenuItem mi, int act, DisplayWindow w) {
    Action = act;
    (menu = m).add (item = mi);
    wind = w;
    next = RW.TasterList;
    RW.TasterList = this;
    TheRoot = RW;
    mi.addActionListener (this);
  }
  public void actionPerformed (ActionEvent e) {
    if (TheRoot.MenusDisabled) return;
    TheRoot.ActionTaster = this;
    TheRoot.dispatchEvent (e);
  }
}
// ------------------------------------------------------------------------- //
class SmList {
  // This is a Smurph list item representing an experiment currently in
  // progress
  int Handle;
  SmList next;
  public SmList (RootWindow RW, int h) {
    SmList sm;
    Handle = h;
    next = null;
    if ((sm = RW.SmurphList) == null) {
      RW.SmurphList = this;
    } else {
      while (sm.next != null) sm = sm.next;
      sm.next = this;
    }
    // A new item is appended at the end of the list, so that its position
    // corresponds to the line number on the menu.
  }
}
// ------------------------------------------------------------------------- //
class SObject {
  // Represents an item of the current object list. The last object on the
  // list is the owner of the remaining ones.
  SObject next;
  String BName, SName, NName;
  public SObject (RootWindow rw) {
    SObject so;
    next = null;
    BName = SName = NName = null;
    // Added in reverse order
    next = rw.ObjectMenu;
    rw.ObjectMenu = this;
  }
}
// -----------------------------------------------------------------------------
// Template stuff; we start with field descriptions
class Field {
  // This covers all fields, including regions
  Field next; // List pointer
  byte type; // The type of this field
  public Field (int tp, Template T) {
    Field f;
    next = null;
    type = (byte)tp;
    if (T.fieldlist == null) {
      T.fieldlist = this;
    } else {
      for (f = T.fieldlist; f.next != null; f = f.next);
      f.next = this;
    }
  }
}
class FixedField extends Field {
  // Fixed contents
  byte dmode; // Display mode
  short x, y; // Starting coordinates
  String contents; // The stuff to be displayed
  public FixedField (Template T, InputBuffer IB) {
    super (Template.FT_FIXED, T);
    dmode = (byte) (IB.getOctet ());
    x = (short) (IB.getInt ());
    y = (short) (IB.getInt ());
    if (IB.getOctet () != PRT.PH_TXT) {
      type = Template.FT_ERROR;
    } else {
      contents = IB.getText ();
    }
  }
}
class FilledField extends Field {
  // Filled dynamically
  byte dmode, // Display mode
          just; // Justification (LEFT, RIGHT)
  short x, y, // Starting coordinates
          length; // The size
  public FilledField (Template T, InputBuffer IB) {
    super (Template.FT_FILLED, T);
    dmode = (byte) (IB.getOctet ());
    just = (byte) (IB.getOctet ());
    x = (short) (IB.getInt ());
    y = (short) (IB.getInt ());
    length = (short) (IB.getInt ());
  }
}
class RegionField extends Field {
  // This is the trickiest one
  short x, y, x1, y1; // Corner coordinates
  byte dmode; // Display mode
  char pc, lc; // Point/line display characters
  // Scaling attributes have been removed in the new version
  public RegionField (Template T, InputBuffer IB) {
    super (Template.FT_REGION, T);
    dmode = (byte) (IB.getOctet ());
    x = (short) (IB.getInt ());
    y = (short) (IB.getInt ());
    x1 = (short) (IB.getInt ());
    y1 = (short) (IB.getInt ());
    pc = (char) (IB.getOctet ());
    lc = (char) (IB.getOctet ());
  }
}
class RepeatField extends Field {
  // A  dummy  field  describing  replication  of a sequence of previous fields
  Field from, to;
  int count; // Replication count 0 == unlimited
  // Note: regions cannot occur within the replication boundaries
  public RepeatField (Template T, InputBuffer IB) {
    super (Template.FT_REPEAT, T);
    int i;
    int f = IB.getInt ();
    int t = IB.getInt ();
    for (i = 0, from = T.fieldlist; i < f && from != null;
                                                        i++, from = from.next);
    if (from == null)
      // This cannot happen; we try to make it sane just in case
      from = T.fieldlist;
    for (to = from; i < t && to != null; i++, to = to.next);
    if (to == null) to = T.fieldlist;
    count = IB.getInt ();
  }
}
class Template {
  final static byte LEFT = 0; // Justification for fixed fields
  final static byte RIGHT = 1;
  final static byte DM_NORMAL = 0; // Display modes
  final static byte DM_REVERSE = 1;
  final static byte DM_BLINKING = 2;
  final static byte DM_BRIGHT = 3;
  final static byte FT_FIXED = 0; // Fixed contents
  final static byte FT_REPEAT = 1; // Replication of last line
  final static byte FT_REGION = 2; // Region description
  final static byte FT_FILLED = 3; // Filled dynamically
  final static byte FT_ERROR = 4; // Special value indicating error
  // This is used in backup items to indicate that the item is in fact
  // invisible, although it falls between the bounds, e.g., because the
  // visible part of the window is narrower than the full width.
  final static byte FT_VOID = 5;
  final static byte TS_ABSOLUTE = 1; // No station-relative
  final static byte TS_RELATIVE = 2; // Mandatory relative
  final static byte TS_FLEXIBLE = 3; // Relative or not
    // The above constants used to be negative in the C++ version -- I have
    // no clue why.
  Template next, // List pointer
           snext; // Another link for selected templates
  String typeidnt; // Object type identifier
  int mode; // The exposure mode
  byte status; // How the window can be related to a station
  String desc; // Description text
  int nfields; // The number of fields
  Field fieldlist; // List of fields
  short xclipd, // Default clipping column
           yclipd; // Default clipping row
  public Template (RootWindow RW, InputBuffer IB) {
    int nf, ftype;
    Field f;
    Template t;
    fieldlist = null;
    next = null;
    // Read the template contents
    if (IB.getOctet () != PRT.PH_TXT) {
      // This is what we do in case of error. The template won't be added
      // anywhere, so the object will be treated as garbage.
      IB.ignore ();
      return;
    }
    typeidnt = IB.getText ();
    mode = IB.getInt ();
    status = (byte) (IB.getOctet ());
    if (IB.getOctet () != PRT.PH_TXT) {
      IB.ignore ();
      return;
    }
    desc = IB.getText ();
    // Both xclipd and yclipd are short by 1
    xclipd = (short) (IB.getInt () + 1);
    yclipd = (short) (IB.getInt () + 1);
    nfields = IB.getInt ();
    for (nf = 0; nf < nfields; nf++) {
      // Process next field
      ftype = IB.getOctet ();
      switch (ftype) {
        case FT_FIXED:
          f = new FixedField (this, IB);
          break;
        case FT_FILLED:
          f = new FilledField (this, IB);
          break;
        case FT_REPEAT:
          f = new RepeatField (this, IB);
          break;
        case FT_REGION:
          f = new RegionField (this, IB);
          break;
        default:
          f = null;
      }
      if (f == null || f.type == FT_ERROR) {
        IB.ignore ();
        return;
      }
    }
    // Done -- add the template at the end of the list
    if (RW.Templates == null) {
      RW.Templates = this;
    } else {
      for (t = RW.Templates; t.next != null; t = t.next);
      t.next = this;
    }
  }
}
// -----------------------------------------------------------------------------
class DisplayDialog extends Frame implements ActionListener {
  // We don't really want this to be a formal Dialog, because there is some
  // controversy as to whether an applet running within its browser's window
  // can have a dialog.
  // Window size
  final static int DWWIDTH = 440;
  final static int DWHEIGHT = 220;
  // The number of selection rows for template id's
  final static int NSELECTIONROWS = 4;
  // The background color of the selection list
  final static Color LISTCOLOR = new Color (202, 225, 255);
  final static Color BGRCOLOR = new Color ( 0, 191, 255);
  final static Color FGRCOLOR = new Color (139, 69, 19);
  final static Color TITCOLOR = new Color (141, 182, 205);
  Button ProceedButton, CancelButton;
  TextField StationField;
  Panel ButtonPanel;
  boolean Done, // Proceed flag (for waiting)
          Cancel; // Cancel flag
  int Status, // Station number or (negated) status
          NStations; // The number of stations
  java.awt.List TemplateList;
  SObject SO;
  Template Templates, SelectedTemplate;
  DisplayDialog (SObject ob, Template tl, int NS) {
    // One column with two rows: the selection area on top of a row of buttons
    setLayout (new BorderLayout (2,1));
    ButtonPanel = new Panel ();
    // One row ot three slots
    ButtonPanel.setLayout (new GridLayout (1, 3));
    ButtonPanel.setBackground (BGRCOLOR);
    ButtonPanel.setForeground (FGRCOLOR);
    ProceedButton = new Button ("Display");
    CancelButton = new Button ("Cancel");
    StationField = new TextField ("No station");
    StationField.setBackground (TITCOLOR);
    TemplateList = new java.awt.List (NSELECTIONROWS, false);
    TemplateList.setBackground (LISTCOLOR);
    ButtonPanel.add (ProceedButton);
    ButtonPanel.add (CancelButton);
    ButtonPanel.add (StationField);
    setTitle ("Display selection dialog");
    SelectedTemplate = null;
    Templates = tl;
    SO = ob;
    add ("Center", TemplateList);
    add ("South", ButtonPanel);
    NStations = NS;
    Done = false;
    Cancel = false;
    while (tl != null) {
      addTemplateList (tl);
      tl = tl.snext;
    }
    pack ();
//  SETSIZE (this, DWWIDTH, GETSIZE (this) . height);
    (this).setSize (DWWIDTH, DWHEIGHT);
    (this).setVisible (true); (this).setVisible (true);
    validate ();
    ProceedButton.addActionListener (this);
    CancelButton.addActionListener (this);
    addWindowListener (new LocalWindowAdapter ());
  }
  void addTemplateList (Template tl) {
    String td;
    switch (tl.status) {
      case Template.TS_ABSOLUTE:
        td = ".. ";
        break;
      case Template.TS_RELATIVE:
        td = "*. ";
        break;
      default:
        td = "** ";
    }
    TemplateList.addItem (td.concat (tl.desc));
  }
  class LocalWindowAdapter extends WindowAdapter {
    public void windowClosing (WindowEvent ev) {
      // To intercept destroy event
      Cancel = Done = true;
    }
  }
  public void actionPerformed (ActionEvent e) {
    int ix;
    String s;
    if (e.getActionCommand () == ProceedButton.getLabel ()) {
      // Check if there is any selection at all
      if ((ix = TemplateList.getSelectedIndex ()) < 0) return;
      for (SelectedTemplate = Templates; SelectedTemplate != null && ix > 0;
        ix--, SelectedTemplate = SelectedTemplate.snext);
      if (SelectedTemplate == null) return; // This cannot happen
      // Now check the station number
      if (SelectedTemplate.status == Template.TS_ABSOLUTE) {
        // The value field is ignored
        Status = -Template.TS_ABSOLUTE;
      } else {
        // We have to examine the value field
        s = (StationField.getText ()).trim ();
        try {
          Status = Integer.valueOf (s).intValue ();
          if (Status < 0 || Status >= NStations)
            Status = -Template.TS_ABSOLUTE;
        } catch (NumberFormatException ex) {
          Status = -Template.TS_ABSOLUTE;
        }
        if (Status < 0 && SelectedTemplate.status == Template.TS_RELATIVE)
          return;
      }
      Done = true;
    } else if (e.getActionCommand () == CancelButton.getLabel ()) {
      Cancel = true; // The order of these two is relevant!
      Done = true;
    }
  }
}
// -----------------------------------------------------------------------------
class IntervalDialog extends Frame implements ActionListener {
  // Window size
  final static int DWWIDTH = 200;
  final static int DWHEIGHT = 90;
  final static Color BGRCOLOR = new Color (238, 233, 191);
  final static Color FGRCOLOR = new Color ( 85, 107, 47);
  final static Color TITCOLOR = new Color (255, 165, 79);
  Button ProceedButton, CancelButton;
  TextField ValueField;
  Panel ButtonPanel;
  boolean Done, // Proceed flag (for waiting)
          Cancel; // Cancel flag
  int Value;
  IntervalDialog (RootWindow RW) {
    // One column with two rows: the input area on top of a row of buttons
    setLayout (new GridLayout (2,1));
    ButtonPanel = new Panel ();
    ButtonPanel.setBackground (BGRCOLOR);
    ButtonPanel.setForeground (FGRCOLOR);
    // One row ot two buttons
    ButtonPanel.setLayout (new GridLayout (1, 2));
    ProceedButton = new Button ("Set");
    CancelButton = new Button ("Cancel");
    ValueField = new TextField (Integer.toString (RW.DisplayInterval));
    ValueField.setBackground (TITCOLOR);
    ButtonPanel.add (ProceedButton);
    ButtonPanel.add (CancelButton);
    setTitle ("Refresh interval");
    add (ValueField);
    add (ButtonPanel);
    pack ();
    // Default size looks fine
    (this).setSize (DWWIDTH, DWHEIGHT);
    Done = false;
    Cancel = false;
    (this).setVisible (true); (this).setVisible (true);
    validate ();
    ProceedButton.addActionListener (this);
    CancelButton.addActionListener (this);
    addWindowListener (new LocalWindowAdapter ());
  }
  class LocalWindowAdapter extends WindowAdapter {
    public void windowClosing (WindowEvent ev) {
      // To intercept destroy event
      Cancel = Done = true;
    }
  }
  public void actionPerformed (ActionEvent e) {
    int ix;
    String s;
    if (e.getActionCommand () == ProceedButton.getLabel ()) {
      s = (ValueField.getText ()).trim ();
      try {
        Value = Integer.valueOf (s).intValue ();
      } catch (NumberFormatException ex) {
        Value = -1;
      }
      if (Value > 0) Done = true;
    } else if (e.getActionCommand () == CancelButton.getLabel ()) {
      Cancel = true;
      Done = true;
    }
  }
}
// -----------------------------------------------------------------------------
class StepDialog extends Frame implements ActionListener {
  // Window size
  final static int DWWIDTH = 200;
  final static int DWHEIGHT = 100;
  final static int INITVALUELENGTH = 32;
  final static Color BGRCOLOR = new Color (255, 165, 0);
  final static Color FGRCOLOR = new Color ( 5, 5, 5);
  final static Color TITCOLOR = new Color (255, 193, 37);
  Button ProceedButton, CancelButton;
  Checkbox EventCheck, TimeCheck, AbsoluteCheck, RelativeCheck;
  CheckboxGroup WhatCG, HowCG;
  TextField ValueField;
  Panel ButtonPanel;
  DisplayWindow W;
  boolean Done, // Proceed flag (for waiting)
          Cancel, // Cancel flag
          Relative, // The value is relative
          Evnt; // The value gives event number
  byte Value [];
  int ValueLength;
  StepDialog (DisplayWindow dw) {
    // One column with two rows: value + buttons
    Value = new byte [INITVALUELENGTH];
    setLayout (new BorderLayout ());
    ButtonPanel = new Panel ();
    ButtonPanel.setBackground (BGRCOLOR);
    ButtonPanel.setForeground (FGRCOLOR);
    // Three rows and two columns
    ButtonPanel.setLayout (new GridLayout (3, 2));
    WhatCG = new CheckboxGroup ();
    HowCG = new CheckboxGroup ();
    ProceedButton = new Button ("Step");
    CancelButton = new Button ("Cancel");
    EventCheck = new Checkbox ("Event", WhatCG, false);
    TimeCheck = new Checkbox ("Time", WhatCG, true);
    AbsoluteCheck = new Checkbox ("Absolute", HowCG, true);
    RelativeCheck = new Checkbox ("Relative", HowCG, false);
    ValueField = new TextField ("  0");
    ValueField.setBackground (TITCOLOR);
    ButtonPanel.add (EventCheck);
    ButtonPanel.add (AbsoluteCheck);
    ButtonPanel.add (TimeCheck);
    ButtonPanel.add (RelativeCheck);
    ButtonPanel.add (ProceedButton);
    ButtonPanel.add (CancelButton);
    W = dw;
    setTitle ("Delayed step dialog");
    add ("North", ValueField);
    add ("Center", ButtonPanel);
    Done = false;
    Cancel = false;
    Evnt = false;
    Relative = false;
    (WhatCG).setSelectedCheckbox (TimeCheck);
    (HowCG).setSelectedCheckbox (AbsoluteCheck);
    pack ();
    (this).setSize (DWWIDTH, DWHEIGHT);
    (this).setVisible (true); (this).setVisible (true);
    validate ();
    ProceedButton.addActionListener (this);
    CancelButton.addActionListener (this);
    addWindowListener (new LocalWindowAdapter ());
  }
  boolean convertToDigits (String s) {
    int ex, ix, sl;
    char c;
    ValueLength = 0;
    sl = s.length ();
    for (ix = 0; ix < sl; ix++) {
      c = s.charAt (ix);
      if (c == '+') continue;
      // This is somewhat heuristic, but let's hope the user isn't
      // malicious
      if (c == 'e' || c == 'E') {
        if (ValueLength == 0) {
          newDigit (1);
        }
        if (++ix >= sl) return false; // Format error
        try {
          ex = Integer.valueOf (s.substring (ix)) . intValue ();
        } catch (NumberFormatException e) {
          ex = -1;
        }
        if (ex < 0 || ex + ValueLength > RootWindow.SANE) return false;
        while (ex-- > 0) newDigit (0);
        return true;
      }
      if (c < '0' || c > '9') return false;
      newDigit ((int)(c - '0'));
    }
    return true;
  }
  void newDigit (int d) {
    byte v [];
    int i;
    if (ValueLength == Value.length) {
      v = new byte [Value.length * 2];
      for (i = 0; i < Value.length; i++) v [i] = Value [i];
      Value = v;
    }
    Value [ValueLength++] = (byte) d;
  }
  class LocalWindowAdapter extends WindowAdapter {
    public void windowClosing (WindowEvent ev) {
      // To intercept destroy event
      Cancel = Done = true;
    }
  }
  public void actionPerformed (ActionEvent e) {
    String s;
    if (e.getActionCommand () == ProceedButton.getLabel ()) {
      Evnt = (WhatCG.getSelectedCheckbox () == EventCheck);
      Relative = (HowCG.getSelectedCheckbox () == RelativeCheck);
      s = (ValueField.getText ()).trim ();
      Done = convertToDigits (s);
    } else if (e.getActionCommand () == CancelButton.getLabel ()) {
      Cancel = true;
      Done = true;
    }
  }
}
// -----------------------------------------------------------------------------
class Backup {
  // This class represents backup storage for a single item. A list of such
  // structures is associated with every display window. The list is rebuilt
  // whenever the window is resized/shifted.
  Backup next;
  byte type;
  int ItemNumber;
  public Backup (int tp, Backup ib, DisplayWindow dw, int it) {
    type = (byte) tp; next = null;
    ItemNumber = it;
    if (ib == null)
      dw.BackupStorage = this;
    else
      ib.next = this;
  }
  public void clear () { };
}
class FixedBackup extends Backup {
  int x, y;
  FixedField Fld;
  public FixedBackup (Backup ib, DisplayWindow dw, int it, FixedField f,
                                                              int xc, int yc) {
    super (Template.FT_FIXED, ib, dw, it);
    x = xc;
    y = yc;
    Fld = f;
  }
}
class FilledBackup extends Backup {
  int x, y;
  FilledField Fld;
  char Contents [];
  public FilledBackup (Backup ib, DisplayWindow dw, int it, FilledField f,
                                                              int xc, int yc) {
    super (Template.FT_FILLED, ib, dw, it);
    int len = (Fld = f).length;
    x = xc;
    y = yc;
    Contents = new char [len];
    clear ();
  }
  public void clear () {
    int len = Contents.length;
    for (int i = 0; i < len; i++) Contents [i] = ' ';
  }
}
class RegionBackup extends Backup {
  final static int INITSEGSIZE = 64; // Initial size of the segment arrays
  int x, y, w, h;
  RegionField Fld;
  // These are segment arrays. The first X-entry specifies the length of the
  // segment + 1, and the first Y-entry gives the segment attribute pattern.
  short SegX [], SegY [];
  public RegionBackup (Backup ib, DisplayWindow dw, int it, RegionField f,
                                            int xc, int yc, int x1c, int y1c) {
    super (Template.FT_REGION, ib, dw, it);
    x = xc; y = yc; w = x1c-xc+1; h = y1c-yc+1;
    SegX = new short [INITSEGSIZE];
    SegY = new short [INITSEGSIZE];
    clear ();
    Fld = f;
  }
  public void clear () {
    // This should do nicely
    SegX [0] = SegY [0] = 0;
  }
  public void store (int n, int x, int y) {
    short sx [], sy [];
    int i, nl;
    if (n >= SegX.length) {
      // The arrays must be re-allocated
      for (nl = SegX.length * 2; nl <= n; nl += nl);
      sx = new short [nl];
      sy = new short [nl];
      for (i = 0; i < SegX.length; i++) {
        sx [i] = SegX [i];
        sy [i] = SegY [i];
      }
      SegX = sx;
      SegY = sy;
    }
    SegX [n] = (short) x;
    SegY [n] = (short) y;
  }
}
class VoidBackup extends Backup {
  // This is used to represent an invisible item, i.e., one that formally
  // qualifies to be displayed because it falls between the item boundaries,
  // but won't show up on the screen because the window is narrower than its
  // full size. We have to count such items because they will be arriving
  // from the simulator, but we can skip them without trying to display them.
  // Note that a single VoidBackup represents Count of subequent invisible
  // items.
  int Count;
  public VoidBackup (Backup ib, DisplayWindow dw, int it) {
    super (Template.FT_VOID, ib, dw, it);
    Count = 1;
  }
}
// -----------------------------------------------------------------------------
class DisplayWindow extends Frame {
  final static String FONT = "Courier";
  final static int FONTSIZE = 12;
  final static int FONTSTYLE = Font.PLAIN;
//  These are for appleviewer under Windows
  final static int TOPMARGIN = 20; // Top offset
  final static int BOTTOMMARGIN = 20; // Bottom offset
//  These are for X (Unix)
//final static int    TOPMARGIN    = 40;   // Top offset
//final static int    BOTTOMMARGIN = 10;   // Bottom offset
//  These are for Windows'95
//final static int    TOPMARGIN    = 10;   // Top offset
//final static int    BOTTOMMARGIN = 50;   // Bottom offset
  final static int LEFTMARGIN = 12;
  final static int RIGHTMARGIN = 18;
  final static int REGIONFRAME = 4; // Space around region
  final static int TOPINSET = 20; // For frame
  final static int BOTTOMINSET = 8+20;
  final static int LEFTINSET = 8;
  final static int RIGHTINSET = 8+8;
  final static int MAXSHORT = 32767;
  final static Color MODECOLORF [] = { // Foregrounds corresponding to modes
                          Color.black, // Normal
                          new Color (188, 238, 104), // Reverse
                          new Color ( 25, 25, 112), // Blinking
                          Color.red, // Highlight
                                        // These are for segment lines
                          Color.blue, Color.green, Color.cyan, Color.magenta,
                          Color.orange, Color.pink, Color.yellow, Color.red
                      };
  final static Color MODECOLORB [] = { // Backgrounds corresponding to modes
                          new Color (176, 176, 176), // Normal
                          new Color ( 50, 205, 50), // Reverse
                          new Color (255, 165, 0), // Blinking
                          Color.yellow // Highlight
                      };
  final static Color FRAMENORMAL = Color.blue;
  final static Color FRAMESTEPPD = Color.red;
  final static Color BACKGRND = new Color (186, 186, 186);
  int LowItem, HighItem, // Item bounds (local perception)
      MaxWidth, MaxHeight, // Maximum width and height in points
      Station;
  RootWindow RW;
  InputBuffer IB;
  DisplayWindow next;
  Template Temp;
  Backup BackupStorage, OldBackupStorage;
  String SName;
  Image OffScreenImage;
  Dimension OffScreenSize;
  Graphics OffScreenGraphics;
  FontMetrics CurrentFontMetrics;
  int CWidth, CHeight, // Character width and height (the font is fixed)
      CAscent, CLeading,
      CDescent,
      CJust, CJustH, // Inter-character space and 1/2 of it
      CFill, // Extra fill after the last character of string
      CAscPMargin, CAscMMargin,
      Xo, Yo, // Offset coordinates of the left upper corner
      XoN, YoN; // New offsets -- to detect shift
  boolean Stepped, LastStepStatus,
          MouseIsUp,
          BackupListIsObsolete,
          RequestBoundaryUpdate;
  int PickX, PickY; // For mouse events
  public DisplayWindow (RootWindow rw, SObject ob, Template tm, int st) {
    String sn;
    IB =(RW = rw).IB;
    Temp = tm;
    next = null;
    RequestBoundaryUpdate = LastStepStatus = Stepped = false;
    MouseIsUp = true;
    // We start at the origin
    Xo = Yo = XoN = YoN = 0;
    Station = st;
    setFont (new Font (FONT, FONTSTYLE, FONTSIZE));
    pack ();
    sn = SName = ob.SName;
    if (ob.NName != null) sn = ob.NName;
    sn += " [" + Temp.mode + ']';
    if (st != -1) sn += " (" + st + ')';
    setTitle (sn);
    // Calculate the size coefficients based on the font
    calculateCharacterSize ();
    OffScreenSize = calculateWindowSize (Temp.xclipd, Temp.yclipd);
    OffScreenImage = createImage (OffScreenSize.width, OffScreenSize.height);
    OffScreenGraphics = OffScreenImage.getGraphics ();
    OffScreenGraphics.setFont (getFont ());
    OffScreenGraphics.setColor (FRAMENORMAL);
    OffScreenGraphics.fillRect (0, 0, OffScreenSize.width,
                                                         OffScreenSize.height);
    OffScreenGraphics.clipRect (LEFTINSET, TOPINSET,
         OffScreenSize.width - RIGHTINSET, OffScreenSize.height - BOTTOMINSET);
    CurrentFontMetrics = OffScreenGraphics.getFontMetrics ();
    calculateMaximumSize ();
    (this).setSize (OffScreenSize);
    setItemBoundaries ();
    BackupStorage = null;
    (this).setVisible (true);
    validate ();
    addMouseListener (new LocalMouseAdapter ());
    addMouseMotionListener (new LocalMouseMotionAdapter ());
    addWindowListener (new LocalWindowAdapter ());
    addKeyListener (new LocalKeyAdapter ());
  }
  public synchronized void destruct () { dispose (); }
  void doDrag (int x, int y) {
    int nx, ny, d;
    nx = Xo + (x - PickX);
    ny = Yo + (y - PickY);
    // Check if the move doesn't get us beyond the maximum window size
    if ((d = OffScreenSize.width - MaxWidth) > 0) {
      // We have more room than needed -- allow the display area to move
      // freely within the boundary.
      if (nx > d)
        nx = d;
      else if (nx < 0)
        nx = 0;
    } else {
      // We have less room than needed -- allow the window to move freely
      // within the display area.
      if (nx < d)
        nx = d;
      else if (nx > 0)
        nx = 0;
    }
    // Now we will do a similar thing with the height. This is a bit trickier
    // because the maximum height can be indefinite.
    if (MaxHeight > 0) {
      // Do this only if the maximum height is definite. Otherwise, we allow
      // anything. What else can we do?
      if ((d = OffScreenSize.height - MaxHeight) > 0) {
        // More room than needed
        if (ny > d) ny = d; else if (ny < 0) ny = 0;
      } else {
        // Less room than needed
        if (ny < d) ny = d; else if (ny > 0) ny = 0;
      }
    }
    Xo = nx;
    Yo = ny;
    PickX = x;
    PickY = y;
    repaint ();
  }
  class LocalWindowAdapter extends WindowAdapter {
    public void windowClosing (WindowEvent ev) {
      // To intercept destroy event
      RW.tasterAction (RootWindow.TA_DELETE, DisplayWindow.this);
    }
  }
  class LocalMouseAdapter extends MouseAdapter {
    public void mousePressed (MouseEvent e) {
      PickX = e.getX ();
      PickY = e.getY ();
      MouseIsUp = false;
      repaint ();
    }
    public void mouseReleased (MouseEvent e) {
      doDrag (e.getX(), e.getY());
      MouseIsUp = true;
    }
  }
  class LocalMouseMotionAdapter extends MouseMotionAdapter {
    public void mouseDragged (MouseEvent e) {
      MouseIsUp = false;
      doDrag (e.getX(), e.getY());
    }
  }
  class LocalKeyAdapter extends KeyAdapter {
    public void keyPressed (KeyEvent e) {
      // We use this for stepping
      switch (e.getKeyChar ()) {
        case (int) 's':
          RW.tasterAction (RootWindow.TA_STEP, DisplayWindow.this);
          return;
        case (int) 'S':
          RW.tasterAction (RootWindow.TA_DSTEP, DisplayWindow.this);
          return;
        case (int) 'u':
          RW.tasterAction (RootWindow.TA_UNSTEP, DisplayWindow.this);
          return;
        case (int) 'U':
          RW.tasterAction (RootWindow.TA_UNSTEPGO, DisplayWindow.this);
          return;
        case (int) ' ':
        case (int) 'g':
        case (int) 'G':
          RW.tasterAction (RootWindow.TA_ADVANCE, null);
          return;
        default:
          return;
      }
    }
  }
  void calculateCharacterSize () {
    FontMetrics fm = getFontMetrics (getFont ());
    CWidth = fm.stringWidth ("AAA") - fm.stringWidth ("AA");
    // This may need to be adjusted later. At the moment, I'm not sure if it
    // works at all.
    CJust = CWidth*2 - fm.stringWidth ("AA");
    if ((CJustH = CJust/2) == 0) CJustH = 1;
    // This is a bit experimental and determines the extra width of a
    // covering rectangle for a piece of string.
    CFill = CWidth/3;
    CHeight = fm.getHeight ();
    CAscent = fm.getAscent ();
    CDescent = CHeight - CAscent;
    CLeading = fm.getLeading ();
    // Precalculated sums to simplify some transformations and comparisons
    CAscPMargin = CAscent + TOPMARGIN;
    CAscMMargin = CAscent - BOTTOMMARGIN;
  }
  Dimension calculateWindowSize (int ncw, int nch) {
    return new Dimension (ncw * CWidth + LEFTMARGIN + RIGHTMARGIN,
      nch * (CHeight + CLeading) + TOPMARGIN + BOTTOMMARGIN);
  }
  int yTransform (int cpos) {
    // Transforms a character row position into baseline y-coordinate
    return CAscPMargin + cpos * CHeight;
  }
  int xTransform (int cpos) {
    // Transforms a character column position into the x-coordinate
    return LEFTMARGIN + cpos * CWidth;
  }
  // Note: the following functions are not synchronized, but whoever uses
  // them must synchronize, because they are executed in sequence and assume
  // some consistency from their predecessors.
  void setItemBoundaries () {
    // Calculates the starting and ending items that will fit in the
    // current window
    int CurrentItem, // Current item number
        yinc, // Row increment for repeating
        y, crpc;
    boolean LowFound = false;
    Field f;
    RepeatField repf;
    repf = null; // To keep the compiler happy
    BackupListIsObsolete = true;
    // This will force backup list (re)creation. We could have done it
    // right away after setItemBoundaries, but we prefer to keep the old
    // list around until the next update arrives from the simulator. This
    // way the window contents will look less jerky.
    for (f = Temp.fieldlist, crpc = yinc = CurrentItem = 0; f != null; ) {
      // This may be suboptimal, but we don't care. This is done once
      // per window resize/shift, and the way we do it, we at least know
      // what we are doing.
      switch (f.type) {
        case Template.FT_FIXED:
          // Ignore it -- fixed fields are never transmitted
          break;
        case Template.FT_FILLED:
          y = yTransform (((FilledField)f).y+yinc)+Yo;
          if (LowFound) {
            // We are looking for the upper limit
            if (y > OffScreenSize.height + CAscMMargin) {
              // This is the first item that doesn't fit on the screen
              HighItem = CurrentItem;
              return;
            }
          } else {
            // Still looking for the low bound
            if (y > TOPMARGIN - CDescent) {
              // The first item that is going to be visible
              LowFound = true;
              LowItem = CurrentItem;
            }
          }
          CurrentItem++;
          break;
        case Template.FT_REGION:
          // This is a region field
          if (LowFound) {
            // Looking for the upper limit
            y = yTransform (((RegionField)f).y+yinc)+Yo;
            // We add yinc just in case. It must be zero because regions are
            // never automatically repeated.
            if (y > BOTTOMMARGIN - CDescent) {
              // The top is invisible. Note that we use the same criteria
              // as for characters. We must, because the starting line of
              // the region may coexist with character fields.
              HighItem = CurrentItem;
              return;
            }
          } else {
            // Looking for the lower limit
            if (yTransform (((RegionField)f).y1+yinc)+Yo >
             TOPMARGIN - CDescent) {
              LowFound = true;
              LowItem = CurrentItem;
            }
          }
          CurrentItem++;
          break;
        case Template.FT_REPEAT:
          repf = (RepeatField)f;
          f = repf.from;
          yinc = 1;
          crpc = 1;
          // Start repeating
          continue;
        default: ; // Do nothing -- should never get here
      }
      // This ends the switch. Now we take care of the repetition status.
      if (yinc > 0 && f == repf.to) {
        // We are repeating and we have reached the last repeated field
        if (repf.count != 0 && crpc >= repf.count) {
          // Count defined and we have reached it -- we switch back to normal
          f = repf.next;
          if (f.type == Template.FT_REPEAT) {
            // I have no clue why I am doing it this way. This code is ported
            // from old DSD. Apparently, defined-count repeats are sometimes
            // followed by undefined-count repeats, but this is the end,
            // I hope.
            repf = (RepeatField)f;
            f = repf.from;
            crpc = 1;
            yinc++; // Note that this accumulates with the previous repeat
          } else {
            crpc = yinc = 0; // No repeating
          }
          continue;
        }
        // Next iteration of this repeat
        yinc++;
        crpc++;
        f = repf.from;
        continue;
      }
      f = f.next;
    }
    // We have reached the end of list
    HighItem = CurrentItem;
    if (!LowFound)
      // Not a single item
      LowItem = CurrentItem;
  }
  void calculateMaximumSize () {
    // Calculates the maximum width and height of the window (needed for
    // semi-intelligent shifting). The maximum height can be infinite,
    // which is represented by -1.
    int yinc, // Row increment for repeating
        x, y, crpc;
    Field f;
    RepeatField repf;
    FixedField fixf;
    FilledField filf;
    RegionField regf;
    MaxWidth = MaxHeight = 0;
    for (repf = null, f = Temp.fieldlist, crpc = yinc = 0; f != null; ) {
      switch (f.type) {
        case Template.FT_FIXED:
          y = yTransform ((fixf = (FixedField)f).y)+yinc;
          if (y > MaxHeight) MaxHeight = y; // Will adjust it later
          x = xTransform (fixf.x) + fixf.contents.length () * CWidth;
          if (x > MaxWidth) MaxWidth = x;
          break;
        case Template.FT_FILLED:
          y = yTransform ((filf = (FilledField)f).y)+yinc;
          if (y > MaxHeight) MaxHeight = y;
          x = xTransform (filf.x) + filf.length * CWidth;
          if (x > MaxWidth) MaxWidth = x;
          break;
        case Template.FT_REGION:
          y = yTransform ((regf = (RegionField)f).y1)+yinc;
          if (y > MaxHeight) MaxHeight = y;
          x = xTransform (regf.x1)+CWidth;
          if (x > MaxWidth) MaxWidth = x;
          break;
        case Template.FT_REPEAT:
          repf = (RepeatField)f;
          f = repf.from;
          yinc = 1;
          crpc = 1;
          // Start repeating
          continue;
        default: ; // Do nothing -- should never get here
      }
      // This ends the switch. Now we take care of the repetition status.
      if (yinc > 0 && f == repf.to) {
        // We are repeating and we have reached the last repeated field
        if (repf.count != 0 && crpc >= repf.count) {
          // Count defined and we have reached it -- we switch back to normal
          f = repf.next;
          if (f.type == Template.FT_REPEAT) {
            // I have no clue why I am doing it this way. This code is ported
            // from old DSD. Apparently, defined-count repeats are sometimes
            // followed by undefined-count repeats, but this is the end,
            // I hope.
            repf = (RepeatField)f;
            f = repf.from;
            crpc = 1;
            yinc++; // Note that this accumulates with the previous repeat
          } else {
            crpc = yinc = 0; // No repeating
          }
          continue;
        }
        if (repf.count == 0) {
          // We are done -- the height is infinite
          MaxHeight = -1;
          break;
        }
        // Next iteration of this repeat
        yinc++;
        crpc++;
        f = repf.from;
        continue;
      }
      f = f.next;
    }
    // We have reached the end of list
    if (MaxHeight != -1) MaxHeight += CHeight; // + BOTTOMMARGIN + TOPMARGIN;
    MaxWidth += CWidth + LEFTMARGIN; // + RIGHTMARGIN + LEFTMARGIN;
  }
  Backup recycleBackup (int item, Backup ib) {
    // Attempts to recycle an existing backup from the old list. We do it
    // only for items that actually arrive from the simulator. The reason is
    // to use their previous contents while they are waiting for an update.
    Backup b;
    while (OldBackupStorage != null && OldBackupStorage.ItemNumber < item)
      OldBackupStorage = OldBackupStorage.next;
    while (OldBackupStorage != null && OldBackupStorage.ItemNumber == item &&
      (OldBackupStorage.type == Template.FT_VOID ||
                                  OldBackupStorage.type == Template.FT_FIXED))
        OldBackupStorage = OldBackupStorage.next;
    if (OldBackupStorage == null || OldBackupStorage.ItemNumber != item)
        return null;
    b = OldBackupStorage;
    OldBackupStorage = b.next;
    b.next = null;
    if (ib == null)
      BackupStorage = b;
    else
      ib.next = b;
    return b;
  }
//  void dumpBackupList () {
//    Backup cb; FixedBackup fb; FilledBackup lb;
//    RegionBackup rb; VoidBackup vb;
//    System.out.println ("New backup list:");
//    for (cb = BackupStorage; cb != null; cb = cb.next) {
//      switch (cb.type) {
//        case Template.FT_FIXED:
//          fb = (FixedBackup)cb;
//          System.out.println ("Fixed: "+fb.x+", "+fb.y+", item = "
//           +fb.ItemNumber+" "+fb.Fld.contents);
//          break;
//        case Template.FT_FILLED:
//          lb = (FilledBackup)cb;
//          System.out.println ("Filled: "+lb.x+", "+lb.y+", item = "
//           +lb.ItemNumber+" "+new String (lb.Contents));
//          break;
//        case Template.FT_REGION:
//          rb = (RegionBackup)cb;
//          System.out.println ("Region: "+rb.x+", "+rb.y+", item = "
//            +rb.ItemNumber);
//          break;
//        default:
//          vb = (VoidBackup)cb;
//          System.out.println ("Void: "+vb.Count);
//      }
//    }
//  }
  void buildBackupList () {
    // Rebuilds the backup list for the new size/shift of the window
    int CurrentItem, // Current item number
        yinc, // Row increment for repeating
        x, y, x1, y1, crpc;
    Field f;
    FixedField fixf;
    FilledField filf;
    RegionField regf;
    RepeatField repf;
    Backup ib = null, tb;
    OldBackupStorage = BackupStorage; // For recycling
    BackupStorage = null;
    BackupListIsObsolete = false; // Mark the list up to date
    // Note: this is essentially the same traversal as in setItemBoundaries:
    // somewhat suboptimal, but then reasonably easy to comprehend.
    repf = null;
    for (f = Temp.fieldlist, crpc = yinc = CurrentItem = 0; f != null;) {
      switch (f.type) {
        case Template.FT_FIXED:
          // Fixed fields don't count to the bounds and are never transmitted,
          // but they are displayed after all, so we have to include them in
          // the backup list.
          y = yTransform ((fixf = (FixedField)f).y+yinc);
          // We have to do it this way, because fixed fields are not counted
          // in the bounds
          if (y+Yo <= TOPMARGIN - CDescent) break; // Invisible -- ignore
          if (y+Yo > OffScreenSize.height + CAscMMargin)
            // The first item that doesn't fit on the screen
            return;
          // OK, we are done with the vertical fit, now we do the horizontal
          x = xTransform (fixf.x);
          if (x+Xo > OffScreenSize.width - RIGHTMARGIN) break; // Invisible
          if (x+Xo + fixf.contents.length () * CWidth <= LEFTMARGIN) break;
          // Some portion of the item is going to be visible, so we have
          // to create a backup item for it.
          ib = new FixedBackup (ib, this, CurrentItem, fixf, x, y);
          // Note that the coordinates are normalised; so they will have to
          // be offset when the item is displayed. We need that, because the
          // offset may change while the list of backups still reflects the
          // previous shape of the window.
          // Also note that we don't attempt to recycle fixed backups, because
          // their old contents are the same as new and they are always ready.
          break;
        case Template.FT_FILLED:
          // This is a bit simpler, because we can use the precalculated bounds
          if (CurrentItem >= HighItem) return; // No more items
          if (CurrentItem >= LowItem) {
            // OK, we should consider this item
            x = xTransform ((filf = (FilledField)f).x);
            if (x+Xo > OffScreenSize.width - RIGHTMARGIN ||
                                   x+Xo + filf.length * CWidth <= LEFTMARGIN) {
              // The item won't show up
              if (ib != null && ib.type == Template.FT_VOID)
                ((VoidBackup)ib) . Count ++;
              else
                ib = new VoidBackup (ib, this, CurrentItem);
              // Note that a single VoidBackup can be used to represent a
              // number of neighbouring invisible items. We will save some
              // time on converting them and displaying.
            } else {
              // The item will show up
              if ((tb = recycleBackup (CurrentItem, ib)) != null)
                // Try to recycle
                ib = tb;
              else
                ib = new FilledBackup (ib, this, CurrentItem, filf, x,
                                                     yTransform (filf.y+yinc));
            }
          }
          CurrentItem++;
          break;
        case Template.FT_REGION:
          if (CurrentItem >= HighItem) return; // No more items
          if (CurrentItem >= LowItem) {
            // Formally, the item is going to be displayed
            x = xTransform ((regf = (RegionField)f).x);
            if (x+Xo > OffScreenSize.width - RIGHTMARGIN ||
                    (x1 = xTransform (regf.x1)+CWidth+CFill)+Xo < LEFTMARGIN) {
              // We won't see the region
              if (ib != null && ib.type == Template.FT_VOID)
                ((VoidBackup)ib) . Count ++;
              else
                ib = new VoidBackup (ib, this, CurrentItem);
            } else {
              if ((tb = recycleBackup (CurrentItem, ib)) != null)
                ib = tb;
              else
                ib = new RegionBackup (ib, this, CurrentItem, regf, x,
                       yTransform (regf.y), x1, yTransform (regf.y1));
            }
          }
          CurrentItem++;
          break;
        case Template.FT_REPEAT:
          repf = (RepeatField)f;
          f = repf.from;
          yinc = 1;
          crpc = 1;
          // Start repeating
          continue;
        default: ; // Do nothing -- should never get here
      }
      // This ends the switch. Now we take care of the repetition status --
      // exactly as before
      if (yinc > 0 && f == repf.to) {
        if (repf.count != 0 && crpc >= repf.count) {
          f = repf.next;
          if (f.type == Template.FT_REPEAT) {
            repf = (RepeatField)f;
            f = repf.from;
            crpc = 1;
            yinc++;
          } else {
            crpc = yinc = 0;
          }
          continue;
        }
        yinc++;
        crpc++;
        f = repf.from;
        continue;
      }
      f = f.next;
    }
    // We have reached the end of list -- just exit
  }
  void setupGraphics () {
    Dimension d = (this).getSize ();
    boolean shifted, resized;
    // Check if we haven't been resized or shifted
    shifted = ((Xo != XoN) || (Yo != YoN));
    resized = (d.width != OffScreenSize.width) ||
      (d.height != OffScreenSize.height); // True if we have been resized
    if (resized || LastStepStatus != Stepped) {
      // We have to change the graphics
      OffScreenImage = createImage (d.width, d.height);
      OffScreenSize = d;
      OffScreenGraphics = OffScreenImage.getGraphics ();
      OffScreenGraphics.setFont (getFont ());
      CurrentFontMetrics = OffScreenGraphics.getFontMetrics();
      LastStepStatus = Stepped;
      OffScreenGraphics.setColor (Stepped ? FRAMESTEPPD : FRAMENORMAL);
      OffScreenGraphics.fillRect (0, 0, OffScreenSize.width,
                                                         OffScreenSize.height);
      OffScreenGraphics.clipRect (LEFTINSET, TOPINSET, d.width - RIGHTINSET,
                                                       d.height - BOTTOMINSET);
    }
    if (shifted && MouseIsUp || resized) {
      setItemBoundaries ();
      RequestBoundaryUpdate = true;
      XoN = Xo;
      YoN = Yo;
    }
  }
  public synchronized void update (Graphics g) {
    // This is the window refresh function
    Backup CurB;
    setupGraphics ();
    OffScreenGraphics.setColor (BACKGRND);
    OffScreenGraphics.fillRect (0, 0,
                                    OffScreenSize.width, OffScreenSize.height);
    OffScreenGraphics.translate (Xo, Yo);
    for (CurB = BackupStorage; CurB != null; CurB = CurB.next) {
      switch (CurB.type) {
        case Template.FT_FIXED:
          displayFixed ((FixedBackup)CurB);
          continue;
        case Template.FT_FILLED:
          displayFilled ((FilledBackup)CurB);
          continue;
        case Template.FT_REGION:
          displayRegion ((RegionBackup)CurB);
          continue;
        default:
          continue; // Ignore void backups
      }
    }
    OffScreenGraphics.translate (-Xo, -Yo);
    g.drawImage (OffScreenImage, 0, 0, null);
  }
  void displayFixed (FixedBackup Bkp) {
    // Determine the rectangle boundary
    String s = Bkp.Fld.contents;
    int xs = Bkp.x,
        ys = Bkp.y-CAscent,
        xw = s.length () * CWidth + CFill,
        yh = CHeight,
        dm = Bkp.Fld.dmode;
    setColors (xs, ys, xw, yh, dm);
    if (dm != Template.DM_BLINKING || RW.BlinkON)
                      OffScreenGraphics.drawString (s, xs+CJustH, Bkp.y);
  }
  void displayFilled (FilledBackup Bkp) {
    int xs = Bkp.x, ys = Bkp.y-CAscent,
        ll = Bkp.Contents.length,
        xw = ll * CWidth + CFill,
        yh = CHeight,
        dm = Bkp.Fld.dmode;
    setColors (xs, ys, xw, yh, dm);
    if (dm != Template.DM_BLINKING || RW.BlinkON)
           OffScreenGraphics.drawChars (Bkp.Contents, 0, ll, xs+CJustH, Bkp.y);
  }
  void displayRegion (RegionBackup Bkp) {
    // Now for the tricky part
    int x = Bkp.x, y = Bkp.y, w = Bkp.w, h = Bkp.h;
    short SegX [] = Bkp.SegX, SegY [] = Bkp.SegY;
    int len = SegX.length;
    int cnt, ix, att;
    RegionField Fld = Bkp.Fld;
    setColors (x, y, w, h, Fld.dmode);
    x += REGIONFRAME; w -= REGIONFRAME+REGIONFRAME;
    y += REGIONFRAME; h -= REGIONFRAME+REGIONFRAME;
    for (ix = 0; ix < len && SegX [ix] != 0; ) {
      // We are looking at the beginning of a new segment
      att = SegY [ix];
      cnt = SegX [ix++]-1; // We offset it to allow zero-length segments
      displaySegment (x, y, w, h, att, SegX, SegY, ix, cnt);
      ix += cnt;
    }
  }
  void displaySegment (int x, int y, int w, int h, int att,
                           short SegX [], short SegY [], int from, int count) {
    // This is close to the forgotten Macintosh version
    int tp, pt, cl;
    int ix, iy, ix1, iy1;
    y += h;
    if (count == 0) return; // Just in case
    tp = (int) (att & 0x3); // Type
    pt = (int) ((att >> 2) & 0xf); // Thickness
    cl = (int) ((att >> 6) & 0xff); // Color
    OffScreenGraphics.setColor (MODECOLORF [cl % MODECOLORF.length]);
    if (tp == 1) {
      // Lines. Thickness ignored for now.
      ix = (SegX [from] * w) / MAXSHORT;
      iy = (SegY [from] * h) / MAXSHORT;
      while (--count > 0) {
        ++from;
        ix1 = (SegX [from] * w) / MAXSHORT;
        iy1 = (SegY [from] * h) / MAXSHORT;
        OffScreenGraphics.drawLine (x + ix, y - iy, x + ix1, y - iy1);
        ix = ix1;
        iy = iy1;
      }
    } else {
      pt += 2; // Make sure we have got something
      while (count-- > 0) {
        ix = (SegX [from] * w) / MAXSHORT;
        iy = (SegY [from] * h) / MAXSHORT;
        if (tp == 2)
          // Points
          OffScreenGraphics.fillOval (x+ix, y-iy, pt, pt);
        else
          // Stripes
          OffScreenGraphics.fillRect (x+ix, y-iy, pt, iy);
        from++;
      }
    }
  }
  void setColors (int xs, int ys, int xw, int yh, int dm) {
    // Draws a background for a display item
    Color cb = MODECOLORB [dm],
          cf = MODECOLORF [dm];
    OffScreenGraphics.setColor (cb);
    if (dm == Template.DM_BRIGHT)
      OffScreenGraphics.fill3DRect (xs, ys, xw, yh, true);
    else
      OffScreenGraphics.fillRect (xs, ys, xw, yh);
    OffScreenGraphics.setColor (cf);
  }
  public synchronized void updateWindow () {
    int po, citem, sitem, CVCount, n, att, spos, x, y;
    Backup cb;
    RegionBackup rb;
    if (IB.getOctet () != PRT.PH_INM) {
      // Basic sanity check
      IB.ignore ();
      return;
    }
    sitem = IB.getInt (); // The starting item location
    if (BackupListIsObsolete) {
      // This is the first update for a resized/shifted window -- the backup
      // list must be rebuilt.
      buildBackupList ();
      // dumpBackupList ();
    }
    cb = BackupStorage;
    citem = LowItem;
    // System.out.println ("Arrived item: "+sitem+", Backup item: "+citem);
    CVCount = 0;
    // Now we do the skipping to catch up with the starting item
    while (cb != null && (citem < sitem || cb.type == Template.FT_FIXED)) {
      if (cb.type == Template.FT_VOID) {
        if ((n = ((VoidBackup)cb).Count + citem) > sitem) {
          // We stop half way into the void
          CVCount = n - sitem;
          citem = sitem;
          break;
        }
        citem = n;
      } else if (cb.type != Template.FT_FIXED) {
        // This is a regular item. Note that fixed items don't count at all.
        cb.clear ();
        citem++;
      }
      cb = cb.next;
    }
    // We need the following for initialization, so that it looks the same,
    // no matter whether we had to skip some backup items, if the list doesn't
    // fit the update for some reason. Note that the skip may stop half-way
    // into an aggregate Void backup.
    if (cb != null && cb.type == Template.FT_VOID && CVCount == 0)
      CVCount = ((VoidBackup)cb).Count;
    // At this point we have found the first backup item matching the first
    // arriving item, or perhaps the backup list is null, in which case we
    // will have to ignore all items. Anyway, we may hit this problem later,
    // so for now let us just start displaying them.
    while (IB.more ()) {
      switch (po = IB.peekOctet ()) {
        case PRT.PH_INM:
        case PRT.PH_BNM:
        case PRT.PH_TXT:
        case PRT.PH_FNM:
          // A regular "filled" item
          if (sitem < citem) {
            sitem++;
            IB.skipNumberOrText ();
            continue;
          }
          if (cb == null) {
            // We have run out of backups
            IB.skipNumberOrText ();
            break;
          }
          if (cb.type == Template.FT_VOID) {
            CVCount--;
            IB.skipNumberOrText ();
            break;
          }
          if (cb.type != Template.FT_FILLED || sitem != cb.ItemNumber) {
            // We are out of sync. I am not quite sure what I should do now.
            RW.Alerts.displayLine ("Template out of sync with window contents");
            // This way we will skip everything that follows
            cb = null;
            IB.skipNumberOrText ();
            break;
          }
          // Now we can simply fill the backup
          switch (po) {
            case PRT.PH_INM:
            case PRT.PH_BNM:
              IB.getInt (((FilledBackup)cb).Fld.length,
                           ((FilledBackup)cb).Contents,
                           0,
                           ((FilledBackup)cb).Fld.just == Template.LEFT);
              break;
            case PRT.PH_TXT:
              IB.getText (((FilledBackup)cb).Fld.length,
                           ((FilledBackup)cb).Contents,
                           0,
                           ((FilledBackup)cb).Fld.just == Template.LEFT);
              break;
            case PRT.PH_FNM:
              IB.getFloat (((FilledBackup)cb).Fld.length,
                           ((FilledBackup)cb).Contents,
                           0,
                           ((FilledBackup)cb).Fld.just == Template.LEFT);
              break;
          }
          break;
        case PRT.PH_REG:
          // This is a region
          if (sitem < citem) {
            sitem++;
            IB.skipRegion ();
            continue;
          }
          if (cb == null) {
            IB.skipRegion ();
            break;
          }
          if (cb.type == Template.FT_VOID) {
            CVCount--;
            IB.skipRegion ();
            break;
          }
          if (cb.type != Template.FT_REGION || sitem != cb.ItemNumber) {
            RW.Alerts.displayLine ("Template out of sync with window contents");
            cb = null;
            IB.skipRegion ();
            break;
          }
          // We are ready to update the segment
          IB.getOctet (); // Skip the REG byte
          rb = (RegionBackup)cb;
          n = 0;
          while (IB.peekOctet () == PRT.PH_SLI) {
            // This is a new segment
            IB.getOctet ();
            spos = IB.getOctet ();
            att = IB.getOctet ();
            att |= (spos << 7);
            rb.store (n, 0, att);
            // Save this location for length; unfortunately, we don't know it
            // in advance
            spos = n;
            n++;
            while (IB.peekOctet () == PRT.PH_INM) {
              x = IB.getInt (); y = IB.getInt ();
              rb.store (n, x, y);
              n++;
            }
            // Now insert the length + 1
            rb.SegX [spos] = (short)(n - spos);
          }
          IB.getOctet (); // This is the REN byte
          break;
        default:
          // Assume this is the end of an update phrase
          while (cb != null) {
            cb.clear ();
            cb = cb.next;
          }
          return;
      }
      sitem++;
      if (cb != null) {
        if (cb.type != Template.FT_VOID || CVCount == 0) {
          if ((cb = cb.next) != null) {
            if (cb.type == Template.FT_VOID) {
              CVCount = ((VoidBackup)cb).Count;
            } else {
              while (cb != null && cb.type == Template.FT_FIXED) cb = cb.next;
            }
          }
        }
      }
    } // while
    while (cb != null) {
      cb.clear ();
      cb = cb.next;
    }
  }
}
// -----------------------------------------------------------------------------
class RootWindow extends Frame implements ActionListener {
  final static int SANE = 512; // Limit for scratch array size
  // Window size
  final static int RWWIDTH = 400;
  final static int RWHEIGHT = 530;
  // List font
  final static String LISTFONT = "Courier";
  final static int LISTFONTSIZE = 12;
  final static int LISTFONTSTYLE = Font.PLAIN;
  // Credit delay. After that much inactivity a credit is sent to the
  // simulator (milliseconds)
  final static int CREDITDELAY = 2000;
  // Display delay in the main processing loop
  final static int DISPLAYDELAY = 600;
  // Blink time (for blinking items)
  final static int BLINKTIME = 600;
  // The number of visible rows in the selection list
  final static int NSELECTIONROWS = 24;
  // The number of tasters
  final static int NTASTERS = 6;
  // Columns of Smurph menu
  final static int SML_DATE = 19; // First 19 columns
  final static int SML_HOST = 10; // Hostname
  final static int SML_PARG = 20; // Program args
  final static int SML_PRID = 5; // Process Id
  // The total is 19+1 + 10+1 + 20+1 + 5 (PId) = 57
  final static int SML_TOTL = SML_DATE+SML_HOST+SML_PARG+SML_PRID + 3;
  // Smurph list header
  final static String SML_HDR =
                "Date/Time           Host       Call Arguments         PID";
  // Columns of object selection menu
  final static int OBL_SNAME = 25; // Standard name
  final static int OBL_BNAME = 20; // Base name
  final static int OBL_NNAME = 10; // Nickname
  // 25 + 1 + 20 + 1 + 10 = 57 (SML_TOTL)
  final static Color BGRCOLOR = new Color (222, 184, 135);
  final static Color FGRCOLOR = new Color (165, 42, 42);
  final static Color SMLISTCOLOR = new Color (188, 238, 104);
  final static Color SMLIHDCOLOR = new Color (255, 99, 71);
  final static Color OMENUCOLOR = new Color ( 0, 206, 209);
  final static Color OWNERCOLOR = new Color (127, 255, 0);
  // Taster actions.
  final static int TA_GETLIST = 1;
  final static int TA_CONNECT = 2;
  final static int TA_STATUS = 3;
  final static int TA_DESCEND = 4;
  final static int TA_ASCEND = 5;
  final static int TA_DISPLAY = 6;
  final static int TA_DISCONNECT = 7;
  final static int TA_UNSTEP = 8; // Unstep
  final static int TA_ADVANCE = 9; // Step advance
  final static int TA_STEP = 10; // Step
  final static int TA_DSTEP = 11; // Delayed step
  final static int TA_DELETE = 12; // Delete window
  final static int TA_SHOWHIDE = 13; // Delete window
  final static int TA_UNSTEPGO = 14; // Unstep all and go
  final static int TA_INTERVAL = 15; // Reset display interval
  final static int TA_TERMINATE = 16; // Terminate Smurph execution
  DSD Main; // Pointer to our predecessor
  SInterface SI; // Simulator interface monitor
  OutputBuffer OB; // Output buffer
  InputBuffer IB; // Input buffer
  AlertArea Alerts; // Place to display errors in
  Taster TasterList; // The list of all tasters
  MenuBar Menus;
  Menu NavigateMenu, StepMenu, StepDelayedMenu, UnstepMenu, DeleteMenu,
       ShowHideMenu;
  Socket MSocket; // Monitor socket
  java.awt.List CurrentSelection, // The present selection
       SelectionHead; // The header
  SObject ObjectMenu; // Current head of the menu
  SmList SmurphList; // List of running simulators
  Template Templates, // Current list of templates
           STemplates; // Selected templates
  boolean DisplaySession, // True if within a display session
          MenusDisabled, // True if menus disabled
          Stepped, // Halted
          NotUpdated, // To tell whether a window update arrived
          BlinkON,
          DialogPending, // Flag == a dialog is pending
          TerminateRequested; // Flag == TA_TERMINATE hit once
  DisplayDialog DD; // Dialog for selecting the display mode ...
  IntervalDialog ID; // the display refresh interval ...
  StepDialog SD; // and for selecting step parameters
  DisplayWindow DisplayWindowList;
  int NStations, DisplayInterval;
  long LastUpdateTime, LastBlinkTime;
  Panel TPanel;
  // This is made 'global' to avoid unnecessary allocation and garbage
  // collection in methods.
  char chbuf [] = new char [SANE];
  // This is set by Taster::actionPerformed to indicate what has happened
  Taster ActionTaster;
  public RootWindow (DSD m) {
    FontMetrics fm;
    int ww;
    Main = m; // Where we come from
    // We start from setting up the GUI. We need it before we get to
    // trying to open a socket because we need a place to display
    // a possible error message. Besides, the socket may be opened and
    // closed several times per session.
    TasterList = null;
    ObjectMenu = null;
    DD = null;
    SD = null;
    ID = null;
    TerminateRequested = DialogPending = BlinkON = DisplaySession = false;
    SmurphList = null;
    DisplayWindowList = null;
    NStations = DisplayInterval = 0;
    // Create menus
    setBackground (BGRCOLOR);
    setForeground (FGRCOLOR);
    NavigateMenu = new Menu ("Navigate");
    StepMenu = new Menu ("Step");
    StepDelayedMenu = new Menu ("Step delayed");
    UnstepMenu = new Menu ("Unstep");
    DeleteMenu = new Menu ("Delete");
    ShowHideMenu = new Menu ("Show/hide");
    Menus = new MenuBar ();
    Menus.add (NavigateMenu);
    Menus.add (StepMenu);
    Menus.add (StepDelayedMenu);
    Menus.add (UnstepMenu);
    Menus.add (DeleteMenu);
    Menus.add (ShowHideMenu);
    setMenuBar (Menus);
    Alerts = new AlertArea (this);
    // We've got evrything except the selection area
    CurrentSelection = new java.awt.List (NSELECTIONROWS, false);
    SelectionHead = new java.awt.List (1, false);
    CurrentSelection.setFont (new Font (LISTFONT, LISTFONTSTYLE, LISTFONTSIZE));
    SelectionHead.setFont (new Font (LISTFONT, LISTFONTSTYLE, LISTFONTSIZE));
    // We only allow a single selection at a time (in all guises of our list).
    // The header item can also be selected. When clicked, it means "ascend".
    setLayout (new BorderLayout ());
    // Our window consists of three pieces: header, selection area, and
    // the alert area stacked in a single column. The lists start empty.
    TPanel = new Panel ();
    TPanel.setLayout (new GridLayout (2,1));
    TPanel.add (CurrentSelection);
    TPanel.add (Alerts);
    add ("North", SelectionHead);
    add ("Center", TPanel);
    // Create the interface monitor
    SI = new SInterface (this);
    OB = SI.OB;
    IB = SI.IB;
    // Done. Now we are ready to try to get connected. This will be handled
    // from the thread.
    enableMenus ();
    setTitle ("DSD Root Window");
    pack ();
    // Determine the window width that should be enough to accommodate the
    // experiment list header
    fm = SelectionHead.getFontMetrics (SelectionHead.getFont ());
    ww = fm.stringWidth (SML_HDR)+40;
    if (ww < RWWIDTH) ww = RWWIDTH;
    (this).setSize (ww, RWHEIGHT);
    (this).setVisible (true);
    validate ();
    ActionTaster = null;
    addWindowListener (new LocalWindowAdapter ());
    addKeyListener (new LocalKeyAdapter ());
  }
  void repack () {
    // I have no clue why I have to do idiocy like this, but otherwise
    // the list looks strange after its contents change
    // TPanel.remove (CurrentSelection);
    // TPanel.add (CurrentSelection, 0);
    // validate ();
  }
  synchronized void enableMenu (Menu m, String s, int Action, DisplayWindow w) {
    Taster ta;
    // Check if not enabled already
    for (ta = TasterList; ta != null; ta = ta.next)
      if (ta.Action == Action && ta.wind == w) return; // Do nothing
    MenuItem mi = new MenuItem (s);
    new Taster (this, m, mi, Action, w);
    mi.addActionListener (this);
  }
  synchronized void disableMenu (Menu m, int Action, DisplayWindow w) {
    Taster ta, tp;
    for (ta = TasterList, tp = null; ta != null; ) {
      if (ta.menu == m && ta.Action == Action && ta.wind == w) {
        ta.menu.remove (ta.item);
        ta = ta.next;
        if (tp == null) TasterList = ta; else tp.next = ta;
      } else {
        tp = ta;
        ta = ta.next;
      }
    }
  }
  void disableMenus () { MenusDisabled = true; }
  synchronized void enableMenus () { setupMenus (); MenusDisabled = false; }
  boolean isDisplayed (DisplayWindow w) {
    // To be called from synchronized methods
    for (DisplayWindow c = DisplayWindowList; c != null; c = c.next)
      if (c == w) return true;
    return false;
  }
  synchronized void setupMenus () {
    // Enables the menus that make sense in the current scenario
    DisplayWindow dw;
    String wname;
    if (DisplaySession) {
      // First remove what doesn't belong here
      disableMenu (NavigateMenu, TA_GETLIST, null);
      disableMenu (NavigateMenu, TA_STATUS, null);
      disableMenu (NavigateMenu, TA_CONNECT, null);
      // Now we go for fixed window menus
      enableMenu (NavigateMenu, "Descend", TA_DESCEND, null);
      enableMenu (NavigateMenu, "Ascend", TA_ASCEND, null);
      enableMenu (NavigateMenu, "Display", TA_DISPLAY, null);
      enableMenu (NavigateMenu, "Reset Refresh Interval", TA_INTERVAL, null);
      enableMenu (NavigateMenu, "Disconnect", TA_DISCONNECT, null);
      enableMenu (NavigateMenu, "Terminate", TA_TERMINATE, null);
      // This is unconditional, so it appears on top of the list
      enableMenu (StepMenu, "ADVANCE", TA_ADVANCE, null);
      // And we replicate it here
      enableMenu (UnstepMenu, "ADVANCE", TA_ADVANCE, null);
      // We make this unconditional -- just in case
      enableMenu (UnstepMenu, "ALL", TA_UNSTEP, null);
      enableMenu (UnstepMenu, "ALL+GO", TA_UNSTEPGO, null);
      for (dw = DisplayWindowList; dw != null; dw = dw.next) {
        wname = dw.getTitle ();
        enableMenu (DeleteMenu, wname, TA_DELETE, dw);
        enableMenu (ShowHideMenu, wname, TA_SHOWHIDE, dw);
        if (dw.Stepped) {
          enableMenu (UnstepMenu, wname, TA_UNSTEP, dw);
          disableMenu (StepMenu, TA_STEP, dw);
          disableMenu (StepDelayedMenu, TA_DSTEP, dw);
        } else {
          disableMenu (UnstepMenu, TA_UNSTEP, dw);
          enableMenu (StepMenu, wname, TA_STEP, dw);
          enableMenu (StepDelayedMenu, wname, TA_DSTEP, dw);
        }
      }
      return;
    }
    // No display session in progress
    if (SmurphList != null) {
      disableMenu (NavigateMenu, TA_GETLIST, null);
      disableMenu (NavigateMenu, TA_DESCEND, null);
      disableMenu (NavigateMenu, TA_ASCEND, null);
      disableMenu (NavigateMenu, TA_DISPLAY, null);
      disableMenu (NavigateMenu, TA_DISCONNECT, null);
      disableMenu (NavigateMenu, TA_TERMINATE, null);
      disableMenu (NavigateMenu, TA_INTERVAL, null);
      disableMenu (StepMenu, TA_ADVANCE, null);
      disableMenu (UnstepMenu, TA_ADVANCE, null);
      disableMenu (UnstepMenu, TA_UNSTEP, null);
      disableMenu (UnstepMenu, TA_UNSTEPGO, null);
      // Window menus must have been disabled earlier
      enableMenu (NavigateMenu, "Connect", TA_CONNECT, null);
      enableMenu (NavigateMenu, "Status", TA_STATUS, null);
      return;
    }
    disableMenu (NavigateMenu, TA_DESCEND, null);
    disableMenu (NavigateMenu, TA_ASCEND, null);
    disableMenu (NavigateMenu, TA_DISPLAY, null);
    disableMenu (NavigateMenu, TA_DISCONNECT, null);
    disableMenu (NavigateMenu, TA_TERMINATE, null);
    disableMenu (NavigateMenu, TA_INTERVAL, null);
    disableMenu (StepMenu, TA_ADVANCE, null);
    disableMenu (UnstepMenu, TA_ADVANCE, null);
    disableMenu (UnstepMenu, TA_UNSTEP, null);
    disableMenu (UnstepMenu, TA_UNSTEPGO, null);
    disableMenu (NavigateMenu, TA_CONNECT, null);
    disableMenu (NavigateMenu, TA_STATUS, null);
    enableMenu (NavigateMenu, "Get List", TA_GETLIST, null);
  }
  void deleteWindowFromMenus (DisplayWindow dw) {
    disableMenu (StepMenu, TA_STEP, dw);
    disableMenu (StepDelayedMenu, TA_DSTEP, dw);
    disableMenu (UnstepMenu, TA_UNSTEP, dw);
    disableMenu (DeleteMenu, TA_DELETE, dw);
    disableMenu (ShowHideMenu, TA_SHOWHIDE, dw);
  }
  class LocalWindowAdapter extends WindowAdapter {
    public void windowClosing (WindowEvent ev) {
      // To intercept destroy event
      Main.terminate ();
    }
  }
  public void actionPerformed (ActionEvent e) {
    if (ActionTaster != null) {
      tasterAction (ActionTaster.Action, ActionTaster.wind);
      ActionTaster = null;
    }
  }
  class LocalKeyAdapter extends KeyAdapter {
    public void keyPressed (KeyEvent e) {
      switch (e.getKeyChar ()) {
        case (int) ' ':
        case (int) 'g':
        case (int) 'G':
          tasterAction (TA_ADVANCE, null);
          return;
        default:
          return;
      }
    }
  }
  void sendBuffer (boolean start) {
    if (OB.flush () == true) {
      SI.closeSocket ();
      Alerts.displayLine ("Broken connection");
      return;
    }
    if (start) {
      if (!DisplaySession) disableMenus ();
      Main.startReceiverThread ();
    }
  }
  boolean sendSignature (int phrase) {
    if (SI.openSocket ()) return true; // Already connected or error
    OB.reset (PRT.PH_SIG);
    OB.sendText (PRT.SERVRSIGN);
    OB.sendOctet (phrase);
    return false;
  }
  public synchronized void deleteDisplayWindows () {
    while (DisplayWindowList != null)
      removeDisplayWindow (DisplayWindowList);
    if (DD != null) {
        (DD).setVisible (false);
        DD.dispose ();
        DD = null;
    }
    if (SD != null) {
        (SD).setVisible (false);
        SD.dispose ();
        SD = null;
    }
    if (ID != null) {
        (ID).setVisible (false);
        ID.dispose ();
        ID = null;
    }
    DialogPending = false;
  }
  synchronized void tasterAction (int Action, DisplayWindow W) {
    int ix, ix1;
    SmList sl;
    SObject so;
    // This has been delayed, so that the user can have a look at the windows
    // after the session has been completed.
    if (!DisplaySession) deleteDisplayWindows ();
    // You have to do it twice to terminate the simulator
    if (Action != TA_TERMINATE) TerminateRequested = false;
    switch (Action) {
      case TA_GETLIST:
        // Send the initial phrase requesting the list of all active
        // experiments   
        Alerts.displayLine ("Waiting for list of programs ... ");
        if (sendSignature (PRT.PH_BPR)) return;
        sendBuffer (true);
        return;
      case TA_CONNECT:
        Stepped = false;
      case TA_STATUS:
        // Connect to a simulator
        if ((ix = CurrentSelection.getSelectedIndex ()) < 0) {
          Alerts.displayLine ("No selection");
          return;
        }
        if (Action == TA_CONNECT)
          Alerts.displayLine ("Establishing handshake ... ");
        for (sl = SmurphList; ix > 0 && sl != null; ix--, sl = sl.next);
        if (sl == null) return; // This cannot happen
        if (sendSignature (Action == TA_CONNECT ? PRT.PH_DRQ : PRT.PH_STA))
          return;
        OB.sendInt (sl.Handle);
        sendBuffer (true);
        return;
      case TA_DESCEND:
        if ((ix = CurrentSelection.getSelectedIndex ()) < 0) {
          Alerts.displayLine ("No selection");
          return;
        }
        for (so = ObjectMenu; ix > 0 && so != null; ix--, so = so.next);
        if (so == null || !DisplaySession) return; // This cannot happen
        Alerts.displayLine ("Waiting for object menu ... ");
        OB.reset (PRT.PH_MEN);
        OB.sendText (so.SName);
        sendBuffer (false);
        return;
      case TA_ASCEND:
        if (ObjectMenu != null && DisplaySession) {
          Alerts.displayLine ("Waiting for object menu ... ");
          for (so = ObjectMenu; so.next != null; so = so.next);
          OB.reset (PRT.PH_UPM);
          OB.sendText (so.SName);
          sendBuffer (false);
        }
        return;
      case TA_DISPLAY:
        ix = CurrentSelection.getSelectedIndex ();
        ix1 = SelectionHead.getSelectedIndex ();
        if (ix >= 0 && ix1 >= 0) {
          Alerts.displayLine ("More than one selection");
          return;
        }
        if (ix < 0 && ix1 < 0) {
          Alerts.displayLine ("No selection");
          return;
        }
        if (ix < 0) ix = -1; // Force the tail -- the owner
        for (so = ObjectMenu; ix != 0; ix--, so = so.next)
          if (so == null || so.next == null) break; // Stop at the end
        if (so == null || !DisplaySession) return;
        requestDisplay (so);
        return;
      case TA_DISCONNECT:
        // Terminate display session
        OB.reset (PRT.PH_END); // We don't care if it works. Disconnection will
        sendBuffer (false); // be forced if the socket is closed.
        return;
      case TA_TERMINATE:
        // You have to hit it twice
        if (!TerminateRequested) {
          Alerts.displayLine ("Click it once more to terminate the experiment");
          TerminateRequested = true;
        } else {
          TerminateRequested = false;
          OB.reset (PRT.PH_EXI);
          sendBuffer (false);
        }
        return;
      case TA_ADVANCE:
        // Step advance
        OB.reset (PRT.PH_SRL);
        sendBuffer (false);
        Stepped = false;
        return;
      case TA_UNSTEPGO:
      case TA_UNSTEP:
        // Unstep (global or local)
        if (W != null) {
          if (isDisplayed (W)) { // Guards against races
            sendUnstepPhrase (W);
            W.Stepped = false;
          }
        } else {
          // Unstep all windows
          for (W = DisplayWindowList; W != null; W = W.next)
            W.Stepped = false;
          OB.reset (PRT.PH_ESM);
          sendBuffer (false);
        }
        enableMenus ();
        if (Action == TA_UNSTEPGO) {
          OB.reset (PRT.PH_SRL);
          sendBuffer (false);
          Stepped = false;
        }
        return;
      case TA_STEP:
        if (isDisplayed (W)) stepWindow (W);
        return;
      case TA_DSTEP:
        // This goes through a dialog
        if (isDisplayed (W) && SD == null) {
          disableMenus (); // Prevent race
          SD = new StepDialog (W);
          DialogPending = true;
        }
        return;
      case TA_INTERVAL:
        // This requires a dialog too
        if (ID != null) return;
        disableMenus (); // Interlock
        ID = new IntervalDialog (this);
        DialogPending = true;
        return;
      case TA_SHOWHIDE:
        if (isDisplayed (W)) {
          if (W.isVisible ())
            (W).setVisible (false);
          else
            (W).setVisible (true);
        }
        return;
      case TA_DELETE:
        if (isDisplayed (W)) deleteWindow (W);
    }
  }
  void resetDisplayInterval () {
    DisplayInterval = ID.Value;
    OB.reset (PRT.PH_DIN);
    OB.sendInt (DisplayInterval);
    sendBuffer (false);
  }
  synchronized void updateItemBoundaries (DisplayWindow dw) {
    // Sends an update for item boundaries
    dw.RequestBoundaryUpdate = false;
    OB.reset (PRT.PH_WND);
    OB.sendText (dw.SName);
    OB.sendInt (dw.Temp.mode);
    if (dw.Station >= 0) OB.sendInt (dw.Station);
    OB.sendOctet (PRT.PH_TRN);
    OB.sendInt (dw.LowItem);
    OB.sendInt (dw.HighItem);
    sendBuffer (false);
  }
  void stepWindowDelayed () {
    // Delayed step -- through a dialog. Note that neither this nor the
    // following method must be synchronized because they are called from
    // synchronized methods.
    DisplayWindow w;
    if (SD == null) return; // Just in case -- cannot happen
    w = SD.W;
    OB.reset (PRT.PH_WND);
    OB.sendText (w.SName);
    OB.sendInt (w.Temp.mode);
    if (w.Station >= 0) OB.sendInt (w.Station);
    OB.sendOctet (PRT.PH_STP);
    OB.sendDigits ((SD.Evnt ? PRT.PH_INM : PRT.PH_BNM),
                                                     SD.Value, SD.ValueLength);
    if (SD.Relative) OB.sendOctet (PRT.PH_REL);
    w.Stepped = true;
    sendBuffer (false);
  }
  void stepWindow (DisplayWindow w) {
    // Immediate step 
    OB.reset (PRT.PH_WND);
    OB.sendText (w.SName);
    OB.sendInt (w.Temp.mode);
    if (w.Station >= 0) OB.sendInt (w.Station);
    OB.sendOctet (PRT.PH_STP);
    w.Stepped = true;
    sendBuffer (false);
    enableMenus ();
  }
  void deleteWindow (DisplayWindow w) {
    // Send cancellation phrase
    OB.reset (PRT.PH_EWN);
    OB.sendText (w.SName);
    OB.sendInt (w.Temp.mode);
    if (w.Station >= 0) OB.sendInt (w.Station);
    sendBuffer (false);
    // Remove the window
    removeDisplayWindow (w);
    enableMenus ();
  }
  boolean nameCompare (String s1, String s2) {
    // Returns true if the type-id part of the two names is the same (this
    // means the part before the first blank)
    int l1 = s1.length (),
        l2 = s2.length (),
        lx, ix;
    char c1, c2;
    for (lx = (l1 < l2) ? l1 : l2, ix = 0; ix < lx; ix++) {
      c1 = s1.charAt (ix); c2 = s2.charAt (ix);
      if (c1 != c2) return false;
      if (c1 == ' ') return true;
    }
    if (l1 > l2) return (s1.charAt (ix) == ' ');
    if (l1 < l2) return (s2.charAt (ix) == ' ');
    return true;
  }
  boolean nameCompare (String s, int n) {
    // Compares the string with character array chbuf
    if (s.length () != n) return false;
    for (int i = 0; i < n; i++) if (chbuf [i] != s.charAt (i)) return false;
    return true;
  }
  void findTemplates (String name, boolean srelative) {
    Template t, st;
    if (name == null) return; // Nothing to do
    for (t = Templates; t != null; t = t.next) {
      if ((srelative && t.status == Template.TS_RELATIVE ||
        !srelative && t.status != Template.TS_RELATIVE) &&
          nameCompare (name, t.typeidnt)) {
        t.snext = null;
        if (STemplates == null)
          STemplates = t;
        else {
          for (st = STemplates; st.snext != null; st = st.snext);
          st.snext = t;
        }
      }
    }
  }
  void requestDisplay (SObject ob) {
    // Processes a display request for object ob
    STemplates = null;
    // This is the order in which templates are presented
    findTemplates (ob.NName, false); // Nickname, absolute
    findTemplates (ob.NName, true); // Nickname, station-relative
    findTemplates (ob.SName, false); // Standard name, absolute
    findTemplates (ob.SName, true); // Standard name, station-relative
    findTemplates (ob.BName, false); // Base name, absolute
    findTemplates (ob.BName, true); // Base name, station-relative
    if (STemplates == null) {
      Alerts.displayLine ("Template not arrived yet");
      return; // There is nothing we can do
    }
    disableMenus ();
    // Check if there is a need to get involved in a dialog
    if (STemplates.snext == null // Only one template ...
     && STemplates.status == Template.TS_ABSOLUTE) { // ... and no choice ...
      createDisplayWindow (ob, STemplates, -Template.TS_ABSOLUTE);
      STemplates = null;
      enableMenus ();
      return;
    }
    // The creation will be taken care of by the display thread. Note that
    // menus remain disabled until we have done something, so we shouldn't
    // worry that another similar request will mess things up for us.
    if (DD != null) {
      // But just in case ...
      STemplates = null;
      enableMenus ();
      return;
    }
    DD = new DisplayDialog (ob, STemplates, NStations);
    DialogPending = true;
    // Disabling menus acts as a locking mechanism for the apparent race
    // condition that we have here
  }
  void sendUnstepPhrase (DisplayWindow dw) {
    OB.reset (PRT.PH_WND);
    OB.sendText (dw.SName);
    OB.sendInt (dw.Temp.mode);
    if (dw.Station >= 0) OB.sendInt (dw.Station);
    OB.sendOctet (PRT.PH_STP);
    OB.sendOctet (PRT.PH_ESM);
    sendBuffer (false);
  }
  synchronized void removeDisplayWindow (DisplayWindow dw) {
    DisplayWindow dc, dp;
    for (dp = null, dc = DisplayWindowList; dc != null; dc = dc.next) {
      if (dc == dw) {
        deleteWindowFromMenus (dc);
        if (dc.Stepped) sendUnstepPhrase (dc);
        (dc).setVisible (false);
        if (dp == null)
          DisplayWindowList = dc.next;
        else
          dp.next = dc.next;
        dc.destruct ();
        return;
      }
      dp = dc;
    }
  }
  synchronized void windowRemove () {
    // Process REM phrase -- forced window removal
    int nl, mode, sttn;
    DisplayWindow w;
    if (IB.getOctet () != PRT.PH_TXT) {
      // Sanity check
      IB.ignore ();
      return;
    }
    nl = IB.getText (SANE, chbuf);
    mode = IB.getInt ();
    sttn = IB.getInt ();
    for (w = DisplayWindowList; w != null; w = w.next) {
      if (nameCompare (w.SName, nl) && w.Station == sttn &&
                                                        w.Temp.mode == mode) {
        // We have got it
        removeDisplayWindow (w);
        Alerts.displayLine ("Object deleted, window removed");
        return;
      }
    }
  }
  synchronized void processStep () {
    Stepped = true;
    Alerts.displayLine ("STEP wait (space to proceed)");
  }
  synchronized void createDisplayWindow (SObject ob, Template tm, int Station) {
    DisplayWindow w;
    if (Station < 0) Station = -1; // Normalise this just in case
    // Check if the window is not already displayed
    for (w = DisplayWindowList; w != null; w = w.next) {
      if (w.SName == ob.SName && w.Station == Station && w.Temp == tm) {
        Alerts.displayLine ("Window already displayed");
        return;
      }
    }
    Alerts.displayLine ("Creating display window ... ");
    w = new DisplayWindow (this, ob, tm, Station);
    w.next = DisplayWindowList;
    DisplayWindowList = w;
    Alerts.appendLine ("done");
    Alerts.displayLine ("Notifying the program ... ");
    // Send the request phrase to the simulator
    OB.reset (PRT.PH_WND);
    OB.sendText (ob.SName);
    OB.sendInt (tm.mode);
    if (Station >= 0) OB.sendInt (Station);
    OB.sendOctet (PRT.PH_TRN);
    OB.sendInt (w.LowItem);
    OB.sendInt (w.HighItem);
    sendBuffer (false);
    Alerts.appendLine ("done");
  }
  int getError () {
    String es;
    int erc;
    while ((erc = IB.peekOctet ()) == PRT.PH_ERR) IB.getOctet ();
    IB.getOctet (); // Error code which we ignore
    if (IB.getOctet () != PRT.PH_TXT) return 0;
    es = IB.getText ();
    Alerts.displayLine (es);
    return erc;
  }
  synchronized void terminateDisplaySession () {
    Alerts.displayLine ("Session terminated, cleaning up ... ");
    if (DisplaySession == false) return;
    Templates = STemplates = null;
    if ((CurrentSelection).getItemCount () != 0) {
      (CurrentSelection).setVisible (false);
      (CurrentSelection).removeAll ();
    }
    if ((SelectionHead).getItemCount () != 0) {
      (SelectionHead).setVisible (false);
      (SelectionHead).removeAll ();
    }
    repack ();
    (CurrentSelection).setVisible (true);
    (SelectionHead).setVisible (true);
    ObjectMenu = null;
    DisplaySession = false;
    enableMenus ();
    Alerts.appendLine ("done");
  }
  synchronized void addObjectMenu (java.awt.List m, SObject o) {
    int ix = 0, sl;
    String s;
    if ((s = o.SName) != null) {
      // This should always be the case
      if ((sl = s.length ()) > OBL_SNAME) sl = OBL_SNAME;
      s.getChars (0, sl, chbuf, ix);
      ix += sl;
      if (o.BName != null || o.NName != null) {
        while (ix < OBL_SNAME+1) chbuf [ix++] = ' ';
        if ((s = o.BName) != null) {
          // This is not mandatory
          if ((sl = s.length ()) > OBL_BNAME) sl = OBL_BNAME;
          s.getChars (0, sl, chbuf, ix);
          ix += sl;
        }
        if ((s = o.NName) != null) {
          while (ix < OBL_SNAME + OBL_BNAME + 2) chbuf [ix++] = ' ';
          if ((sl = s.length ()) > OBL_NNAME) sl = OBL_NNAME;
          s.getChars (0, sl, chbuf, ix);
          ix += sl;
        }
      }
    } else {
      // This cannot happen
      chbuf [ix++] = ' ';
    }
    // Item are added in the reverse order
    m.addItem (new String (chbuf, 0, ix));
  }
  synchronized void processMenuPhrase () {
    SObject so;
    disableMenus ();
    while (IB.peekOctet () == PRT.PH_TXT) {
      // More items. They come as triplets: SName, BName, NName.
      so = new SObject (this);
      IB.getOctet ();
      // SName
      if (IB.peekOctet () != 0)
        so.SName = IB.getText ();
      else
        // Don't allocate room for empty names. In fact, SName cannot be empty,
        // but...
        IB.getOctet ();
      if (IB.peekOctet () != PRT.PH_TXT) break; // This cannot happen
      IB.getOctet ();
      // BName
      if (IB.peekOctet () != 0) so.BName = IB.getText (); else IB.getOctet ();
      if (IB.peekOctet () != PRT.PH_TXT) break; // Cannot happen
      IB.getOctet ();
      // NName
      if (IB.peekOctet () != 0) so.NName = IB.getText (); else IB.getOctet ();
    }
    // We have read the menu. Now we have to setup the list on the screen.
    (SelectionHead).setVisible (false);
    (CurrentSelection).setVisible (false);
    if ((SelectionHead).getItemCount () != 0) (SelectionHead).removeAll ();
    if ((CurrentSelection).getItemCount () != 0) (CurrentSelection).removeAll ();
    SmurphList = null;
    CurrentSelection.setBackground (OMENUCOLOR);
    SelectionHead.setBackground (OWNERCOLOR);
    if (ObjectMenu != null) {
      for (so = ObjectMenu; so.next != null; so = so.next)
        addObjectMenu (CurrentSelection, so);
      // The ordering of items is reversed, so the first arrived item (the
      // owner) is the last item on the list
      addObjectMenu (SelectionHead, so);
    }
    (SelectionHead).setVisible (true);
    (CurrentSelection).setVisible (true);
    repack ();
    enableMenus ();
    Alerts.appendLine ("done");
  }
  // This is a function to make sure things are synchronized
  synchronized void addNewTemplate () { new Template (this, IB); }
  public void runReceiver () {
    // This is the primary function of the receiver thread
    int Phrase;
    boolean ReceivingTemplates = false;
    while (Main.ReceiverThread == Thread.currentThread ()) {
      // Read one phrase
      IB.reset ();
      if ((Phrase = IB.getOctet ()) == 0) {
        // Error, disconnection, garbage
        Alerts.displayLine ("Receiver disconnected");
        if (DisplaySession) terminateDisplaySession ();
        enableMenus ();
        SI.closeSocket ();
        Main.ReceiverThread = null;
        continue;
      }
      if (Phrase == PRT.PH_BLK) {
        // This is a simulator block
        NotUpdated = true;
        while (IB.more ()) {
          // In this loop we process various phrases of the simulator
          // block
          Phrase = IB.getOctet ();
          switch (Phrase) {
            case PRT.PH_BPR:
              // The header: it starts with the precision, which we ignore
              if (!DisplaySession) {
                DisplaySession = true;
                Main.startDisplayThread ();
                Alerts.appendLine ("done");
                Alerts.displayLine ("Waiting for object menu ... ");
                enableMenus ();
                IB.getInt ();
                NStations = IB.getInt ();
                DisplayInterval = IB.getInt ();
                continue;
              }
              break; // This means "illegal phrase"
            case PRT.PH_MEN:
              if (!DisplaySession) break;
              Templates = null;
              ObjectMenu = null;
              processMenuPhrase ();
              continue;
            case PRT.PH_TPL:
              // This is a new template
              if (!DisplaySession) break;
              if (!ReceivingTemplates) {
                Alerts.displayLine ("Receiving templates - wait ... ");
                ReceivingTemplates = true;
              }
              addNewTemplate ();
              continue;
            case PRT.PH_ERR:
              getError ();
              continue;
            case PRT.PH_WND:
              if (!DisplaySession) break;
              windowUpdate ();
              continue;
            case PRT.PH_STP:
              if (!DisplaySession) break;
              processStep ();
              continue;
            case PRT.PH_REM:
              if (!DisplaySession) break;
              // Forced window removal
              windowRemove ();
              continue;
            case PRT.PH_EXI:
              Alerts.displayLine ("Execution terminated");
            case PRT.PH_END:
              if (DisplaySession) terminateDisplaySession ();
              enableMenus ();
              SI.closeSocket ();
              Main.ReceiverThread = null;
              return;
            default:
              break; // Illegal phrase
          } // switch
          illegalPhrase (Phrase);
          enableMenus ();
          break; // This means "continue outer loop"
        } // while
        if (ReceivingTemplates) {
          ReceivingTemplates = false;
          Alerts.appendLine ("done");
        }
        continue;
      }
      // Not a block
      if (Phrase == PRT.PH_ERR) {
        // Error
        getError ();
        // If we are in the middle of a display session, this error must
        // be serious, because it has arrived in a special monitor block.
        // I don't think this can happen, perhaps except for simulator
        // disconnection.
        if (DisplaySession) terminateDisplaySession ();
        SI.closeSocket ();
        Main.ReceiverThread = null;
        enableMenus ();
        return;
      }
      if (Phrase == PRT.PH_MBL) {
        // This is a monitor block
        if (DisplaySession) {
          Alerts.displayLine ("Illegal MBL phrase in display session");
          IB.ignore ();
          enableMenus ();
          continue;
        }
        if ((Phrase = IB.peekOctet ()) == PRT.PH_MEN || Phrase == PRT.PH_STA) {
          // This is a list of active Smurphs or a status response
          if (Phrase == PRT.PH_MEN)
            readSmurphList ();
          else
            readStatusResponse ();
          SI.closeSocket ();
          Main.ReceiverThread = null;
          enableMenus ();
          return;
        }
        if (Phrase == PRT.PH_ERR) {
          getError ();
          SI.closeSocket ();
          Main.ReceiverThread = null;
          enableMenus ();
          return;
        }
        if (Phrase == 0) {
          // This is a void response to a list request
          Alerts.appendLine ("nothing there");
          SI.closeSocket ();
          Main.ReceiverThread = null;
          enableMenus ();
          return;
        }
      }
      // Something weird, perhaps we shouldn't just ignore it ?
      illegalPhrase (Phrase);
      IB.ignore ();
      enableMenus ();
      Main.ReceiverThread = null;
      return;
    }
  }
  synchronized void windowUpdate () {
    // Processes a window update phrase
    int nl, mode, sttn;
    DisplayWindow w;
    if (IB.getOctet () != PRT.PH_TXT) {
      // Some basic sanity check
      IB.ignore ();
      return;
    }
    // Credit control
    if (NotUpdated) {
      NotUpdated = false;
      LastUpdateTime = System.currentTimeMillis ();
    }
    // End credit control
    nl = IB.getText (SANE, chbuf);
    mode = IB.getInt ();
    sttn = IB.getInt ();
    for (w = DisplayWindowList; w != null; w = w.next) {
      // Locate the window
      if (nameCompare (w.SName, nl) && w.Station == sttn &&
                                                        w.Temp.mode == mode) {
        // We have got it
        w.updateWindow ();
        return;
      }
    }
    // We have to ignore the WND phrase, because the window doesn't exist
    IB.getInt (); // Starting item
    while (IB.more ()) {
      switch (IB.peekOctet ()) {
        case PRT.PH_INM:
        case PRT.PH_BNM:
        case PRT.PH_TXT:
        case PRT.PH_FNM:
          IB.skipNumberOrText ();
          continue;
        case PRT.PH_REG:
          IB.skipRegion ();
          continue;
        default:
          return;
      }
    }
  }
  void illegalPhrase (int ph) {
    if (ph == PRT.PH_ERR)
      // This may arrive spontaneously
      getError ();
    else
      Alerts.displayLine ("Illegal phrase: "+ph);
    IB.ignore ();
  }
  void readSmurphList () {
    // Reads the list of active experiments arriving from the monitor
    int NLines = 0, Handle, PId;
    String sb;
    (CurrentSelection).setVisible (false);
    (SelectionHead).setVisible (false);
    if ((CurrentSelection).getItemCount () != 0) (CurrentSelection).removeAll ();
    CurrentSelection.setBackground (SMLISTCOLOR);
    if ((SelectionHead).getItemCount () != 0) (SelectionHead).removeAll ();
    SelectionHead.setBackground (SMLIHDCOLOR);
    SelectionHead.addItem (SML_HDR);
    SelectionHead.deselect (0);
    SmurphList = null;
    chbuf [SML_DATE] = chbuf [SML_DATE+SML_HOST+1] =
      chbuf [SML_DATE+SML_HOST+SML_PARG+2] = ' ';
    while (IB.more ()) {
      if ((Handle = IB.getOctet ()) != PRT.PH_MEN) {
        illegalPhrase (Handle);
        break;
      }
      Handle = IB.getInt ();
      new SmList (this, Handle);
      IB.getInt (SML_PRID, chbuf, SML_DATE+SML_HOST+SML_PARG+3, false);
      IB.getText (SML_HOST, chbuf, SML_DATE+1, true);
      IB.getText (SML_PARG, chbuf, SML_DATE + SML_HOST + 2, true);
      IB.getText (SML_DATE, chbuf, 0, true);
      sb = new String (chbuf, 0, SML_TOTL);
      CurrentSelection.addItem (sb);
    }
    (CurrentSelection).setVisible (true);
    (SelectionHead).setVisible (true);
    repack ();
    Alerts.appendLine ("done");
  }
  void readStatusResponse () {
    float cputime;
    String s;
    int sl, ix;
    "RM = " . getChars (0, 5, chbuf, 0);
    while (IB.more ()) {
      if ((sl = IB.getOctet ()) != PRT.PH_STA) {
        illegalPhrase (sl);
        return;
      }
      if (IB.peekOctet () != PRT.PH_INM) {
        // If the simulator gets disconnected in the meantime
        Alerts.displayLine ("Disconnected");
        IB.ignore ();
        return;
      }
      // Skip the handle that we ignore for now
      IB.getInt (10, chbuf, 5, true);
      IB.getInt (10, chbuf, 5, true);
      for (ix = 10+5; chbuf [ix-1] == ' '; ix--);
      // This is where the message number ends
      ", ST = " . getChars (0, 7, chbuf, ix);
      ix += 7;
      IB.getInt (15, chbuf, ix, true);
      for (ix += 15; chbuf [ix-1] == ' '; ix--);
      // This is where the simulated time ends
      ", CP = " . getChars (0, 7, chbuf, ix);
      ix += 7;
      // This is in milliseconds, so we convert it to seconds
      cputime = (float) (((double) IB.getInt ()) / 1000.0);
      sl = (s = Float.toString (cputime)) . length ();
      s . getChars (0, sl, chbuf, ix);
      ix += sl;
      s = new String (chbuf, 0, ix);
      Alerts.displayLine (s);
    }
  }
  synchronized void advanceDialogs () {
    // In principle, this can handle multiple simultaneous dialogs, although
    // only one dialog at a time can in fact show up at present.
    if (DD != null) {
      // Handle display dialog
      if (DD.Done) {
        if (!DD.Cancel)
          createDisplayWindow (DD.SO, DD.SelectedTemplate, DD.Status);
        (DD).setVisible (false);
        DD.dispose ();
        DD = null;
        STemplates = null;
      }
    }
    if (SD != null) {
      // Handle step dialog
      if (SD.Done) {
        if (!SD.Cancel)
          stepWindowDelayed ();
        (SD).setVisible (false);
        SD.dispose ();
        SD = null;
      }
    }
    if (ID != null) {
      // Handle interval dialog
      if (ID.Done) {
        if (!ID.Cancel)
          resetDisplayInterval ();
        (ID).setVisible (false);
        ID.dispose ();
        ID = null;
      }
    }
    // Do this only if there are no dialogs pending
    if (DD == null && SD == null && ID == null) {
      DialogPending = false;
      enableMenus ();
    }
  }
  synchronized void sendCredit () {
    OB.reset (PRT.PH_CRE);
    sendBuffer (false);
  }
  public void runDisplay () {
    // This is the main loop of the root window
    DisplayWindow dw;
    long CTime;
    LastBlinkTime = LastUpdateTime = 0;
    while ((DisplaySession || DisplayWindowList != null) &&
     Main.DisplayThread == Thread.currentThread ()) {
      // We are still alive
      for (dw = DisplayWindowList; dw != null; dw = dw.next) {
        if (dw.RequestBoundaryUpdate) updateItemBoundaries (dw);
        dw.repaint ();
      }
      try {
        try {
          Thread.sleep (DISPLAYDELAY);
        } catch (InterruptedException e) {
          if (Main.DisplayThread == null)
                return;
          break;
        }
      } catch (ThreadDeath e) {
        Main.DisplayThread = null;
        return;
      }
      if ((CTime = System.currentTimeMillis ()) - LastBlinkTime > BLINKTIME) {
        if (BlinkON) BlinkON = false; else BlinkON = true;
        LastBlinkTime = CTime;
      }
      if (DisplayWindowList != null && CTime - LastUpdateTime > CREDITDELAY) {
        sendCredit ();
        LastUpdateTime = CTime;
      }
      // Process dialogs
      if (DialogPending) advanceDialogs ();
      // Check for active connection
      if (IB.error || OB.error) {
        terminateDisplaySession ();
        SI.closeSocket ();
        Main.ReceiverThread = null;
        enableMenus ();
        return;
      }
    }
    Main.DisplayThread = null;
  }
}
// ------------------------------------------------------------------------- //
class AlertArea extends TextArea {
  final static int NROWS = 4; // The number of visible rows
  final static int NCOLS = 40; // Visible columns
  final static int NLINES = 24; // The total number of lines
  final static Color BACKGROUND = new Color (238, 210, 238);
  final static Color FOREGROUND = new Color (205, 92, 92);
  int LPos [] = new int [NLINES]; // Line endings for scrolling
  RootWindow RT; // Where we come from
  AlertArea (RootWindow op) {
    super ("", NROWS, NCOLS, SCROLLBARS_HORIZONTAL_ONLY);
    int i;
    RT = op;
    for (i = 0; i < NLINES; i++) LPos [i] = -1; // No lines
    setBackground (BACKGROUND);
    setForeground (FOREGROUND);
    setEditable (false);
  }
  public synchronized void displayLine (String s) {
    int i, loc, sp;
    for (loc = NLINES-1; (loc >= 0) && (LPos [loc] < 0); loc--);
    loc++;
    if (loc == NLINES) {
      // Scroll the first line out
      (this).replaceRange ("", 0, sp = LPos [0] + 1);
      for (i = 0; i < NLINES-1; i++) LPos [i] = LPos [i+1] - sp;
      loc = NLINES-1;
    }
    sp = (loc == 0) ? 0 : LPos [loc-1];
    (this).append ("\n");
    (this).append (s);
    // We assume that the text doesn't have '\n' at the end
    LPos [loc] = sp + s.length () + 1;
  }
  public synchronized void appendLine (String s) {
    int i, loc, sp;
    for (loc = NLINES-1; (loc >= 0) && (LPos [loc] < 0); loc--);
    if (loc < 0) {
      // No line to append to -- this shouldn't happen
      displayLine (s);
      return;
    }
    (this).append (s);
    LPos [loc] += s.length ();
  }
}
// ------------------------------------------------------------------------- //
class OutputBuffer {
  // Initial size of the output buffer
  final static int BUFSIZE = 512;
  OutputStream OS;
  int size, // Current total size
          fill, // Current fill
          lastsdscr;
  boolean lastshalf, // For descriptor processing
          error; // I/O error flag
  byte BlockBuffer [];
  byte digits [] = new byte [16]; // For encoding numbers
  public OutputBuffer () {
    BlockBuffer = new byte [size = BUFSIZE];
    fill = 0;
    OS = null;
  }
  void grow () {
    byte bb [] = BlockBuffer;
    int i;
    BlockBuffer = new byte [size += size];
    for (i = 0; i < bb.length; i++) BlockBuffer [i] = bb [i];
  }
  public synchronized void disconnect () {
    OS = null;
  }
  public synchronized boolean connect (Socket s) {
    error = false;
    try {
      OS = s.getOutputStream ();
    } catch (IOException e) {
      error = true;
    }
    return error;
  }
  public void sendOctet (int oct) {
    if (fill == size) grow ();
    BlockBuffer [fill++] = (byte) oct;
  }
  public void sendChar (char c) {
    if (fill == size) grow ();
    BlockBuffer [fill++] = (byte)(c & 0x7f);
  }
  public void reset (int phrase) {
    BlockBuffer [0] = (byte) phrase;
    fill = 4;
  }
  public synchronized boolean flush () {
    // Set the length field
    BlockBuffer [1] = (byte) ((fill >> 16) & 0xff);
    BlockBuffer [2] = (byte) ((fill >> 8) & 0xff);
    BlockBuffer [3] = (byte) ( fill & 0xff);
    if (OS != null && !error) {
      try {
        OS.write (BlockBuffer, 0, fill);
      } catch (IOException e) {
        error = true;
      }
    }
    return error;
  }
  void sendDescriptor (int d) {
    if (lastshalf) {
      sendOctet (lastsdscr | (d & 017));
      lastshalf = false;
    } else {
      lastshalf = true;
      lastsdscr = (d & 017) << 4;
    }
  }
  void flushDescriptor () {
    if (lastshalf) {
      sendOctet (lastsdscr);
      lastshalf = false;
    }
  }
  public void sendInt (int val) {
    int ndigits;
    boolean sig;
    sendOctet (PRT.PH_INM);
    lastshalf = false;
    if (val >= 0) {
      sig = false;
      val = -val;
    } else
      sig = true;
    for (ndigits = 0; val != 0; ) {
      digits [ndigits++] = (byte)(-(val % 10));
      val /= 10;
    }
    while (ndigits-- > 0) sendDescriptor (digits [ndigits]);
    sendDescriptor (sig ? PRT.DSC_ENN : PRT.DSC_ENP);
    flushDescriptor ();
  }
  public void sendInt (String val) {
    // This sends an integer that has been already converted to string. I am
    // not sure whether we need it at all (we definitely need one for the
    // opposite direction), but let's just do it as an exercise.
    int ix, len = val.length ();
    boolean neg = false;
    char nc;
    sendOctet (PRT.PH_INM);
    lastshalf = false;
    for (ix = 0; ix < len; ix++) {
      nc = val.charAt (ix);
      if ((Character.isWhitespace (nc))) continue;
      if (nc == '-') neg = true;
      // As you can see, this code is somewhat heuristic. We assume that the
      // input is OK.
      if (Character.isDigit (nc)) {
        // Expedite the next descriptor
        nc -= '0';
        sendDescriptor ((int)nc);
      }
    }
    sendDescriptor (neg ? PRT.DSC_ENN : PRT.DSC_ENP);
    flushDescriptor ();
  }
  public void sendDigits (int tp, byte v [], int len) {
    // This sends a nonnegative integer represented as a sequence of digits.
    int ix;
    sendOctet (tp); // This is either PH_INM or PH_BNM
    lastshalf = false;
    for (ix = 0; ix < len; ix++) sendDescriptor ((int)(v[ix]));
    sendDescriptor (PRT.DSC_ENP);
    flushDescriptor ();
  }
  public void sendText (String s) {
    int ix, len = s.length ();
    sendOctet (PRT.PH_TXT);
    for (ix = 0; ix < len; ix++) sendChar (s.charAt (ix));
    sendOctet (0);
  }
}
// ------------------------------------------------------------------------- //
class InputBuffer {
  // The size of the input buffer. Unlike the output buffer, the input
  // buffer doesn't grow.
  final static int BUFSIZE = 8192;
  InputStream IS;
  int fill, // Current fill
          length, // Remaining length of current spurt
          ptr, // Current out pointer
          dscrlast;
  boolean dscrhalf, // For descriptor processing
          error; // I/O error flag
  byte BlockBuffer [];
  char digits [] = new char [128]; // For encoding numbers
  int ndigits; // The current fill of digits
  public InputBuffer () {
    BlockBuffer = new byte [BUFSIZE];
    ptr = fill = length = 0;
    IS = null;
  }
  public void disconnect () {
    IS = null;
  }
  public synchronized boolean connect (Socket s) {
    error = false;
    try {
      IS = s.getInputStream ();
    } catch (IOException e) {
      error = true;
    }
    return error;
  }
  public boolean more () { return length > 0 && !error; }
  synchronized void getMore () {
    int nr;
    if (error) return;
    if (length == 0) {
      // We are starting a new spurt
      while (length < 4) {
        // Read the header. It will tell you how much you can safely expect.
        try {
          if ((nr = IS.read (BlockBuffer, length, 4-length)) == 0) {
            error = true;
            return;
          }
          length += nr;
        } catch (IOException e) {
          error = true;
          return;
        }
      }
      // Determine the length
      length = ((((int) (BlockBuffer [1])) & 0xff) << 16) +
               ((((int) (BlockBuffer [2])) & 0xff) << 8) +
                (((int) (BlockBuffer [3])) & 0xff) - 3;
      // Length is set to length - 3 to eliminate the three length bytes. This
      // way the first read will only read the phrase byte.
      fill = 1;
      ptr = 0;
      return;
    }
    // Note that the first time around we will effectively read just one
    // byte, i.e., the phrase indicator. This read will set the length for
    // subsequent reads.
    fill = (length > BUFSIZE) ? BUFSIZE : length;
    for (ptr = 0; ptr < fill; ) {
      try {
        if ((nr = IS.read (BlockBuffer, ptr, fill-ptr)) == 0) {
          error = true;
          return;
        }
        ptr += nr;
      } catch (IOException e) {
        error = true;
        return;
      }
    }
    ptr = 0;
  }
  public void reset () {
    // Reset for reading a new spurt
    while (more ()) getOctet ();
    getMore ();
  }
  public void ignore () {
    while (more ()) getOctet ();
  }
  public int getOctet () {
    // Make sure to always return something
    if (length == 0 || error) return 0;
    if (ptr == fill) {
      getMore ();
      if (error) return 0;
    }
    length--;
    return ((int)(BlockBuffer [ptr++])) & 0xff;
  }
  public int peekOctet () {
    if (length == 0 || error) return 0;
    if (ptr == fill) {
      getMore ();
      if (error) return 0;
    }
    return ((int)BlockBuffer [ptr]) & 0xff;
  }
  int getDescriptor () {
    int rslt;
    if (dscrhalf) {
      // Take the second half
      dscrhalf = false;
      rslt = dscrlast & 0x0f;
    } else {
      // Get a new byte
      dscrlast = getOctet ();
      dscrhalf = true;
      rslt = (dscrlast >> 4) & 0x0f;
    }
    return (rslt);
  }
  public int getInt () {
    int res, des;
    if (peekOctet () == PRT.PH_INM) getOctet ();
    res = 0; dscrhalf = false;
    while (dscrhalf || more ()) {
      if ((des = getDescriptor ()) == PRT.DSC_ENP) // Positive number
        return (-res);
      else if (des == PRT.DSC_ENN) // Negative number
        return (res);
      else if (des > 9) // Spurt error
        // This is an error and it should never happen
        des = 0;
      res = res * 10 - des;
    }
    return (res);
  }
  void ovflw (int len) {
    // Overflow fill
    for (int i = 0; i < len; i++) digits [i] = '*';
    ndigits = len;
  }
  int esize (int ex) {
    // Determines the amount of space taken by the exponent
    int sz;
    if (ex < 0) {
      sz = 3;
      ex = -ex;
    } else
      sz = 2;
    if (ex > 9) {
      sz++;
      if (ex > 99) {
        sz++;
        if (ex > 999) sz++;
      }
    }
    return sz;
  }
  void expset (int pos, int ex) {
    // Plug an exponent into the number at location 'pos'
    int nd;
    digits [pos++] = 'E';
    if (ex < 0) {
      digits [pos++] = '-';
      ex = -ex;
    }
    if (ex < 10) {
      digits [pos++] = (char) ('0' + ex);
    } else if (ex < 100) {
      digits [pos++] = (char) ('0' + (ex / 10)); ex %= 10;
      digits [pos++] = (char) ('0' + ex);
    } else if (ex < 1000) {
      digits [pos++] = (char) ('0' + (ex / 100)); ex %= 100;
      digits [pos++] = (char) ('0' + (ex / 10)); ex %= 10;
      digits [pos++] = (char) ('0' + ex);
    } else {
      digits [pos++] = (char) ('0' + (ex / 1000)); ex %= 1000;
      digits [pos++] = (char) ('0' + (ex / 100)); ex %= 100;
      digits [pos++] = (char) ('0' + (ex / 10)); ex %= 10;
      digits [pos++] = (char) ('0' + ex);
    }
    ndigits = pos;
  }
  void fitInt (int len) {
    // Fits the absolute integer from 'digits' into no more than 'len' positions
    int exponent, ies, idig;
    if (ndigits <= len) return;
    while (true) {
      // Initial exponent size ('Ex')
      idig = 2;
      while ((ies = esize (exponent=ndigits-len+idig)) != idig) idig = ies;
      if (len < ies + 1) {
        // We can't even fit a single digit
        ovflw (len);
        return;
      }
      // We can fit this many digits
      idig = len - ies;
      if (digits [idig] >= '5') {
        // Now do the rounding
        for (ies = idig-1; ies >= 0; ies--) {
          if ((digits [ies] = (char) (digits [ies] + 1)) <= '9')
            break;
          else
            digits [ies] = '0';
        }
        if (ies < 0) {
          // The number of digits has grown. This means that our
          // number looks precisely like this: 10..0
          for (ies = idig; ies > 0; ies--) digits [ies] = '0';
          digits [0] = '1';
          ndigits++;
          // The rest will be taken care of by the next iteration of the
          // outer loop
        } else
          break;
      } else
        break; // Goto-less programming is certainly fun, isn't it?
    }
    // Now we are ready to plug in the exponent
    expset (idig, exponent);
  }
  void fitFloat (int len, int exp) {
    int dep, lz, lf, ies;
    // Fits the absolute float from 'digits' into no more than 'len' positions
    while (true) {
      // We need this loop to redo things if the number gets changed due to
      // a round-up
      if (exp >= 0 && ndigits + exp <= len) {
        // This looks like an integer that can be fit easily
        while (exp-- > 0) digits [ndigits++] = '0';
        // Note: we assume that len is always sane. Sanity checks should be
        // done at the moment when the template is decoded.
        return;
      }
      if (exp < 0 &&
       // Exponent is negative
       (dep = ndigits + exp) < len &&
       // The decimal point appears in the visible region
       ndigits >= -exp - 2
       // No excessive significance loss. Note that we are going to lose
       // at least 2 slots for an exponent, if we go for it.
                                              ) {
        // Here we go for a decimal point without an exponent.
        if (dep < 0) {
          // We need a few leading zeros
          lz = -dep;
          dep = 0;
        } else
          lz = 0; // No leading zeros
        // This is the index of the last digit that is going to show up + 1
        if ((lf = len - lz - 1) < ndigits) {
          // A round up may be required?
          if (digits [lf] >= '5') {
            // It is, I'm sorry I asked
            for (ies = lf-1 ; ies >= 0; ies--) {
              if ((digits [ies] = (char) (digits [ies] + 1)) <= '9')
                break;
              else
                digits [ies] = '0';
            }
            if (ies < 0) {
              // The number of digits has grown. We are going to redo
              // everything. The number of significant digits
              // we will end up displaying will be no more than we decided
              // earlier (even if we switch to exponent format) so we
              // can round the number now and leave it this way for our
              // successors.
              for (ies = lf; ies > 0; ies--) digits [ies] = '0';
              digits [0] = '1';
              exp += ndigits - lf;
              ndigits = lf+1;
              continue;
            }
          }
        } else
          lf = ndigits;
        // Insert the dot
        ndigits = lf + lz + 1;
        for (ies = lf; ies > dep; ies--) digits [ies+lz] = digits [ies-1];
        while (lz-- > 0) digits [ies+lz] = '0';
        digits [ies] = '.';
        return;
      }
      break;
    }
    while (true) {
      // We need an exponent. Perhaps we should be careful about significance,
      // but I think that a number like this is going to be much more legible
      // if there is a dot in some fixed place. The way we do it, the dot is
      // after the first significant digit.
      exp += ndigits - 1; // Dot after first digit
      dep = esize (exp);
      // We can show that many digits
      if ((lf = len - dep - 1) < 1) {
        ovflw (len);
        return;
      }
      if (lf >= ndigits)
        lf = ndigits;
      else {
        // Round up
        if (digits [lf] >= '5') {
          for (ies = lf-1; ies >= 0; ies--) {
            if ((digits [ies] = (char) (digits [ies] + 1)) <= '9')
              break;
            else
              digits [ies] = '0';
          }
          if (ies < 0) {
            for (ies = lf; ies > 0; ies--) digits [ies] = '0';
            digits [0] = '1';
            exp += ndigits - lf;
            ndigits = lf+1;
            continue;
          }
        }
      }
      // Insert the dot
      for (ies = lf; ies > 1; ies--) digits [ies] = digits [ies-1];
      digits [1] = '.';
      // The following also sets ndigits
      expset (lf+1, exp);
      return;
    }
  }
  public void skipNumberOrText () {
    // Removes a number from the input stream 
    getOctet ();
    while (more () && peekOctet () < PRT.PH_MIN) getOctet ();
  }
  public void skipRegion () {
    // This ignores a region update in the input stream
    while (more () && getOctet () != PRT.PH_REN);
  }
  public void getInt (int len, char [] res, int off, boolean left) {
    // Returns character representation of the integer number
    int rr, des, ll = len;
    boolean neg;
    ndigits = 0;
    neg = false;
    if ((rr = peekOctet ()) == PRT.PH_INM || rr == PRT.PH_BNM) getOctet ();
    dscrhalf = false;
    while (dscrhalf || more ()) {
      // Get all the digits into 'digits'
      if ((des = getDescriptor ()) == PRT.DSC_ENP) // Positive number
        break;
      else if (des == PRT.DSC_ENN) { // Negative number
        neg = true;
        break;
      } else if (des > 9) // Spurt error
        // This is an error and it should never happen
        des = 0;
      // We've got a new digit
      if (ndigits < 120)
        // This is to maintain sanity -- all numbers will be shorter
        // than that.
        digits [ndigits++] = (char) (des + '0');
    }
    // Make sure there is at least one digit
    if (ndigits == 0) digits [ndigits++] = '0';
    // Reserve room for the sign
    if (neg) ll--;
    fitInt (ll);
    if (left)
      rr = 0; // This is where we start
    else {
      rr = ll - ndigits;
      for (des = 0; des < rr; des++) res [off++] = ' ';
    }
    if (neg) {
      res [off++] = '-';
      rr++;
    }
    for (des = 0; des < ndigits; des++) res [off++] = digits [des];
    rr += ndigits;
    // If left justified
    while (rr++ < len) res [off++] = ' ';
  }
  public void getFloat (int len, char [] res, int off, boolean left) {
    // Character representation of a floating point number
    int rr, des, ad, exp, ll = len;
    boolean neg, afterdot;
    neg = afterdot = false;
    ndigits = ad = exp = 0;
    if (peekOctet () == PRT.PH_FNM) getOctet ();
    dscrhalf = false;
    while (dscrhalf || more ()) {
      // Collect all digits of the mantissa
      switch (des = getDescriptor ()) {
        case PRT.DSC_ENP: // End of positive number
          break;
        case PRT.DSC_ENN: // End of negative number
          neg = true;
          break;
        case PRT.DSC_DOT:
          afterdot = true;
          continue;
        case PRT.DSC_EXP: // The exponent (this must be decoded)
          exp += getInt ();
          continue; // This will be followed by ENP or ENN
        default:
          // This looks like a digit
          if (des > 9) des = 0; // Sanity maintenance
          if (ndigits < 120) {
            digits [ndigits++] = (char) ('0' + des);
            if (afterdot) exp--;
          } else {
            // Should never get here
            if (!afterdot) exp++;
          }
          continue;
      }
      break;
    }
    // Make sure there is at least one digit
    if (ndigits == 0) digits [ndigits++] = '0';
    if (neg) ll--;
    fitFloat (ll, exp);
    // Now adjust the result and pad it with blanks
    if (left)
      rr = 0; // This is where we start
    else {
      rr = ll - ndigits;
      for (des = 0; des < rr; des++) res [off++] = ' ';
    }
    if (neg) {
      res [off++] = '-';
      rr++;
    }
    for (des = 0; des < ndigits; des++) res [off++] = digits [des];
    rr += ndigits;
    // If left justified
    while (rr++ < len) res [off++] = ' ';
  }
  public void getText (int len, char [] res, int off, boolean left) {
    int go, ix, di;
    if (peekOctet () == PRT.PH_TXT) getOctet ();
    len += (ix = off);
    while (more ()) {
      if ((go = getOctet ()) == 0) break;
      if (off < len) res [off++] = (char) go;
    }
    if (left) {
      while (off < len) res [off++] = ' ';
    } else {
      if ((di = len - off) > 0) {
        for (go = off-1; go >= ix; go--) res [di+go] = res [go];
        for (go = ix; go < di; go++) res [go] = ' ';
      }
    }
  }
  public int getText (int len, char [] res) {
    int go, ix, di;
    if (peekOctet () == PRT.PH_TXT) getOctet ();
    ix = 0;
    while (more ()) {
      if ((go = getOctet ()) == 0) break;
      if (ix < len) res [ix++] = (char) go;
    }
    return ix;
  }
  public String getText () {
    int ix = 0, go;
    if (peekOctet () == PRT.PH_TXT) getOctet ();
    while (more ()) {
      if ((go = getOctet ()) == 0) break;
      if (ix < 128) digits [ix++] = (char) go;
    }
    return new String (digits, 0, ix);
  }
}
// ------------------------------------------------------------------------- //
final class PRT {
  // Declares protocol constants (borrowed from display.h)
  /* ------------ */
  /* Phrase bytes */
  /* ------------ */
  final static int PH_BLK = 255; // Start of update block
  final static int PH_MEN = 254; // Menu request
  final static int PH_INM = 253; // Integer number
  final static int PH_FNM = 252; // Floating-point number
  final static int PH_BNM = 251; // BIG number
  final static int PH_TXT = 250; // Text
  final static int PH_STA = 249; // Status request
  final static int PH_STP = 248; // Step
  final static int PH_DIN = 247; // Display interval
  final static int PH_BPR = 246; // Initial parameters
  final static int PH_DRQ = 245; // Display request
  final static int PH_REM = 244; // Remove an object
  final static int PH_WND = 243; // Request window
  final static int PH_EWN = 242; // Cancel window
  final static int PH_CRE = 241; // Credit message
  final static int PH_REG = 240; // Region
  final static int PH_REN = 239; // Region end
  final static int PH_SLI = 238; // Region line segment
  final static int PH_ERR = 237; // Error indication
  final static int PH_END = 236; // Terminate display
  final static int PH_EXI = 235; // Terminate execution
  final static int PH_SIG = 234; // Signature block
  final static int PH_UPM = 233; // Up menu request
  final static int PH_REL = 232; // Relative step delay
  final static int PH_ESM = 231; // Exit step mode
  final static int PH_TRN = 230; // Truncate
  final static int PH_SRL = 228; // Step release
  final static int PH_TPL = 227; // Template
  final static int PH_MBL = 226; // Monitor block
  final static int PH_MIN = 226; // The current minimum
  // ------------------------------------------------- //
  // Note: minimum phrase code is 224, i.e., 1110 0000 //
  // ------------------------------------------------- //
  /* ----------------------------------- */
  /* Numerical descriptors special codes */
  /* ----------------------------------- */
  final static int DSC_DOT = 10; // Decimal point
  final static int DSC_EXP = 11; // Exponent
  final static int DSC_ENN = 12; // End of negative number
  final static int DSC_ENP = 13; // End of positive number
  // Note:  descriptor  codes  14  and 15 are illegal. No legal byte can
  //        look like a phrase code. Well, probably we can get away with the
  //        assumption that no legal data byte looks like PH_MEN, but just
  //        in case ...
  /* ----------- */
  /* Error codes */
  /* ----------- */
  final static int DERR_ILS = 0; // Illegal spurt (formal error)
  final static int DERR_NSI = 1; // Negative display interval
  final static int DERR_ONF = 2; // Object not found
  final static int DERR_NMO = 3; // Nonexistent mode
  final static int DERR_ISN = 4; // Illegal station number
  final static int DERR_CUN = 5; // No window to unstep
  final static int DERR_NOT = 6; // Note
  final static int DERR_UNV = 7; // Not available
  final static int DERR_NRP = 8; // Response timeout
  final static int DERR_DSC = 9; // Disconnected
  /* ------------- */
  /* DSD signature */
  /* ------------- */
  final static String SERVRSIGN = "dservers";
  final static int SIGNLENGTH = 8;
}
