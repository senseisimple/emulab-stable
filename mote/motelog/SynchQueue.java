/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.util.LinkedList;

public class SynchQueue extends LinkedList {
    
    public SynchQueue() {
	super();
    }

    public synchronized void queueAdd(Object o) {
	this.addFirst(o);
    }

    public synchronized Object queueRemove() {
	return this.removeLast();
    }

    public synchronized Object peek() {
	return this.getLast();
    }

}
