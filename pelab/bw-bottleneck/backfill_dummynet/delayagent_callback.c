/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003, 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * agent-callback.c --
 *
 *      Delay node agent callback handling.
 *
 */


/******************************* INCLUDES **************************/
#include "main.h"
#include "systemf.h"
#include <assert.h>
#include <math.h>
/******************************* INCLUDES **************************/


/******************************* EXTERNS **************************/
extern structlink_map *link_map;
extern int link_index;
extern structlink_map *old_map;
extern int old_length;
extern int s_dummy; 
extern int debug;

extern void dump_link(structlink_map *);
extern void dump_link_map(void);

/******************************* EXTERNS **************************/

/********************************FUNCTION DEFS *******************/

static struct flowspec blankfs = { "", "", 0, 0 };

/**
 * Compare two flowspecs for equality.
 *
 * @return Zero if the two flowspecs are equal.
 */
static
int flowspeccmp(struct flowspec *fs1, struct flowspec *fs2)
{
  int retval = 1;
  
  assert(fs1 != NULL);
  assert(fs2->srcport >= 0);
  assert(fs2->dstport >= 0);
  assert(fs2 != NULL);
  assert(fs2->srcport >= 0);
  assert(fs2->dstport >= 0);

  if (strcmp(fs1->dest, fs2->dest) == 0 &&
      strcmp(fs1->protocol, fs2->protocol) == 0 &&
      fs1->srcport == fs2->srcport &&
      fs1->dstport == fs2->dstport) {
    retval = 0;
  }
  
  return retval;
}

/**
 * Find a structlink_map that matches the given object name and flow
 * specification.
 *
 * @param objname The object name to search for.
 * @param fs The flow specification to match.
 * @return A structlink_map object that matches the given parameters or NULL.
 */
static
structlink_map_t find_map(char *objname, struct flowspec *fs)
{
  structlink_map_t retval = NULL;
  int i;
  
  assert(objname != NULL);
  assert(strlen(objname) > 0);
  assert(fs != NULL);
  
  for(i = 0; i < link_index && !retval; i++){
    if(!strcmp(link_map[i].linkvnodes[0], objname) ||
       !strcmp(link_map[i].linkvnodes[1], objname)) {
      if (flowspeccmp(&link_map[i].fs, fs) == 0) {
	retval = &link_map[i];
      }
    }
  }

  return retval;
}

/*
 * Enable pipes for a node-to-node path in a cloud.
 * For every path there is an outgoing-from-the-delay-node (incoming to
 * the node) pipe handling delay and PLR.  However, not every path will
 * have a unique bandwidth pipe.  We allow for a node to have a shared,
 * incoming-to-the-delay-node (outgoing from the node) BW pipe used for
 * all destinations that do not have a unique pipe.
 */
static void
activate_pipe(int mapix, char *args)
{
  structlink_map_t link = &link_map[mapix];
  char *redir = ">/dev/null";

  if (debug)
    redir = "";

  /*
   * If not done already, create the delay pipe
   */
  if (link->inactive) {
    if (link->clouddir == 3) {  
      if (debug)
	info("activating delay/BW pipe %d\n", link->pipes[0]);
      else
	info("  create delay/BW pipe %d\n", link->pipes[0]);

      systemf("ipfw add %d pipe %d ip from any to %s in recv %s %s",
	      link->pipes[0],
	      link->pipes[0],
	      link->fs.dest,
	      link->interfaces[0],
	      redir);
      systemf("ipfw pipe %d config bw %d delay %d plr 0 queue %d %s",
	      link->pipes[0],
	      link->params[0].bw.bandwidth,
	      link->params[0].delay.delay,
	      link->params[0].q_size,
	      redir);
      link->inactive = 0;
    }
    else if (link->clouddir == 2) {
      if (debug)
	info("activating delay pipe %d\n", link->pipes[0]);
      else
	info("  create delay pipe %d\n", link->pipes[0]);

      systemf("ipfw add %d pipe %d ip from %s to any in recv %s %s",
	      link->pipes[0],
	      link->pipes[0],
	      link->fs.dest,
	      link->interfaces[0],
	      redir);
      systemf("ipfw pipe %d config bw 0 delay %d plr %f queue %d %s",
	      link->pipes[0],
	      link->params[0].delay.delay,
	      (double)link->params[0].loss.plr/0x7fffffff,
	      link->params[0].q_size,
	      redir);
      link->inactive = 0;
    }
    /*
     * XXX determine if there is a flow spec involved, see if there is
     * an explicit bandwidth provided.  If so, we may need to create a
     * flow-specific BW shaping pipe.
     */
    else if (link->fs.dest[0] && strstr(args, "BANDWIDTH")) {
      if (debug)
	info("activating BW pipe %d\n", link->pipes[0]);
      else
	info("  create BW pipe %d\n", link->pipes[0]);

      systemf("ipfw add %d pipe %d ip from any to %s in recv %s %s",
	      link->pipes[0],
	      link->pipes[0],
	      link->fs.dest,
	      link->interfaces[0],
	      redir);
      systemf("ipfw pipe %d config bw %d delay 0 plr 0 queue %d %s",
	      link->pipes[0],
	      link->params[0].bw.bandwidth,
	      link->params[0].q_size,
	      redir);
      link->inactive = 0;
    }
  }
}

/*************************** agent_callback **********************
 This function is called from the event system when an event
 notification is recd. from the server. It checks whether the 
 notification is valid (sanity check). If not print a warning,else
 call handle_pipes which does the rest of thejob
 *************************** agent_callback **********************/

#define LO_RULE_NO 100
#define HI_RULE_NO 59999

