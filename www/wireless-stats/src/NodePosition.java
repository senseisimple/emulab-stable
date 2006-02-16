/*
 * NodePosition.java
 *
 * Created on February 1, 2006, 9:41 PM
 *
 * To change this template, choose Tools | Options and locate the template under
 * the Source Creation and Management node. Right-click the template and choose
 * Open. You can then make changes to the template in the Source Editor.
 */

/**
 *
 * @author david
 */
public interface NodePosition {
    public float[] getPosition(String nodeName);
    
    public java.awt.Point getPoint(String nodeName);
}
