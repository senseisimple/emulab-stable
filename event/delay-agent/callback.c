/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
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
/******************************* INCLUDES **************************/


/******************************* EXTERNS **************************/
extern structlink_map link_map[MAX_LINKS];
extern int link_index;
extern int s_dummy; 
extern int debug;
/******************************* EXTERNS **************************/


/********************************FUNCTION DEFS *******************/


/*************************** agent_callback **********************
 This function is called from the event system when an event
 notification is recd. from the server. It checks whether the 
 notification is valid (sanity check). If not print a warning,else
 call handle_pipes which does the rest of thejob
 *************************** agent_callback **********************/

void agent_callback(event_handle_t handle,
		    event_notification_t notification, void *data)
{
  #define MAX_LEN 50

  char objname[MAX_LEN];
  char eventtype[MAX_LEN];
  int  l_index;

    /* get the name of the object, eg. link0 or link1*/
  if(event_notification_get_string(handle,
                                   notification,
                                   "OBJNAME", objname, MAX_LEN) == 0){
    error("could not get the objname \n");
    return;
  }

  /* check we are handling the objectname for which we have recd an event */
    if ((l_index = check_object(objname)) == -1){
    error("not handling events for this object\n");
    return;
  }

  /* get the eventtype, eg up/down/modify*/
  if(event_notification_get_string(handle,
                                   notification,
                                   "EVENTTYPE", eventtype, MAX_LEN) == 0){
    error("could not get the eventtype \n");
    return;
  }

  /* call function to handle this event for this object */
  handle_pipes(objname, eventtype,notification, handle,l_index);
   
  return;
}

/******************** handle_pipes ***************************************
This dispatch function checks the event type and dispatches to the appropriate
routine to handle
 ******************** handle_pipes ***************************************/

void handle_pipes (char *objname, char *eventtype,
		   event_notification_t notification, event_handle_t handle,
		   int l_index)
{
  
  /*link_map[index] contains the relevant info*/

  /* as of now, we only support duplex links in the testbed. Also we
     require that both pipes of the duplex link are changed during the
     event callback. Later when we add simplex links, we will also add
     support to send event which will have effect on only one side of a
     duplex link.
   */
  if(strcmp(link_map[l_index].linktype,"duplex") == 0){

    if(strcmp(eventtype, TBDB_EVENTTYPE_UP) == 0){
      handle_link_up(objname, l_index);
    }
    else if(strcmp(eventtype, TBDB_EVENTTYPE_DOWN) == 0){
      handle_link_down(objname, l_index);
    }
    else if(strcmp(eventtype, TBDB_EVENTTYPE_MODIFY) == 0){
      handle_link_modify(objname, l_index, handle, notification);
    }
    else error("unknown link event type\n");

  }
   else {
     error( "not handling simplex links yet ");
     return;
   }

  if(debug){
    system ("echo ======================================== >> /tmp/ipfw.log"); 
    system("(date;echo PARAMS ; ipfw pipe show all) >> /tmp/ipfw.log");
  }
}

/***************** checkevent **************************************
 * checks if we are handling the link for which we got the event
 ***************** checkevent **************************************/

int check_object (char *objname)
{
  return search(objname);
}


/*********************** search ********************************
This function does a linear search on the link_map and returns
the index of the link_map entry which matches with the objectname
from the event notification

There will hardly be a few entries in this table, so we dont need
anything fancier than a linear search.

*********************** search ********************************/