void agent_callback(event_handle_t handle,
		    event_notification_t notification, void *data)
{
  #define MAX_LEN 50

  char objname[MAX_LEN];
  char eventtype[MAX_LEN];
  char args[BUFSIZ];
  struct flowspec fs = { "", "", 0, 0 };
  char *dest, *srcport_str, *dstport_str, *protocol;
  int i, dest_len, srcport_len, dstport_len, protocol_len;
  static int lo_rule_no = LO_RULE_NO;
  static int hi_rule_no = HI_RULE_NO;
  char *redir = ">/dev/null";

  if (debug)
    redir = "";

    /* get the name of the object, eg. link0 or link1*/
  if(event_notification_get_string(handle,
                                   notification,
                                   "OBJNAME", objname, MAX_LEN) == 0 ||
     strlen(objname) == 0){
    error("could not get the objname \n");
    return;
  }

  /* get the eventtype, eg up/down/modify*/
  if(event_notification_get_string(handle,
                                   notification,
                                   "EVENTTYPE", eventtype, MAX_LEN) == 0){
    error("could not get the eventtype \n");
    return;
  }
  
  event_notification_get_arguments(handle,
				   notification, args, sizeof(args));

  /*
   * Get the flowspec parameters.  If there are none, the default
   * initialization (all zeros) will be used.
   */
  dest_len = event_arg_get(args, "DEST", &dest);
  protocol_len = event_arg_get(args, "PROTOCOL", &protocol);
  srcport_len = event_arg_get(args, "SRCPORT", &srcport_str);
  dstport_len = event_arg_get(args, "DSTPORT", &dstport_str);

  if (dest_len > (int)sizeof(fs.dest)) {
    error("DEST is too large: (%d>%d) %s\n", dest_len, sizeof(fs.dest), dest);
    return;
  }
  else if (dest_len > 0) {
    strncpy(fs.dest, dest, dest_len);
    fs.dest[dest_len] = '\0';
  }
  if (protocol_len > (int)sizeof(fs.protocol)) {
    error("PROTOCOL is too large\n");
    return;
  }
  else if (protocol_len > 0) {
    strncpy(fs.protocol, protocol, protocol_len);
    fs.protocol[protocol_len] = '\0';
  }
  if (srcport_len > 0 && sscanf(srcport_str, "%d", &fs.srcport) != 1) {
    error("SRCPORT is not a number\n");
    return;
  }
  if (dstport_len > 0 && sscanf(dstport_str, "%d", &fs.dstport) != 1) {
    error("DSTPORT is not a number\n");
    return;
  }
  
  if (strcmp(eventtype, TBDB_EVENTTYPE_CLEAR) == 0) {
    structlink_map_t lm;

    /* Handle a CLEAR event. */
    if (!debug) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      info("%d.%06d: %s: CLEAR: %s\n",
	   tv.tv_sec, tv.tv_usec, objname, args);
    }
    
    if (dest_len > 0 && srcport_len > 0 && dstport_len > 0 &&
	protocol_len > 0) {
      if ((lm = find_map(objname, &fs)) == NULL) {
	error("unknown flow for agent %s\n", objname);
      }
      else {
	if (debug)
	  info("clearing pipe %d\n", lm->pipes[0]);
	else
	  info("  clear pipe: %d\n", lm->pipes[0]);
	/* Delete the rule/pipe and */
	systemf("ipfw delete %d %s", lm->pipes[0], redir);
	systemf("ipfw pipe delete %d %s", lm->pipes[0], redir);
	/* ... mark the structure as free for another use. */
	strcpy(lm->linkvnodes[0], "__free");
	strcpy(lm->linkvnodes[1], "__free");
	lm->fs = blankfs;
      }
    }
    else {
      /*
       * We cannot just flush the world, because the delay node
       * might be shared.  So, we find all pipes associated with
       * the indicated object that have non-null flow info.
       */
      if (debug)
	info("clearing all flow pipes for %s\n", objname);
      for (lm = &link_map[0]; lm < &link_map[link_index]; lm++) {
	if (strcmp(lm->linkname, objname) != 0 ||
	    strcmp(lm->linkvnodes[0], "__free") == 0 ||
	    lm->fs.dest[0] == '\0')
	  continue;

        if (!lm->inactive) {
	  if (debug)
	    info("clearing pipe %d\n", lm->pipes[0]);
	  else
	    info("  clear pipe: %d\n", lm->pipes[0]);

	  /* Delete the rule/pipe and */
	  systemf("ipfw delete %d %s", lm->pipes[0], redir);
	  systemf("ipfw pipe delete %d %s", lm->pipes[0], redir);
	}
	/* ... mark the structure as free for another use. */
	strcpy(lm->linkvnodes[0], "__free");
	strcpy(lm->linkvnodes[1], "__free");
	lm->fs = blankfs;
      }
      if (link_map != old_map) {
	free(link_map);
	link_map = old_map;
	link_index = old_length;
	lo_rule_no = LO_RULE_NO;
	hi_rule_no = HI_RULE_NO;
      }
    }

    if (debug)
      dump_link_map();
    return;
  }

  if (strcmp(eventtype, TBDB_EVENTTYPE_CREATE) == 0) {
    if (!debug) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      info("%d.%06d: %s: CREATE: %s\n",
	   tv.tv_sec, tv.tv_usec, objname, args);
    }

    if (dest_len == -1) {
      if (debug)
	info("creating per-host pipes for %s\n", objname);
      
      if (link_map == old_map) {
	link_map = NULL;
	link_index = 0;
      }

      /*
       * For every link we were managing, create two pipes for that link
       * and a particular destination.  The set of destinations are currently
       * pulled from the /etc/hosts file.
       */
      for (i = 0; i < old_length; i++) {
	struct hostent *he;
	
	/* preserve original rules */
	realloc_map();
	link_map[link_index] = old_map[i];
	link_index++;

	/* if not related to our link, we are done */
	if (strcmp(old_map[i].linkname, objname) != 0)
	  continue;

	while ((he = gethostent()) != NULL) {
	  char dest[32];
	  int j;

	  inet_ntop(he->h_addrtype, he->h_addr, dest, sizeof(dest));

	  /* addresses we don't care about */
	  if (strcmp(dest, "127.0.0.1") == 0 || strcmp(dest, "0.0.0.0") == 0)
	    continue;
		  
	  /* XXX can only handle duplex links/lans (need two pipes) */
	  if (old_map[i].numpipes < 2)
	    continue;    	  

	  /*
	   * Pipe 0 is for delay and will be created for all flows
	   * Pipe 1 is for BW and will only created if a MODIFY (with DEST=)
	   * event explicitly specifies a BW.
	   */
	  for (j = 0; j < old_map[i].numpipes; j++) {
	    realloc_map();
	    link_map[link_index] = old_map[i];
	    link_map[link_index].islan = 0;
	    if (old_map[i].islan) {
	      link_map[link_index].clouddir = j ? 2 : 1;
	    } else {
	      /*
	       * We don't have enough pipes to do the hybrid technique
	       * for a link.  We just arrange to set delay and BW on the
	       * same link.
	       */
	      link_map[link_index].clouddir = 3;
	    }
	    link_map[link_index].numpipes = 1;
	    strncpy(link_map[link_index].fs.dest, dest,
		    sizeof(link_map[link_index].fs.dest));
	    link_map[link_index].inactive = 1;
	    link_map[link_index].pipes[0] = hi_rule_no;
	    link_map[link_index].interfaces[0] = old_map[i].interfaces[j];
	    link_map[link_index].params[0] = old_map[i].params[j];
	    hi_rule_no -= 1;
	    link_index += 1;
	  }
	}
	endhostent();
      }
    }
    else if (srcport_len <= 0 && dstport_len <= 0 && protocol_len <= 0) {
      error("CREATE event missing SRCPORT, DSTPORT, and PROTOCOL args\n");
    }
    else {
      struct flowspec mainfs = { "", "", 0, 0 };
      structlink_map_t mainlm, lm;

      strcpy(mainfs.dest, fs.dest);
      mainlm = find_map(objname, &mainfs);
      if (mainlm == NULL) {
	error("No such agent: %s\n", objname);
      }
      else if ((lm = find_map(objname, &fs)) != NULL) {
	info("%s flow already exists\n", objname);
      }
      else {
	int rule_no;

	if ((lm = find_map("__free", &blankfs)) == NULL) {
	  /* No free structlink_map objects, allocate a new one. */
	  realloc_map();
	  
	  /* XXX need to relocate the basis pipe due to realloc */
	  if ((mainlm = find_map(objname, &mainfs)) == NULL) {
	    error("No such agent: %s\n", objname);
	  }

	  lm = &link_map[link_index];
	  link_index += 1;
	  rule_no = lo_rule_no;
	  lo_rule_no += 1;
	}
	else {
	  /* Reuse an existing structlink_map object. */
	  rule_no = lm->pipes[0];
	}
	  
	if (debug)
	  info("creating per-flow pipe\n");
	else
	  info("  create flow pipe %d\n", rule_no);

	*lm = *mainlm;
	lm->clouddir = 0;
	lm->inactive = 0;
	lm->islan = 0;
	lm->numpipes = 1;
	lm->fs = fs;
	lm->pipes[0] = rule_no;
	systemf("ipfw add %d pipe %d %s from any to %s "
		"src-port %d dst-port %d in recv %s %s",
		lm->pipes[0],
		lm->pipes[0],
		lm->fs.protocol,
		lm->fs.dest,
		lm->fs.srcport,
		lm->fs.dstport,
		lm->interfaces[0],
		redir);

	/*
	 * Initialize its characteristics from the "basis" pipe.
	 * Note that if the basis is a funky BW only pipe, we need to
	 * extract the delay/PLR from the following pipe.
	 */
	lm->params[0] = mainlm->params[0];
	assert(mainlm->clouddir != 2);
	if (mainlm->clouddir == 1) {
		assert((mainlm+1) < &link_map[link_index]);
		lm->params[0].delay = (mainlm+1)->params[0].delay;
		lm->params[0].loss = (mainlm+1)->params[0].loss;
	}
	systemf("ipfw pipe %d config bw %d delay %d plr 0 queue %d %s",
		lm->pipes[0],
		lm->params[0].bw.bandwidth,
		lm->params[0].delay.delay,
		lm->params[0].q_size,
		redir);
      }
    }

    if (debug)
      dump_link_map();
    return;
  }

  /*
   * We could be an agent for several nodes on the same lan, so need to
   * loop over the pipe sets and possibly repeat all the work multiple
   * times. Sigh.
   */
  for(i = 0; i < link_index; i++){
    if(!strcmp(link_map[i].linkname, objname) ||
       !strcmp(link_map[i].linkvnodes[0], objname) ||
       !strcmp(link_map[i].linkvnodes[1], objname)) {
      if (flowspeccmp(&link_map[i].fs, &fs) == 0) {
	handle_pipes(objname, eventtype, args, i);
      }
    }
  }
}

