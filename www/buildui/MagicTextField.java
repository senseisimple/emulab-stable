import java.awt.*;
import java.awt.event.*;


class MagicTextField implements TextListener, FocusListener {
    public TextField tf;
    
    private boolean zapped;
    private boolean wasAuto;

    public void focusGained( FocusEvent f ) {
	tf.selectAll();
    }
    
    public void focusLost( FocusEvent f ) {
    }

    public MagicTextField() {
	tf = new TextField();
	tf.addTextListener( this );
	tf.addFocusListener( this );
	wasAuto = false;
    }

    public void setText( String t ) {
	wasAuto = 
	    (0 == t.compareTo("<auto>")) || (0 == t.compareTo("<multiple>"));
	tf.setText( t );
    }
	
    public void textValueChanged( TextEvent e ) {
	if (0 == tf.getText().compareTo("<auto>")) {
	    //wasAuto = true;
	    tf.setForeground( Color.blue );
	} else if (0 == tf.getText().compareTo("<multiple>")) {
	    tf.setForeground( Color.lightGray );
	} else {
	    tf.setForeground( Color.black );
	    if (wasAuto) {
		wasAuto = false;
		//tf.setText( "" );
		//tf.dispatchEvent( e );
	    }
	}
    }
  
};
