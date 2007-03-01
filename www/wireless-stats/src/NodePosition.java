/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

public interface NodePosition {
    public Position getPosition(String nodeName);
    public java.util.Enumeration getNodeList();
    public java.awt.Point getPoint(String nodeName);
}