/******************** handle_pipes ***************************************
This dispatch function checks the event type and dispatches to the appropriate
routine to handle
 ******************** handle_pipes ***************************************/

void handle_pipes (char *objname, char *eventtype, char *args, int l_index)
{
  /*link_map[index] contains the relevant info*/

  if (!debug) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    info("%d.%06d: %s(%d): %s: %s\n",
	 tv.tv_sec, tv.tv_usec, objname, l_index, eventtype, args);
  }

  if(strcmp(eventtype, TBDB_EVENTTYPE_UP) == 0){
    handle_link_up(objname, l_index);
  }
  else if(strcmp(eventtype, TBDB_EVENTTYPE_DOWN) == 0){
    handle_link_down(objname, l_index);
  }
  else if(strcmp(eventtype, TBDB_EVENTTYPE_MODIFY) == 0){
    handle_link_modify(objname, l_index, args);
  }
  else error("unknown link event type\n");

  if(debug){
    system ("echo ======================================== >> /tmp/ipfw.log"); 
    system("(date;echo PARAMS ; ipfw pipe show all) >> /tmp/ipfw.log");
  }
}

/******************* handle_link_up **************************
This handles the link_up event. If link is already up, it returns
without doing anything. If link is down, it calls set_link_params
for looking up the link params in the link_table and configuring 
them into dummynet
 ******************* handle_link_up ***************************/

void handle_link_up(char * linkname, int l_index)
{
  /* get the pipe params from the params field of the
     link_map table. Set the pipe params in dummynet
   */
  if (debug) {
    info("==========================================\n");
    info("recd. UP event for link = %s\n", linkname);
  }

  /* no need to do anything if link is already up*/
  if(link_map[l_index].stat == LINK_UP)
    return;
   
  link_map[l_index].stat = LINK_UP;
  set_link_params(l_index, 0, -1);
}

/******************* handle_link_down **************************
This handles the link_down event. If link is already down, it returns
without doing anything. If link is up, it calls get_link_params
to populate the link_map table with current params. It then calls
set_link_params to write back the pipe params, but with plr = 1.0,
so that the link goes down
 ******************* handle_link_down***************************/

void handle_link_down(char * linkname, int l_index)
{
  /* get the pipe params from dummynet
   * Store them in the params field of the link_map table.
   * Change the pipe config so that plr = 1.0
   * so that packets are blackholed
   */
  if (debug) {
    info("==========================================\n");
    info("recd. DOWN event for link = %s\n", linkname);
  }

  if(link_map[l_index].stat == LINK_DOWN)
    return;

  if(get_link_params(l_index) == 1){
    link_map[l_index].stat = LINK_DOWN;
    set_link_params(l_index, 1, -1);
  }
  else error("could not get params\n");
}

/*********** handle_link_modify *****************************
It gets the new link params from the notification and updates
the link_map table with these params. If the link is down,
it just returns. IF the link is up, then it calls set_link_params
to set the new params
 *********** handle_link_modify *****************************/

void handle_link_modify(char * linkname, int l_index, char * args)
{
  /* Get the new pipe params from the notification, and then
     update the new set of params by setting the params in
     dummynet
   */
  char myargs[BUFSIZ];
  int i, p_which = -1;

  if (debug) {
    info("==========================================\n");
    info("recd. MODIFY event for link = %s (%d)\n", linkname, l_index);
  }

  strncpy(myargs, args, BUFSIZ);

  /*
   * As a convience to the user, we create virt_agents entries
   * for each "link-vnode" so that users can talk to a specific
   * side of a duplex link, or a specific node in a lan (in which
   * case it refers to both pipes, not just one). Look at the
   * object name to determine ahead of time which pipe. Note that
   * for the lan node case, the virt agent entry exists strictly
   * to direct the event to this agent; there might be an actual
   * pipe spec inside the event if the user wants to one side
   * of the link (to the switch or from the switch).
   */
  if (!link_map[l_index].islan) {
    for (i = 0; i < link_map[l_index].numpipes; i++) {
      if(!strcmp(link_map[l_index].linkvnodes[i], linkname)) {
	p_which = i;
      }
    }
  }
  
  /* Create the pipe(s) if it hasn't been done already */
  if (link_map[l_index].clouddir != 0)
    activate_pipe(l_index, myargs);

  /* if the link is up, then get the params from dummynet,
     get the params from the notification and then merge
     and write back to dummynet
   */
  if(link_map[l_index].stat == LINK_UP){
    if(get_link_params(l_index) == 1)
      if(get_new_link_params(l_index, myargs, &p_which) == 1)
	set_link_params(l_index, 0, p_which);
  } else
    /* link is down, so just change in the link_map*/
    get_new_link_params(l_index, myargs, &p_which);
}

/**
 * Copy a distribution table returned from dummynet into malloc'd memory.
 *
 * @param entries The number of entries in the table.
 * @param table_inout Pointer to the table memory as returned by
 * IP_DUMMYNET_GET.  On return, the pointer will be incremented to point to the
 * memory following the table.
 * @return The malloc'd table or NULL if the table size was zero.
 */
int *copy_table(int entries, int **table_inout)
{
  int *retval = NULL;

  if (entries > 0) {
    size_t len = entries * sizeof(int);
    
    if ((retval = malloc(len)) != NULL) {
      bcopy(*table_inout, retval, len);
    }
    else {
      error("copy_table: can't allocate memory for table\n");
    }

    *table_inout += entries;
  }
  
  return retval;
}

/* link field changed in 6.1 */
#if __FreeBSD_version >= 601000
#define DN_PIPE_NEXT(p)	((p)->next.sle_next)
#else
#define DN_PIPE_NEXT(p)	((p)->next)
#endif

/****************** get_link_params ***************************
for both the pipes of the duplex link, get the pipe params
sing getsockopt and store these params in the link map
table
****************** get_link_params ***************************/

int get_link_params(int l_index)
{
  struct dn_pipe *pipes;
  int fix_size;
  int num_bytes, num_alloc;
  void * data = NULL;
  int p_index = 0;
  int pipe_num;
  while( p_index < link_map[l_index].numpipes ){
    
    pipe_num = link_map[l_index].pipes[p_index];

    /* get the pipe structure from Dummynet, resizing the data
       as required
     */
    fix_size = sizeof(*pipes);
    num_alloc = fix_size;
    num_bytes = num_alloc;

    data = malloc(num_bytes);
    if(data == NULL){
      error("malloc: cant allocate memory\n");
      return 0;
    }
    
    while (num_bytes >= num_alloc) {
      num_alloc = num_alloc * 2 + 200;
      num_bytes = num_alloc ;
      if ((data = realloc(data, num_bytes)) == NULL){
	error("cant alloc memory\n");
	return 0;
      }
    
      if (getsockopt(s_dummy, IPPROTO_IP, IP_DUMMYNET_GET,
		     data, &num_bytes) < 0){
	error("error in getsockopt\n");
	return 0;
      }

    }

    /* now search the pipe list for the required pipe*/
    {

    void *next = data ;
    struct dn_pipe *p = (struct dn_pipe *) data;
    struct dn_flow_queue *q ;
    int l, *tables ;

    for ( ; num_bytes >= sizeof(*p) ; p = (struct dn_pipe *)next ) {
       
     
       if ( DN_PIPE_NEXT(p) != (struct dn_pipe *)DN_IS_PIPE )
	  break ;

       l = sizeof(*p) + p->fs.rq_elements * sizeof(*q) +
	 p->delay.entries * sizeof(int) +
	 p->delay.qentries * sizeof(int) +
	 p->bandwidth.entries * sizeof(int) +
	 p->bandwidth.qentries * sizeof(int) +
	 p->loss.entries * sizeof(int) +
	 p->loss.qentries * sizeof(int) ;
       next = (void *)p  + l ;
       num_bytes -= l ;
       q = (struct dn_flow_queue *)(p+1) ;
       tables = (int *)(q + p->fs.rq_elements);

       if (pipe_num != p->pipe_nr)
	   continue;

       /* Free the old tables. */
       free(link_map[l_index].params[p_index].delay.table);
       free(link_map[l_index].params[p_index].delay.quantum);

       free(link_map[l_index].params[p_index].bw.table);
       free(link_map[l_index].params[p_index].bw.quantum);
       
       free(link_map[l_index].params[p_index].loss.table);
       free(link_map[l_index].params[p_index].loss.quantum);

       /* grab pipe delay and bandwidth */
       link_map[l_index].params[p_index].delay = p->delay;
       link_map[l_index].params[p_index].delay.quantum =
	 copy_table(p->delay.qentries, &tables);
       link_map[l_index].params[p_index].delay.table =
	 copy_table(p->delay.entries, &tables);
       
       link_map[l_index].params[p_index].bw = p->bandwidth;
       link_map[l_index].params[p_index].bw.quantum =
	 copy_table(p->bandwidth.qentries, &tables);
       link_map[l_index].params[p_index].bw.table =
	 copy_table(p->bandwidth.entries, &tables);
       
       link_map[l_index].params[p_index].loss = p->loss;
       link_map[l_index].params[p_index].loss.quantum =
	 copy_table(p->loss.qentries, &tables);
       link_map[l_index].params[p_index].loss.table =
	 copy_table(p->loss.entries, &tables);

       /* get flow set parameters*/
       get_flowset_params( &(p->fs), l_index, p_index);

       /* get dynamic queue parameters*/
       get_queue_params( &(p->fs), l_index, p_index);
     }
  }

    free(data);
    /* go for the next pipe in the duplex link*/
    p_index++; 
  }

  return 1;
}

