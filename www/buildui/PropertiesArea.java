import java.awt.*;
import java.awt.event.*;
import java.util.*;

abstract synchronized class PropertiesArea extends Panel implements TextListener, ActionListener {

    private Panel child;

    private GridBagLayout layout;
    private GridBagLayout childLayout;
    private GridBagConstraints gbc;
    private MagicTextField nameEdit;
    private boolean started;
    private boolean childVisible;

    private Expando expando;

    private Vector currentThingees;

    private static Color darkBlue;

    public boolean isStarted() { return started; }

    static {
	darkBlue  = new Color( 0.3f, 0.3f, 0.5f );
    }

    private void setVisibleAll( boolean b ) {
	childVisible = b;
	validate();
	invalidate();

	getParent().doLayout();
	//child.setVisible(b);
	//doLayout();
    }
	
    public void hideProperties() {
	setVisibleAll( false );
        expando.setState( false );
    }

    public void showProperties() {
	setVisibleAll( true );
	expando.setState( true );
    }

    public void actionPerformed( ActionEvent e ) {
	if (e.getSource() == expando) {
	    if (e.getID() == ActionEvent.ACTION_PERFORMED) {
		setVisibleAll(0 == e.getActionCommand().compareTo("down"));
	    }
	}
    }

    private MagicTextField addField( String name ) {
	MagicTextField tf = new MagicTextField();
	Label l = new Label(name);
	l.setForeground( Color.white );
	tf.tf.setBackground( Color.white );

	childLayout.setConstraints( l, gbc );
	child.add( l );
	childLayout.setConstraints( tf.tf, gbc );
	child.add( tf.tf );
	tf.tf.addTextListener( this );

        tf.tf.setVisible( true );
	l.setVisible( true );

	return tf;
    }
    
    private Vector propertyList;

    class Property {
	public String name;
	public String def;
	public MagicTextField textField;
    }

    public void textValueChanged( TextEvent e ) {
	upload();
	if (nameEdit != null 
	    && e.getSource() == nameEdit.tf) { // hack.
	    getParent().repaint();
	}
    }

    public void addProperty( String name, String desc, String def ) {
	Property p = new Property();
	p.name = name;
	p.def = def;
	p.textField = addField( desc );
	if (0 == name.compareTo("name")) { nameEdit = p.textField; }
	propertyList.addElement( p );
    }


    public PropertiesArea() { 
	super();
	child = new Panel();

	//setVisible( false );
	setVisible( true );
	started = false;
	setBackground( darkBlue );
	child.setBackground( darkBlue );
	//child.setBackground( Color.red );
	//setBackground( Color.green );
	expando = new Expando( getName() );
	expando.addActionListener( this );
	propertyList = new Vector();
	nameEdit = null;

	//layout = new GridBagLayout();
	//setLayout( layout );

	setLayout( null );
      
	gbc = new GridBagConstraints();
	gbc.fill = GridBagConstraints.BOTH;
	gbc.weightx = 1.0;
	gbc.gridwidth = GridBagConstraints.REMAINDER;

	//layout.setConstraints( expando, gbc );


	childLayout = new GridBagLayout();
	child.setLayout( childLayout );
	//child.setVisible( true );
	//layout.setConstraints( child, gbc );
	currentThingees = null;

	expando.setVisible( true );
	child.setVisible( true );
	add( expando );
	add( child );
	childVisible = true;
    }
    
    public void doLayout()
    //    public void myLayout()
    {
	Dimension d = expando.getPreferredSize();
	expando.setSize( 640 - 480 - 16, d.height );
	expando.setLocation( 0, 0 );
	Dimension d2 = child.getPreferredSize();
	child.setLocation( 2, d.height + 2 );
	if (childVisible) {
	    child.setSize( 640-480-16-2,d2.height );
	} else {
	    child.setSize( 640-480-16-2, 0 );
	}
	setSize( 640 - 480 - 16, d.height + d2.height + 2 );
    }

    public Dimension getPreferredSize()
    {
	Dimension d = expando.getPreferredSize();
	Dimension d2 = child.getPreferredSize();
	if (childVisible) {
	    return new Dimension( 640 - 480 - 16, d.height + d2.height + 2 );
	} else {
	    return new Dimension( 640 - 480 - 16, d.height);
	}
    }

    public abstract boolean iCare( Thingee t );
    public abstract String getName();

    private void download() {
	Enumeration et = Thingee.selectedElements();
	
	currentThingees = new Vector();

	int thingsICareAbout = 0;

	boolean first = true;
	while (et.hasMoreElements()) {
	    Thingee t = (Thingee)et.nextElement();

	    if (iCare( t )) {
		Enumeration e = propertyList.elements();

		thingsICareAbout++;
		currentThingees.addElement( t );

		while (e.hasMoreElements()) {
		    Property p = (Property)e.nextElement();
		    String value = t.getProperty( p.name, p.def );
		    
		    if (first) {
			p.textField.setText( value );
		    } else if (0 != p.textField.tf.getText().compareTo( value )) {
			p.textField.setText( "<multiple>" );
		    }
		}
		first = false;
	    }
	}

	if (nameEdit != null) {
	    if (thingsICareAbout > 1) {
		nameEdit.tf.setEditable( false );
		nameEdit.tf.setBackground( darkBlue );
	    } else {
		nameEdit.tf.setEditable( true );
		nameEdit.tf.setBackground( Color.white );
	    }
	}
    }

    private void upload() {
	if (currentThingees == null) { return; }

	Enumeration et = currentThingees.elements();

	while (et.hasMoreElements()) {
	    Thingee t = (Thingee)et.nextElement();

	    //if (iCare(t)) {
		Enumeration e = propertyList.elements();
		
		while (e.hasMoreElements()) {
		    Property p = (Property)e.nextElement();
		    if (p.textField.tf.isEditable()) {
			String s = p.textField.tf.getText();
			if (0 != s.compareTo("<multiple>")) {
			    t.setProperty(p.name, s);
			}
		    }
		}
		//}
	}
    }

    public void refresh() {
	upload();
	download();
    }

    public void start() {
	started = true;	
	//setVisible( true );
	download();
	//child.setVisible( true );
	//myLayout();//doLayout();
	validate();

	invalidate();
	getParent().doLayout();
    }

    public void stop() {
	started = false;
	//setVisible( false );
	upload();

    }
};