int search(char* objname){
  /* for now we do a linear search, maybe bin search later*/
  int i;
  for(i = 0; i < link_index; i++){
    if(strcmp(link_map[i].linkname, objname) == 0)
      return i;
  }
  return -1;
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
  info("==========================================\n");
   info("recd. UP event for link = %s\n", linkname);

   /* no need to do anything if link is already up*/
   if(link_map[l_index].stat == LINK_UP)
      return;
   
#if 1
  /* 0 => dont blackhole, get the plr from the link map table
     2 => both pipes
       -1 => both pipes*/
  set_link_params(l_index, 0, 2, -1);
  
   link_map[l_index].stat = LINK_UP;
    info("==========================================\n");
#endif
  
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
    info("==========================================\n");
    info("recd. DOWN event for link = %s\n", linkname);

    /* if link is already down, no need to do anything*/
    if(link_map[l_index].stat == LINK_DOWN)
      return;
#if 1
  if(get_link_params(l_index) == 1){
    /* 1 => set plr = 1.0 to blackhole packets
       2 => both pipes
       -1 => both pipes*/
    set_link_params(l_index, 1, 2, -1); 
    link_map[l_index].stat = LINK_DOWN;
  }
  else error("could not get params\n");
#endif
  info("==========================================\n");
  
}

/*********** handle_link_modify *****************************
It gets the new link params from the notification and updates
the link_map table with these params. If the link is down,
it just returns. IF the link is up, then it calls set_link_params
to set the new params
 *********** handle_link_modify *****************************/