/************ get_flowset_params ********************************
  get the flowset params from the dummnet pipe data structure
************** get_flowset_params ******************************/

void get_flowset_params(struct dn_flow_set *fs, int l_index,
			int p_index)
{
  
  /* grab pointer to the params structure of the pipe in the
     link_map table
   */
    structpipe_params *p_params
      = &(link_map[l_index].params[p_index]);


  /* q size*/
    p_params->q_size = fs->qsize;

    /* initialise flags */
    p_params->flags_p = 0;

    /* q unit */
    if (fs->flags_fs & DN_QSIZE_IS_BYTES)
      p_params->flags_p |= PIPE_QSIZE_IN_BYTES;

    /* q plr*/
   #if 0 
    p_params->plr = fs->plr;
   #endif

#if 0
    p_params->loss.plr = 1.0*fs->plr/(double)(0x7fffffff);
#endif
    
    /* hash table/bucket size */
    p_params->buckets = fs->rq_size;

#if 0
    /* number of queues in the pipe */
    p_params->n_qs = fs->rq_elements;
#endif
    
    /* q type and corresponding parameters*/

    /* internally dummynet sets DN_IS_RED (only) if the queue is RED and
       sets DN_IS_RED and DN_IS_GENTLE_RED(both) if queue is GRED*/

    if (fs->flags_fs & DN_IS_RED) {
      /* get GRED/RED params */

      p_params->red_gred_params.w_q =  1.0*fs->w_q/(double)(1 << SCALE_RED);
      p_params->red_gred_params.max_p = 1.0*fs->max_p/(double)(1 << SCALE_RED);
      p_params->red_gred_params.min_th = SCALE_VAL(fs->min_th);
      p_params->red_gred_params.max_th = SCALE_VAL(fs->max_th);

      if(fs->flags_fs & DN_IS_GENTLE_RED) 
	p_params->flags_p |= PIPE_Q_IS_GRED;
      else
	p_params->flags_p |= PIPE_Q_IS_RED;
      
    }
    /* else droptail*/

  }

/********* get_queue_params ******************************
 get the queue specific params like the pipe flow mask 
 which is used to create dynamic queues as new flows are
 formed
 ********* get_queue_params ******************************/

void get_queue_params(struct dn_flow_set *fs,
		      int l_index, int p_index)
{
  /* grab the queue mask. Actually a pipe can have multiple masks,
     HANDLE THIS LATER
   */
  
  structpipe_params *p_params
    = &(link_map[l_index].params[p_index]);
  
  
  if(fs->flags_fs & DN_HAVE_FLOW_MASK){
    p_params->flags_p    |= PIPE_HAS_FLOW_MASK;
    p_params->id.proto     = fs->flow_mask.proto;
    p_params->id.src_ip    = fs->flow_mask.src_ip;
    p_params->id.src_port  = fs->flow_mask.src_port;
    p_params->id.dst_ip    = fs->flow_mask.dst_ip;
    p_params->id.dst_port  = fs->flow_mask.dst_port;
  }

  return;
}

/****************** set_link_params ***************************
for both the pipes of the duplex link, set the pipe params
sing setsockopt getting the param values from the link_map table
**************************************************************/

void set_link_params(int l_index, int blackhole, int p_which)
{
  /* Grab the pipe params from the link_map table and then
     set them into dummynet by calling setsockopt
   */
  int p_index;

  if (debug) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    info("setting at %ld.%d\n", tv.tv_sec, tv.tv_usec);
  }

  for (p_index = 0; p_index < link_map[l_index].numpipes; p_index++) {
      /*
       * Want to do all the pipes, or just the one pipe that was
       * specified.
       */
       if(p_which == -1 || p_which == p_index) {
	    struct dn_pipe pipe;
	    /* get the params stored in the link table*/
	    structpipe_params *p_params
	      = &(link_map[l_index].params[p_index]);

	    if (debug)
	      info("entered the loop, pindex = %d %s %s (%s)\n", p_index,
		   link_map[l_index].linkvnodes[p_index],
		   link_map[l_index].fs.dest,
		   link_map[l_index].inactive ? "inactive" : "active");

	    if (link_map[l_index].inactive)
	      continue;

	    memset(&pipe, 0, sizeof pipe);

	    switch (link_map[l_index].clouddir) {
	    case 1:
	      p_params->delay.delay = 0;
	      p_params->loss.dist = DN_DIST_CONST_RATE;
	      p_params->loss.plr = 0;
	      break;
	    case 2:
	      p_params->bw.bandwidth = 0;
	      break;
	    default:
	      break;
	    }

	    /* set the bandwidth and delay*/
	    pipe.bandwidth = p_params->bw;
	    pipe.delay = p_params->delay;
	    pipe.loss = p_params->loss;

	    /* set the pipe number*/
	    pipe.pipe_nr = link_map[l_index].pipes[p_index];

	    /* if we want to blackhole (in the case of EVENT_DOWN,
	       then we set all other pipe params same, but change the
	       plr to 1.0
	       */
	    {
	      if (blackhole) {
		pipe.loss.plr = 0x7fffffff;
		pipe.loss.dist = DN_DIST_CONST_RATE;
	      }

	      pipe.fs.plr = pipe.loss.plr;
	    }

	    /* set the queue size*/
	    pipe.fs.qsize = p_params->q_size;
	    /* set the number of buckets used for dynamic queues*/
	    pipe.fs.rq_size = p_params->buckets;
#if 0
	    pipe.fs.rq_elements = num_qs;
#endif
	    /* initialise pipe flags to zero */
	    pipe.fs.flags_fs = 0;

	    /* set if the q size is in slots or in bytes*/
	    if(p_params->flags_p & PIPE_QSIZE_IN_BYTES)
	      pipe.fs.flags_fs |= DN_QSIZE_IS_BYTES;
   
#if 0
	    pipe.fs.flow_mask.proto = 0;
	    pipe.fs.flow_mask.src_ip = 0;
	    pipe.fs.flow_mask.dst_ip = 0;
	    pipe.fs.flow_mask.src_port = 0;
	    pipe.fs.flow_mask.dst_port = 0;

#endif

	    /* set if the pipe has a flow mask*/
	    if(p_params->flags_p & PIPE_HAS_FLOW_MASK){
	      pipe.fs.flags_fs |= DN_HAVE_FLOW_MASK;

	      pipe.fs.flow_mask.proto = p_params->id.proto;
	      pipe.fs.flow_mask.src_ip = p_params->id.src_ip;
	      pipe.fs.flow_mask.dst_ip = p_params->id.dst_ip;
	      pipe.fs.flow_mask.src_port = p_params->id.src_port;
	      pipe.fs.flow_mask.dst_port = p_params->id.dst_port;
	    }
	    /* set the queing discipline and other relevant params*/

	    if(p_params->flags_p & (PIPE_Q_IS_GRED | PIPE_Q_IS_RED)){
	      /* set GRED params */
	      pipe.fs.flags_fs |= DN_IS_RED;
	      pipe.fs.max_th = p_params->red_gred_params.max_th;
	      pipe.fs.min_th = p_params->red_gred_params.min_th;
	      pipe.fs.w_q = (int) ( p_params->red_gred_params.w_q * (1 << SCALE_RED) ) ;
	      pipe.fs.max_p = (int) ( p_params->red_gred_params.max_p * (1 << SCALE_RED) );

	      if(p_params->flags_p & PIPE_Q_IS_GRED)
		pipe.fs.flags_fs |= DN_IS_GENTLE_RED;

	      if(pipe.bandwidth.bandwidth){
		size_t len ; 
		int lookup_depth, avg_pkt_size ;
		double s, idle, weight, w_q ;
		struct clockinfo clock ;
		int t ;

		len = sizeof(int) ;
		if (sysctlbyname("net.inet.ip.dummynet.red_lookup_depth", 
				 &lookup_depth, &len, NULL, 0) == -1){
		  error("cant get net.inet.ip.dummynet.red_lookup_depth");
		  return;
		}
		if (lookup_depth == 0) {
		  info("net.inet.ip.dummynet.red_lookup_depth must" 
			    "greater than zero") ;
		  return;
		}
	   
		len = sizeof(int) ;
		if (sysctlbyname("net.inet.ip.dummynet.red_avg_pkt_size", 
			&avg_pkt_size, &len, NULL, 0) == -1){

		  error("cant get net.inet.ip.dummynet.red_avg_pkt_size");
		  return;
		}
		if (avg_pkt_size == 0){
		  info("net.inet.ip.dummynet.red_avg_pkt_size must" 
				"greater than zero") ;
		  return;
		}

		len = sizeof(struct clockinfo) ;

		if (sysctlbyname("kern.clockrate", 
				 &clock, &len, NULL, 0) == -1) {
		  error("cant get kern.clockrate") ;
		  return;
		}
	   

		/* ticks needed for sending a medium-sized packet */
		s = clock.hz * avg_pkt_size * 8 / pipe.bandwidth.bandwidth;

		/*
		 * max idle time (in ticks) before avg queue size becomes 0. 
		 * NOTA:  (3/w_q) is approx the value x so that 
		 * (1-w_q)^x < 10^-3. 
		 */
		w_q = ((double) pipe.fs.w_q) / (1 << SCALE_RED) ; 
		idle = s * 3. / w_q ;
		pipe.fs.lookup_step = (int) idle / lookup_depth ;
		if (!pipe.fs.lookup_step) 
		  pipe.fs.lookup_step = 1 ;
		weight = 1 - w_q ;
		for ( t = pipe.fs.lookup_step ; t > 0 ; --t ) 
		  weight *= weight ;
		pipe.fs.lookup_weight = (int) (weight * (1 << SCALE_RED)) ;
	     
	      }
	 
	    }
	    /*  else DROPTAIL*/

	    /* now call setsockopt*/
	    if (setsockopt(s_dummy,IPPROTO_IP, IP_DUMMYNET_CONFIGURE, &pipe,sizeof pipe)
		< 0)
	      error("IP_DUMMYNET_CONFIGURE setsockopt failed\n");
	  }
    }
  
}

