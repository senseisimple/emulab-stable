/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
    public Position getPosition(String nodeName);
    public java.util.Enumeration getNodeList();
    public java.awt.Point getPoint(String nodeName);
}