void handle_link_modify(char * linkname, int l_index,
			event_handle_t handle,
			event_notification_t notification)
{
  /* Get the new pipe params from the notification, and then
     update the new set of params by setting the params in
     dummynet
   */
  int p_which = -1;
  int n_pipes = 0;
//  char argstring[50];

  info("==========================================\n");
  info("recd. MODIFY event for link = %s\n", linkname);
  
  /* if the link is up, then get the params from dummynet,
     get the params from the notification and then merge
     and write back to dummynet
   */
  if(link_map[l_index].stat == LINK_UP){
    if(get_link_params(l_index) == 1)
      if(get_new_link_params(l_index, handle, notification,
			     &n_pipes,&p_which) == 1)
	/* 0 => dont blackhole, get the plr
	 * from the link map table*/
	/*info(" n_pipes = %d p_which = %d\n", n_pipes, p_which);*/
	set_link_params(l_index, 0, n_pipes, p_which);
    
				    
  } else
    /* link is down, so just change in the link_map*/
     get_new_link_params(l_index, handle, notification,
			 &n_pipes, &p_which);
}

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
  while( p_index < 2){
    
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
    int l ;

    for ( ; num_bytes >= sizeof(*p) ; p = (struct dn_pipe *)next ) {
       
     
       if ( p->next != (struct dn_pipe *)DN_IS_PIPE )
	  break ;

       l = sizeof(*p) + p->fs.rq_elements * sizeof(*q) ;
       next = (void *)p  + l ;
       num_bytes -= l ;
       q = (struct dn_flow_queue *)(p+1) ;

       if (pipe_num != p->pipe_nr)
	   continue;

       /* grab pipe delay and bandwidth */
       link_map[l_index].params[p_index].delay = p->delay;
       link_map[l_index].params[p_index].bw = p->bandwidth;

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

    p_params->plr = 1.0*fs->plr/(double)(0x7fffffff);
    
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

void set_link_params(int l_index, int blackhole,int t_pipes, int p_which)
{
  /* Grab the pipe params from the link_map table and then
     set them into dummynet by calling setsockopt
   */
 /*bw, delay, plr, buckets, qsize, mask, qtype*/

  int p_index = 0;
  
  while(p_index < 2)
    {
      if((t_pipes == 2) || (t_pipes == 0) || (p_which == p_index))
	{
	    struct dn_pipe pipe;
	    /* get the params stored in the link table*/
	    structpipe_params *p_params
	      = &(link_map[l_index].params[p_index]);

	    info("entered the loop, pindex = %d\n", p_index);
	
	    memset(&pipe, 0, sizeof pipe);

	    /* set the bandwidth and delay*/
	    pipe.bandwidth = p_params->bw;
	    pipe.delay = p_params->delay;

	    /* set the pipe number*/
	    pipe.pipe_nr = link_map[l_index].pipes[p_index];

	    /* if we want to blackhole (in the case of EVENT_DOWN,
	       then we set all other pipe params same, but change the
	       plr to 1.0
	       */
      
	    if(blackhole == 1){
	      double d = 1.0;
	      pipe.fs.plr = (int)(d*0x7fffffff);
	    }
	    else{ /* we want to change plr*/
	      /* if the pipe was initially down, and if the user wants it up and he
		 has not specied any plr, then use the default plr = 0.0*/
	      if((int)(p_params->plr) == 1)
		pipe.fs.plr = (int)(0*0x7fffffff);
	      else
		pipe.fs.plr = (int)(p_params->plr*0x7fffffff);
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

	      if(pipe.bandwidth){
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
		s = clock.hz * avg_pkt_size * 8 / pipe.bandwidth;

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
      /* go on to the next pipe in the duplex link*/
      p_index++;
    }

}

/********* get_new_link_params ***************************
  For a modify event, this function gets the parameters
  from the event notification
 ********************************************************/

int get_new_link_params(int l_index, event_handle_t handle,
			 event_notification_t notification, int* p_pipes,
			 int * pipe_which)
{
  /* get the params of the pipe that were sent in the notification and
     store those values into the link_map table
   */

  char argstring[50];
  char * argtype = NULL;
  char * argvalue = NULL;
  int tot_pipes = 0;
  int p_num = -1;
  char *temp = NULL;
  
  if(event_notification_get_string(handle,
                                   notification,
                                   "ARGS", argstring, 50) != 0){
    info("ARGS = %s\n", argstring);
    temp = argstring;
    
    argtype = strsep(&temp,"=");
    
    while((argvalue = strsep(&temp," \n"))){

      if(strcmp(argtype,"BANDWIDTH")== 0){
	info("Bandwidth = %d\n", atoi(argvalue) * 1000);
	 if(p_num == -1){
	   /* this is for both pipes*/
	   link_map[l_index].params[0].bw =
	   link_map[l_index].params[1].bw =
	     atoi(argvalue) * 1000;
	 }
	 else /* do only for the specific pipe*/
	     link_map[l_index].params[p_num].bw = atoi(argvalue) * 1000;
      }

      else if (strcmp(argtype,"DELAY")== 0){
	 info("Delay = %d\n", atoi(argvalue));
	  if(p_num == -1){
	   /* this is for both pipes*/
	   link_map[l_index].params[0].delay =
	   link_map[l_index].params[1].delay =
	     atoi(argvalue);
	 }
	 else
	   link_map[l_index].params[p_num].delay = atoi(argvalue);
       }

       else if (strcmp(argtype,"PLR")== 0){
	 info("Plr = %f\n", atof(argvalue));
	 if(p_num == -1){
	   /* this is for both pipes*/
	 link_map[l_index].params[0].plr =
	   link_map[l_index].params[1].plr =
	     atof(argvalue);

	 }
	 else /* do only for the specific pipe*/
	   link_map[l_index].params[p_num].plr = atof(argvalue);
       }

       /* Queue parameters */
       
       else if (strcmp(argtype,"LIMIT")== 0){
	 info("QSize/Limit = %d\n", atoi(argvalue));
	 if(p_num == -1){
	   /* this is for both pipes*/
	   link_map[l_index].params[0].q_size =
	   link_map[l_index].params[1].q_size =
	     atoi(argvalue);
	   /* set the PIPE_QSIZE_IN_BYTES flag to 0,
	      since we assume that limit is in slots/packets by
	      default
	      */
	   link_map[l_index].params[0].flags_p &=
	     ~PIPE_QSIZE_IN_BYTES;
	   link_map[l_index].params[1].flags_p &=
	     ~PIPE_QSIZE_IN_BYTES;
	 } else /* do only for the specific pipe*/
	   {
	     link_map[l_index].params[p_num].q_size = atoi(argvalue);
	     link_map[l_index].params[p_num].flags_p &= ~PIPE_QSIZE_IN_BYTES;
	   }
       }

       else if (strcmp(argtype,"QUEUE-IN-BYTES")== 0){
	 int qsztype = atoi(argvalue);
	 if(qsztype == 0){
	   info("QSize in slots/packets");
	   /* queue size is in slots/packets*/
	   if(p_num == -1){
	     /* this is for both pipes*/
	     link_map[l_index].params[0].flags_p &=
	       ~PIPE_QSIZE_IN_BYTES;
	     link_map[l_index].params[1].flags_p &=
	       ~PIPE_QSIZE_IN_BYTES;
	   }else /* just do for the specific pipe*/
	     link_map[l_index].params[p_num].flags_p &=
	       ~PIPE_QSIZE_IN_BYTES;
	 }
	 else if(qsztype == 1)
	   {
	     info("QSize in bytes\n");
	      if(p_num == -1){
		/* this is for both pipes*/
		/* queue size is in bytes*/
		link_map[l_index].params[0].flags_p |= PIPE_QSIZE_IN_BYTES;
		link_map[l_index].params[1].flags_p |= PIPE_QSIZE_IN_BYTES;
	      } else /*do for the specific pipe*/
		link_map[l_index].params[p_num].flags_p |= PIPE_QSIZE_IN_BYTES;
	   }
       }
       else if(strcmp(argtype,"MAXTHRESH")== 0){
	 info("Maxthresh = %d \n", atoi(argvalue));
	 if(p_num == -1){
	   link_map[l_index].params[0].red_gred_params.max_th
	     = link_map[l_index].params[1].red_gred_params.max_th
	       = atoi(argvalue);
	 } else
	   link_map[l_index].params[p_num].red_gred_params.max_th
	       = atoi(argvalue);
       }
       else if(strcmp(argtype,"THRESH")== 0){
	 info("Thresh = %d \n", atoi(argvalue));
	 if(p_num == -1){
	   link_map[l_index].params[0].red_gred_params.min_th
	     = link_map[l_index].params[1].red_gred_params.min_th
	       = atoi(argvalue);
	 }else
	   link_map[l_index].params[p_num].red_gred_params.min_th
	       = atoi(argvalue);
       }
       else if(strcmp(argtype,"LINTERM")== 0){
	 info("Linterm = %f\n", 1.0 / atof(argvalue));
	 if(p_num == -1){
	   link_map[l_index].params[0].red_gred_params.max_p
	     = link_map[l_index].params[1].red_gred_params.max_p
	       = 1.0 / atof(argvalue);
	 }else
	   link_map[l_index].params[p_num].red_gred_params.max_p
	       = 1.0 / atof(argvalue);
       }
       else if(strcmp(argtype,"Q_WEIGHT")== 0){
	 info("Qweight = %f\n", atof(argvalue));
	 if(p_num == -1){
	   link_map[l_index].params[0].red_gred_params.w_q
	     = link_map[l_index].params[1].red_gred_params.w_q
	       = atof(argvalue);
	 }else
	   link_map[l_index].params[p_num].red_gred_params.w_q
	       = atof(argvalue);
       }
       else if(strcmp(argtype,"PIPE")== 0){
	 info("PIPE = %s\n", argvalue);

	 tot_pipes++;
	 if(strcmp(argvalue, "pipe0") == 0)
	     p_num = 0;
	 else if(strcmp(argvalue, "pipe1") == 0)
	   p_num = 1;
	 else error("unrecognized pipe number\n");
       }
       else {
	 error("unrecognized argument\n");
	 error("%s -> %s \n", argtype, argvalue);
       }
       argtype = strsep(&temp,"=");
     }
  }
  
  info("In func. get_new \n");
  info("tot_pips = %d p_num = %d \n", tot_pipes, p_num);
  
  *p_pipes = tot_pipes;
  *pipe_which = p_num;
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
    if((int)(link_map[l_index].params[0].plr) == 1
       || (int)(link_map[l_index].params[1].plr) == 1)
      link_map[l_index].stat = LINK_DOWN;
    else
      link_map[l_index].stat = LINK_UP;
    l_index++;
  }
  return 1;
}
/********************************FUNCTION DEFS *******************/