/*
 * NOTE: The ln_gamma and build_poisson functions were taken out of the
 * patched ipfw2.c
 */

static double
ln_gamma(double xx)
{
	/* log gamma function adapted from numerical recipes in C */

	if (xx<1.0)                           /* Use reflection formula */
	{
		double piz = 3.14159265359 * (1.0-xx);
		return log(piz/sin(piz))-ln_gamma(2.0-xx);
	}
	else
	{
		static double cof[6]={76.18009173,-86.50532033,24.01409822,
				      -1.231739516,0.120858003e-2,-0.536382e-5};
		double x,tmp,ser;
		int j;

		x=xx-1.0; tmp=x+5.5;
		tmp -= (x+0.5)*log(tmp);  ser=1.0;
		for (j=0;j<=5;j++) { x += 1.0; ser += cof[j]/x; }
		return -tmp+log(2.50662827465*ser);
	}
}

/*
 * build_poisson()
 * given a mean, build a table for poisson distribution.
 *
 * remember calculus long, long ago, estimating the area under the function by 
 * calculating f(1) + f(2) + f(3) + ... ? Well, that is sort of what we are
 * doing here. if f(1)=1, we put one 1 in the table. if f(7)=3, we put three 
 * 7's in the table.  When the table is complete, one can randomly choose
 * values from this table, with the results following a poisson distribution.
 */
static int *
build_poisson(int mean, int *entries)
{
	int p, x, i;
	int *table;
	double probability;

	if (mean <= 0)
		errx(EX_USAGE,"invalid mean %d\n",mean);

	x = *entries = 0;

	if ( ! ( table = malloc(sizeof(int)*0x8200)))
		err(1,"poisson table ate all of the memory\n");

	do {
		/* P(x) =  e^-mean * mean^x
		 *         ----------------
		 *              x!
		 *
		 * we will have approximately 0x8000 entries in our table.
		 * there is no deep meaning to 0x8000. arbitrary.
		 *
		 */
		probability = log(mean) * x - mean - ln_gamma(1.0 + x);
		probability = (probability < -40.0) ? 0.0 : exp(probability);
		probability *= 0x8000;
		for(p= (int)probability ;p;p--) {
			table[(*entries)++]=x;
			/* since we truncate when casting to int, I don't even think
			 * we need any more than 0x8000 entries. However, I do not
			 * feel confident enough to chance it.
			 */
			if (*entries == 0x8200)
				return table;
		}
		x++;
	} while(x < mean || probability >= 1);

	if (0) {
		info("%d  - ", mean);
		for (i = 0; i < *entries; i++) {
			info(" %d", table[i]);
		}
		info("\n");
	}
	
	return table;
}

/**
 * Convert a distribution type given by name to an integer constant.
 *
 * @param name The distribution name.
 * @param defval The default value to return if 'name' doesn't match a known
 * type.
 * @return A DN_DIST_ value.
 */
int dist_name_to_enum(char *name, int defval)
{
  int retval = defval;

  if (strcasecmp(name, "uniform") == 0)
    retval = DN_DIST_UNIFORM;
  else if (strcasecmp(name, "poisson") == 0)
    retval = DN_DIST_POISSON;
  else if (strcasecmp(name, "random") == 0)
    retval = DN_DIST_TABLE_RANDOM;
  else if (strcasecmp(name, "determ") == 0)
    retval = DN_DIST_TABLE_DETERM;

  return retval;
}

/**
 * @param table The string form of a distribution table, where each entry is
 * separated by a forward slash ('/').
 * @return The number of entries in the table.
 */
int table_size(char *table)
{
  int lpc, retval = 1;

  assert(table != NULL);
  
  for (lpc = 0; table[lpc] != '\0'; lpc++) {
    if (table[lpc] == '/')
      retval += 1;
  }

  return retval;
}

/**
 * Parse the string form of a distribution table into an array of integers.
 *
 * @param table The string form of a distribution table, where each entry is
 * separated by a forward slash ('/').
 * @param count_out The number of elements in the table or zero if there was
 * an error creating the table.
 * @return An integer array containing the values given in 'table' or NULL if
 * there was an error.
 */
int *parse_int_table(char *table, int *count_out)
{
  int err = 0, *retval = NULL;

  *count_out = table_size(table);
  if ((retval = malloc(sizeof(int) * *count_out)) != NULL) {
    int lpc = 0;
    
    do {
      if (*table == '/')
	table += 1;
      if (sscanf(table, "%d", &retval[lpc]) != 1)
	err = 1;
      lpc += 1;
    } while (!err && (table = strchr(table, '/')) != NULL);
  }
  else {
    error("parse_int_table: can't allocate memory for table.\n");
    *count_out = 0;
  }

  if (err) {
    free(retval);
    retval = NULL;
    *count_out = 0;
  }

  return retval;
}

int *expand_int_table(int value, int count)
{
  int *retval = NULL;

  if ((retval = malloc(sizeof(int) * count)) != NULL) {
    int lpc = 0;

    for (lpc = 0; lpc < count; lpc++) {
      retval[lpc] = value;
    }
  }

  return retval;
}

/**
 * Parse the string form of a distribution table containing loss values into an
 * array of integers.  Loss values are expected to be between 0.0 and 1.0,
 * which are then converted to an integer value between 0 and 7fffffff, since
 * that is what dummynet expects.
 *
 * @param table The string form of a distribution table, where each entry is
 * separated by a forward slash ('/').
 * @param count_out The number of elements in the table or zero if there was
 * an error creating the table.
 * @return An integer array containing the values given in 'table' or NULL if
 * there was an error.  
 */
int *parse_loss_table(char *table, int *count_out)
{
  int err = 0, *retval = NULL;

  *count_out = table_size(table);
  if ((retval = malloc(sizeof(int) * *count_out)) != NULL) {
    int lpc = 0;
    
    do {
      double dval;

      if (*table == '/')
	table += 1;
      if (sscanf(table, "%lf", &dval) == 1) {
	if (dval < 0.0)
	  dval = 0.0;
	else if (dval > 1.0)
	  dval = 1.0;
	retval[lpc] = (dval * 0x7fffffff);
      }
      else {
	info("invalid table entry: %s\n", table);
	err = 1;
      }
      lpc += 1;
    } while (!err && (table = strchr(table, '/')) != NULL);
  }
  else {
    error("parse_loss_table: can't allocate memory for table.\n");
  }

  if (err) {
    free(retval);
    retval = NULL;
    *count_out = 0;
  }
  
  return retval;
}

/**
 * Duplicate an integer array.
 *
 * @param table The array to duplicate.
 * @param count The number of entries in the table.
 * @return The newly allocated table or NULL if count was zero or an error
 * occurred.
 */
int *tabledup(int *table, int count)
{
  int *retval = NULL;

  assert(table != NULL);
  assert(count >= 0);
  
  if (count > 0) {
    if ((retval = malloc(sizeof(int) * count)) != NULL) {
      bcopy(table, retval, sizeof(int) * count);
    }
    else {
      error("tabledup: can't allocate memory for table.\n");
    }
  }
  
  return retval;
}

/********* get_new_link_params ***************************
  For a modify event, this function gets the parameters
  from the event notification
 ********************************************************/

int get_new_link_params(int l_index, char *argstring, int *pipe_which)
{
  /* get the params of the pipe that were sent in the notification and
     store those values into the link_map table
   */
  char * argtype = NULL;
  char * argvalue = NULL;
  int p_num = 0;
  int gotpipe = 0;
  int islan = link_map[l_index].islan;
  int dobpois = 0, dodpois = 0, dolpois = 0;
  char *temp = NULL;

  /* Allow upper level to init the pipe */
  if (*pipe_which >= 0) {
      gotpipe = 1;
      p_num   = *pipe_which;
  }
  
  if(argstring){
    unsigned long bq = -1, dq = -1, lq = -1;
    
    temp = argstring;
    
    argtype = strsep(&temp,"=");
    
    while((argvalue = strsep(&temp," \n"))){

        //pramod-CHANGES:
      /* Backfill parameters. */
      if(strcmp(argtype,"BACKFILL")== 0){
	if (debug)
	  info("Backfill = %d\n", atoi(argvalue) * 1000);
	link_map[l_index].params[p_num].bw.backfill = atoi(argvalue) * 1000;
    //pramod-What does this do??
	if (! gotpipe) {
	  link_map[l_index].params[1].bw.backfill =
		  link_map[l_index].params[0].bw.backfill;
	}
      }
      /* BANDWIDTH parameters. */
      else if(strcmp(argtype,"BANDWIDTH")== 0){
	if (debug)
	  info("Bandwidth = %d\n", atoi(argvalue) * 1000);
	link_map[l_index].params[p_num].bw.bandwidth = atoi(argvalue) * 1000;
	link_map[l_index].params[p_num].bw.dist = DN_DIST_CONST_RATE;
	if (! gotpipe) {
	  link_map[l_index].params[1].bw.dist = DN_DIST_CONST_RATE;
	  link_map[l_index].params[1].bw.bandwidth =
		  link_map[l_index].params[0].bw.bandwidth;
	}
      }
      else if (strcmp(argtype,"BWQUANTUM")== 0){
	 if (debug)
	   info("Bandwidthq = %d\n", atoi(argvalue));
	 bq = atoi(argvalue);
      }
      else if (strcmp(argtype,"BWQUANTABLE")== 0){
	 if (debug)
	   info("Bandwidthqt = %s\n", argvalue);
	 
	 free(link_map[l_index].params[p_num].bw.quantum);
	 link_map[l_index].params[p_num].bw.quantum =
	   parse_int_table(argvalue,
			   &link_map[l_index].params[p_num].bw.qentries);
	 if (! gotpipe) {
	   link_map[l_index].params[1].bw.qentries =
	     link_map[l_index].params[0].bw.qentries;
	   free(link_map[l_index].params[1].bw.quantum);
	   link_map[l_index].params[1].bw.quantum =
	     tabledup(link_map[l_index].params[0].bw.quantum,
		      link_map[l_index].params[0].bw.qentries);
	 }
      }
      else if (strcmp(argtype,"BWMEAN")== 0){
	 if (debug)
	   info("Bandwidth mean = %d\n", atoi(argvalue) * 1000);
	 link_map[l_index].params[p_num].bw.mean = atoi(argvalue) * 1000;
	 if (! gotpipe) {
	   link_map[l_index].params[1].bw.mean =
	     link_map[l_index].params[0].bw.mean;
	 }
      }
      else if (strcmp(argtype,"BWSTDDEV")== 0){
	 if (debug)
	   info("Bandwidth stddev = %d\n", atoi(argvalue));
	 link_map[l_index].params[p_num].bw.stddev = atoi(argvalue);
	 if (! gotpipe) {
	   link_map[l_index].params[1].bw.stddev =
	     link_map[l_index].params[0].bw.stddev;
	 }
      }
      else if (strcmp(argtype,"BWDIST")== 0){
	 if (debug)
	   info("bwdist = %s\n", argvalue);

	 link_map[l_index].params[p_num].bw.dist =
	   dist_name_to_enum(argvalue,
			     link_map[l_index].params[p_num].bw.dist);
	 if (link_map[l_index].params[p_num].bw.dist == DN_DIST_POISSON)
	     dobpois = 1;
	 if (! gotpipe) {
	   link_map[l_index].params[1].bw.dist =
	     link_map[l_index].params[0].bw.dist;
	 }
      }
      else if (strcmp(argtype,"BWTABLE")== 0){
	 int lpc;

	 if (debug)
	   info("bwtable = %s\n", argvalue);

	 free(link_map[l_index].params[p_num].bw.table);
	 link_map[l_index].params[p_num].bw.table =
	   parse_int_table(argvalue,
			   &link_map[l_index].params[p_num].bw.entries);
	 for (lpc = 0;
	      lpc < link_map[l_index].params[p_num].bw.entries;
	      lpc++) {
	     link_map[l_index].params[p_num].bw.table[lpc] *= 1000;
	 }
	 if (! gotpipe) {
	   link_map[l_index].params[1].bw.entries =
	     link_map[l_index].params[0].bw.entries;
	   free(link_map[l_index].params[1].bw.table);
	   link_map[l_index].params[1].bw.table =
	     tabledup(link_map[l_index].params[0].bw.table,
		      link_map[l_index].params[0].bw.entries);
	 }
      }
      
      /* DELAY parameters. */
      else if (strcmp(argtype,"DELAY")== 0){
	 if (debug)
	   info("Delay = %d\n", atoi(argvalue));
	 link_map[l_index].params[p_num].delay.delay = atoi(argvalue);
	 link_map[l_index].params[p_num].delay.dist = DN_DIST_CONST_TIME;
	 if (! gotpipe) {
	   link_map[l_index].params[1].delay.delay =
	     link_map[l_index].params[0].delay.delay;
	   link_map[l_index].params[1].delay.dist =
	     link_map[l_index].params[0].delay.dist;
	 }
      }
      else if (strcmp(argtype,"DELAYQUANTUM")== 0){
	 if (debug)
	   info("Delayq = %d\n", atoi(argvalue));
	 dq = atoi(argvalue);
      }
      else if (strcmp(argtype,"DELAYQUANTABLE")== 0){
	 if (debug)
	   info("Delayqt = %s\n", argvalue);
	 
	 free(link_map[l_index].params[p_num].delay.quantum);
	 link_map[l_index].params[p_num].delay.quantum =
	   parse_int_table(argvalue,
			   &link_map[l_index].params[p_num].delay.qentries);
	 if (! gotpipe) {
	   link_map[l_index].params[1].delay.qentries =
	     link_map[l_index].params[0].delay.qentries;
	   free(link_map[l_index].params[1].delay.quantum);
	   link_map[l_index].params[1].delay.quantum =
	     tabledup(link_map[l_index].params[0].delay.quantum,
		      link_map[l_index].params[0].delay.qentries);
	 }
      }
      else if (strcmp(argtype,"DELAYMEAN")== 0){
	 if (debug)
	   info("Delay mean = %d\n", atoi(argvalue));
	 link_map[l_index].params[p_num].delay.mean = atoi(argvalue);
	 if (! gotpipe) {
	   link_map[l_index].params[1].delay.mean =
	     link_map[l_index].params[0].delay.mean;
	 }
      }
      else if (strcmp(argtype,"DELAYSTDDEV")== 0){
	 if (debug)
	   info("Delay stddev = %d\n", atoi(argvalue));
	 link_map[l_index].params[p_num].delay.stddev = atoi(argvalue);
	 if (! gotpipe) {
	   link_map[l_index].params[1].delay.stddev =
	     link_map[l_index].params[0].delay.stddev;
	 }
      }
      else if (strcmp(argtype,"DELAYDIST")== 0){
	 if (debug)
	   info("delaydist = %s\n", argvalue);

	 link_map[l_index].params[p_num].delay.dist =
	   dist_name_to_enum(argvalue,
			     link_map[l_index].params[p_num].delay.dist);
	 if (link_map[l_index].params[p_num].delay.dist == DN_DIST_POISSON)
	     dodpois = 1;
	 if (! gotpipe) {
	   link_map[l_index].params[1].delay.dist =
	     link_map[l_index].params[0].delay.dist;
	 }
      }
      else if (strcmp(argtype,"DELAYTABLE")== 0){
	 if (debug)
	   info("delaytable = %s\n", argvalue);

	 free(link_map[l_index].params[p_num].delay.table);
	 link_map[l_index].params[p_num].delay.table =
	   parse_int_table(argvalue,
			   &link_map[l_index].params[p_num].delay.entries);
	 if (! gotpipe) {
	   link_map[l_index].params[1].delay.entries =
	     link_map[l_index].params[0].delay.entries;
	   free(link_map[l_index].params[1].delay.table);
	   link_map[l_index].params[1].delay.table =
	     tabledup(link_map[l_index].params[0].delay.table,
		      link_map[l_index].params[0].delay.entries);
	 }
      }
      
      /* PLR parameters. */
      else if (strcmp(argtype,"PLR")== 0){
	 if (debug)
	   info("Plr = %f\n", atof(argvalue));
	 link_map[l_index].params[p_num].loss.plr =
	   (int)(atof(argvalue) * 0x7fffffff);
	 link_map[l_index].params[p_num].loss.dist = DN_DIST_CONST_RATE;
	 link_map[l_index].params[p_num].loss.qentries = 0;
	 free(link_map[l_index].params[p_num].loss.quantum);
	 link_map[l_index].params[p_num].loss.quantum = NULL;
	 if (! gotpipe) {
	   link_map[l_index].params[1].loss.plr =
	     link_map[l_index].params[0].loss.plr;
	   link_map[l_index].params[1].loss.dist =
	     link_map[l_index].params[0].loss.dist;
	   link_map[l_index].params[1].loss.qentries = 0;
	   free(link_map[l_index].params[1].loss.quantum);
	   link_map[l_index].params[1].loss.quantum = NULL;
	 }
	 if (debug)
	   info("plr = %x\n", link_map[l_index].params[p_num].loss.plr);
      }
      else if (strcmp(argtype,"PLRQUANTUM")== 0){
	 if (debug)
	   info("plrq = %d\n", atoi(argvalue));
	 lq = atoi(argvalue);
      }
      else if (strcmp(argtype,"PLRQUANTABLE")== 0){
	 if (debug)
	   info("plrqt = %s\n", argvalue);
	 
	 free(link_map[l_index].params[p_num].loss.quantum);
	 link_map[l_index].params[p_num].loss.quantum =
	   parse_int_table(argvalue,
			   &link_map[l_index].params[p_num].loss.qentries);
	 if (! gotpipe) {
	   link_map[l_index].params[1].loss.qentries =
	     link_map[l_index].params[0].loss.qentries;
	   free(link_map[l_index].params[1].loss.quantum);
	   link_map[l_index].params[1].loss.quantum =
	     tabledup(link_map[l_index].params[0].loss.quantum,
		      link_map[l_index].params[0].loss.qentries);
	 }
      }
      else if (strcmp(argtype,"PLRMEAN")== 0){
	 if (debug)
	   info("plr mean = %f\n", atof(argvalue));
	 link_map[l_index].params[p_num].loss.mean = 
	   (int)(atof(argvalue) * 0x7fffffff);
	 if (! gotpipe) {
	   link_map[l_index].params[1].loss.mean =
	     link_map[l_index].params[0].loss.mean;
	 }
      }
      else if (strcmp(argtype,"PLRSTDDEV")== 0){
	 if (debug)
	   info("plr stddev = %f\n", atof(argvalue));
	 link_map[l_index].params[p_num].loss.stddev =
	   (int)(atof(argvalue) * 0x7fffffff);
	 if (! gotpipe) {
	   link_map[l_index].params[1].loss.stddev =
	     link_map[l_index].params[0].loss.stddev;
	 }
      }
      else if (strcmp(argtype,"PLRDIST")== 0){
	 if (debug)
	   info("plrdist = %s\n", argvalue);

	 link_map[l_index].params[p_num].loss.dist =
	   dist_name_to_enum(argvalue,
			     link_map[l_index].params[p_num].loss.dist);
	 if (link_map[l_index].params[p_num].loss.dist == DN_DIST_POISSON)
	     dolpois = 1;
	 if (! gotpipe) {
	   link_map[l_index].params[1].loss.dist =
	     link_map[l_index].params[0].loss.dist;
	 }
      }
      else if (strcmp(argtype,"PLRTABLE")== 0){
	 if (debug)
	   info("plrtable = %s\n", argvalue);

	 free(link_map[l_index].params[p_num].loss.table);
	 link_map[l_index].params[p_num].loss.table =
	   parse_loss_table(argvalue,
			    &link_map[l_index].params[p_num].loss.entries);
	 if (! gotpipe) {
	   link_map[l_index].params[1].loss.entries =
	     link_map[l_index].params[0].loss.entries;
	   free(link_map[l_index].params[1].loss.table);
	   link_map[l_index].params[1].loss.table =
	     tabledup(link_map[l_index].params[0].loss.table,
		      link_map[l_index].params[0].loss.entries);
	 }
      }
      else if (strcmp(argtype,"MAXINQ")== 0){
	 if (debug)
	   info("maxinq = %s\n", argvalue);

	 link_map[l_index].params[p_num].loss.maxinq = atoi(argvalue);
      }
      
       /* Queue parameters. Slightly different since we do not want
	  to set the queue params for a lan node in the from-switch
	  direction. Note, by convention the 0 pipe is to the switch
          and the 1 pipe is from the switch. */
       
       else if (strcmp(argtype,"LIMIT")== 0){
	 if (debug)
	   info("QSize/Limit = %d\n", atoi(argvalue));
	 /* set the PIPE_QSIZE_IN_BYTES flag to 0,
	    since we assume that limit is in slots/packets by default
	 */
	 link_map[l_index].params[p_num].q_size = atoi(argvalue);
	 link_map[l_index].params[p_num].flags_p &= ~PIPE_QSIZE_IN_BYTES;
	 if (!gotpipe && !islan) {
	   link_map[l_index].params[1].q_size =
		   link_map[l_index].params[0].q_size;
	   link_map[l_index].params[0].flags_p =
		   link_map[l_index].params[1].flags_p;
	 }
       }

       else if (strcmp(argtype,"QUEUE-IN-BYTES")== 0){
	 int qsztype = atoi(argvalue);
	 if(qsztype == 0){
	   if (debug)
	     info("QSize in slots/packets");
	   link_map[l_index].params[p_num].flags_p &= ~PIPE_QSIZE_IN_BYTES;
	 }
	 else {
	   if (debug)
	     info("QSize in bytes\n");
	   link_map[l_index].params[p_num].flags_p |= PIPE_QSIZE_IN_BYTES;
	 }
	 if (!gotpipe && !islan) {
	   link_map[l_index].params[1].flags_p =
		   link_map[l_index].params[0].flags_p;
	 }
       }
       else if(strcmp(argtype,"MAXTHRESH")== 0){
	 if (debug)
	   info("Maxthresh = %d \n", atoi(argvalue));
	 link_map[l_index].params[p_num].red_gred_params.max_th =
		 atoi(argvalue);
	 if (!gotpipe && !islan) {
	   link_map[l_index].params[1].red_gred_params.max_th =
		   link_map[l_index].params[0].red_gred_params.max_th;
	 }
       }
       else if(strcmp(argtype,"THRESH")== 0){
	 if (debug)
	   info("Thresh = %d \n", atoi(argvalue));
	 link_map[l_index].params[p_num].red_gred_params.min_th =
		 atoi(argvalue);
	 if (!gotpipe && !islan) {
	    link_map[l_index].params[1].red_gred_params.min_th =
		    link_map[l_index].params[0].red_gred_params.min_th;
	 }
       }
       else if(strcmp(argtype,"LINTERM")== 0){
	 if (debug)
	   info("Linterm = %f\n", 1.0 / atof(argvalue));
	 link_map[l_index].params[p_num].red_gred_params.max_p =
		 1.0 / atof(argvalue);
	 if (!gotpipe && !islan) {
	   link_map[l_index].params[1].red_gred_params.max_p =
		   link_map[l_index].params[0].red_gred_params.max_p;
	 }
       }
       else if(strcmp(argtype,"Q_WEIGHT")== 0){
	 if (debug)
	   info("Qweight = %f\n", atof(argvalue));
	 link_map[l_index].params[p_num].red_gred_params.w_q =
		 atof(argvalue);
	 if (!gotpipe && !islan) {
	   link_map[l_index].params[1].red_gred_params.w_q =
		   link_map[l_index].params[0].red_gred_params.w_q;
	 }
       }
       else if(strcmp(argtype,"PIPE")== 0){
	 if (debug)
	   info("PIPE = %s\n", argvalue);

	 gotpipe++;
	 if(strcmp(argvalue, "pipe0") == 0)
	   p_num = 0;
	 else if(strcmp(argvalue, "pipe1") == 0)
	   p_num = 1;
	 else if(isdigit(argvalue[0])) {
	   int pipeno = atoi(argvalue);

	   if(link_map[l_index].pipes[0] == pipeno)
	     p_num = 0;
	   else if(link_map[l_index].pipes[1] == pipeno)
	     p_num = 1;
	   else {
	     error("unrecognized pipe number\n");
	     return -1;
	   }
	 }
	 else {
	   error("unrecognized pipe argument\n");
	   return -1;
	 }
       }
       else if(strcmp(argtype,"DEST")== 0 ||
	       strcmp(argtype,"SRCPORT") == 0 ||
	       strcmp(argtype,"DSTPORT") == 0 ||
	       strcmp(argtype,"PROTOCOL") == 0){
       }
       else {
	 error("unrecognized argument\n");
	 error("%s -> %s \n", argtype, argvalue);
       }
       argtype = strsep(&temp,"=");
     }

    if (bq != -1) {
      link_map[l_index].params[p_num].bw.qentries =
	link_map[l_index].params[p_num].bw.entries;
      link_map[l_index].params[p_num].bw.quantum =
	expand_int_table(bq, link_map[l_index].params[p_num].bw.entries);
      if (! gotpipe) {
	link_map[l_index].params[1].bw.qentries =
	  link_map[l_index].params[1].bw.entries;
	link_map[l_index].params[1].bw.quantum =
	  expand_int_table(bq, link_map[l_index].params[1].bw.entries);
      }
    }
    if (dq != -1) {
      link_map[l_index].params[p_num].delay.qentries =
	link_map[l_index].params[p_num].delay.entries;
      link_map[l_index].params[p_num].delay.quantum =
	expand_int_table(dq, link_map[l_index].params[p_num].delay.entries);
      if (! gotpipe) {
	link_map[l_index].params[1].delay.qentries =
	  link_map[l_index].params[1].delay.entries;
	link_map[l_index].params[1].delay.quantum =
	  expand_int_table(dq, link_map[l_index].params[1].delay.entries);
      }
    }
    if (lq != -1) {
      link_map[l_index].params[p_num].loss.qentries =
	link_map[l_index].params[p_num].loss.entries;
      link_map[l_index].params[p_num].loss.quantum =
	expand_int_table(lq, link_map[l_index].params[p_num].loss.entries);
      if (! gotpipe) {
	link_map[l_index].params[1].loss.qentries =
	  link_map[l_index].params[1].loss.entries;
	link_map[l_index].params[1].loss.quantum =
	  expand_int_table(lq, link_map[l_index].params[1].loss.entries);
      }
    }
  }
  if (gotpipe)
    *pipe_which = p_num;

  /* Poisson distribution tables are generated below. */
  if (dobpois) {
    free(link_map[l_index].params[p_num].bw.table);
    link_map[l_index].params[p_num].bw.table =
      build_poisson(link_map[l_index].params[p_num].bw.mean,
		    &link_map[l_index].params[p_num].bw.entries);
    if (! gotpipe) {
      free(link_map[l_index].params[1].bw.table);
      link_map[l_index].params[1].bw.table =
	build_poisson(link_map[l_index].params[1].bw.mean,
		      &link_map[l_index].params[1].bw.entries);
    }
  }
  if (dodpois) {
    free(link_map[l_index].params[p_num].delay.table);
    link_map[l_index].params[p_num].delay.table =
      build_poisson(link_map[l_index].params[p_num].delay.mean,
		    &link_map[l_index].params[p_num].delay.entries);
    if (! gotpipe) {
      free(link_map[l_index].params[1].delay.table);
      link_map[l_index].params[1].delay.table =
	build_poisson(link_map[l_index].params[1].delay.mean,
		      &link_map[l_index].params[1].delay.entries);
    }
  }
  if (dolpois) {
    int i;
    
    free(link_map[l_index].params[p_num].loss.table);
    link_map[l_index].params[p_num].loss.table =
      build_poisson(link_map[l_index].params[p_num].loss.mean>>0x10,
		    &link_map[l_index].params[p_num].loss.entries);
    for (i = 0; i < link_map[l_index].params[p_num].loss.entries; i++) {
      link_map[l_index].params[p_num].loss.table[i] <<= 0x10;
    }
    if (! gotpipe) {
      free(link_map[l_index].params[1].loss.table);
      link_map[l_index].params[1].loss.table =
	build_poisson(link_map[l_index].params[1].loss.mean>>0x10,
		      &link_map[l_index].params[1].loss.entries);
      for (i = 0; i < link_map[l_index].params[1].loss.entries; i++) {
	link_map[l_index].params[1].loss.table[i] <<= 0x10;
      }
    }
  }
  
  return 1;
}
 /**************** get_link_info ****************************
  This builds up the initial link_map table's pipe params
   by getting the parameters from Dummynet
 **************** get_link_info ****************************/

int get_link_info(){
  int l_index = 0;
  while(l_index < link_index){
    if(get_link_params(l_index) == 0)
      return 0;
    if((int)(link_map[l_index].params[0].loss.plr) == 1
       || (int)(link_map[l_index].params[1].loss.plr) == 1)
      link_map[l_index].stat = LINK_DOWN;
    else
      link_map[l_index].stat = LINK_UP;
    l_index++;
  }
  return 1;
}
/********************************FUNCTION DEFS *******************/
