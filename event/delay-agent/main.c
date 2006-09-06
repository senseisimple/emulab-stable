/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * agent-main.c --
 *
 *      Delay node agent main file
 *
 */

/*********************************INCLUDES********************************/
#define REAL_WORLD 1

#if REAL_WORLD
  #include "main.h"
#else
  #include "main-d.h"
#endif
/*********************************INCLUDES********************************/

/************************GLOBALS*****************************************/
/* for reading in lines from config. files*/
char line[MAX_LINE_LENGTH];

/* address_tuple_t for subscribing to events*/
address_tuple_t event_t = NULL;

/* handle returned by event_register*/
event_handle_t handle;

/* temporary buffer for forming the server URL*/
char buf[BUFSIZ];

/* This holds the mapping between links as relevant to the event system,
   and the physical interfaces and pipe numbers
 */
structlink_map link_map[MAX_LINKS];

/* holds the number of entries in the link_map*/
int link_index = 0;

/* Raw socket*/
int s_dummy;

/* agent IP address */
char *ipaddr = NULL;

/* my pid/eid for event subscription */
char *myexp  = NULL;

/* The list of linknames in tuple format, for the event subscription */
char myobjects[1024];
char lanobjects[1024];

int debug = 0;

char buf_link [MAX_LINKS][MAX_LINE_LENGTH];

void reset_callback(event_handle_t handle,
		    event_notification_t notification, void *data);
char *myvnode;

int main(int argc, char **argv)
{
  char c;
  char *server = "localhost";
  char * port  = NULL;
  char *map_file = NULL;
  char *log_file = "/tmp/agentlog";
  char *pid_file = NULL;
  char *keyfile = NULL;
  FILE *mp = NULL;
  //char *log = NULL;
  char buf[BUFSIZ];

#if REAL_WORLD
  char ipbuf[BUFSIZ];
#endif
  
  opterr = 0;

  /* get params from the optstring */
  while ((c = getopt(argc, argv, "s:p:f:dE:l:i:k:j")) != -1) {
        switch (c) {
	  case 'd':
	      debug++;
	      break;
          case 's':
              server = optarg;
              break;
	  case 'p':
	      port = optarg;
	      break;
	  case 'f':
	      map_file = optarg;
	      break;
	  case 'l':
	      log_file = optarg;
	      break;
	  case 'i':
	      pid_file = optarg;
	      break;
	  case 'E':
	      myexp = optarg;
	      break;
	  case 'k':
	      keyfile = optarg;
	      break;
	  case 'j':
	      myvnode = optarg;
	      break;
	  case '?':
          default:
	      usage(argv[0]);
	      break;
        }
    }

  /*Check if all params are specified, otherwise, print usage and exit*/
  if(NULL == server || NULL == map_file)
      usage(argv[0]);

  if (debug)
     loginit(0, log_file);
  else {
      /* Become a daemon */
      daemon(0, 0);
      
      if (log_file)
	  loginit(0, log_file);
      else
	  loginit(1, "agent-thing");
  }

  /* open the map file*/
  if(NULL == (mp = fopen(map_file,"r")))
    {
      error("cannot open %s \n", map_file);
      exit(-1);
    }

  {

    char * temp = NULL;
    char *sep = " \n";

    while(fgets(buf_link[link_index], MAX_LINE_LENGTH, mp)){
      temp = buf_link[link_index];
      link_map[link_index].linkname = strsep(&temp, sep);
      link_map[link_index].linktype = strsep(&temp, sep);

      if(!strcmp(link_map[link_index].linktype,"duplex")){
	/*
	 * By convention, the first pipe is towards the switch if its
	 * a lan node delay. This is important cause of queue params,
	 * which we do not want to set on the pipe coming from the switch.
	 */
        link_map[link_index].vnodes[0] = strsep(&temp, sep);
	link_map[link_index].vnodes[1] = strsep(&temp,sep);
        link_map[link_index].interfaces[0] = strsep(&temp, sep);
	link_map[link_index].interfaces[1] = strsep(&temp,sep);
	link_map[link_index].pipes[0] = atoi(strsep(&temp,sep));
	link_map[link_index].pipes[1] = atoi(strsep(&temp,sep));
	sprintf(link_map[link_index].linkvnodes[0], "%s-%s",
		link_map[link_index].linkname, link_map[link_index].vnodes[0]);
	sprintf(link_map[link_index].linkvnodes[1], "%s-%s",
		link_map[link_index].linkname, link_map[link_index].vnodes[1]);

	if (!strcmp(link_map[link_index].vnodes[0],
		    link_map[link_index].vnodes[1])) {
	  link_map[link_index].islan = 1;
	}
	link_map[link_index].numpipes = 2;
      }
      else{
        link_map[link_index].vnodes[0] = strsep(&temp, sep);
        link_map[link_index].interfaces[0] = strsep(&temp, sep);
	link_map[link_index].pipes[0] = atoi(strsep(&temp,sep));
	sprintf(link_map[link_index].linkvnodes[0], "%s-%s",
		link_map[link_index].linkname, link_map[link_index].vnodes[0]);
	link_map[link_index].numpipes = 1;
      }
      
      /*
       * Form the comma separated list of linkname for the subscription
       * There are two objects, one for the lan, and one for lan-vnode.
       */
      /* Do not have name yet, so add it to the string */
      if (strlen(myobjects)) {
        strcat(myobjects, ",");
        strcat(lanobjects, ",");
      }
      sprintf(&myobjects[strlen(myobjects)], "%s,%s",
	      link_map[link_index].linkname,
	      link_map[link_index].linkvnodes[0]);

      /* For the reset event below */
      sprintf(&lanobjects[strlen(lanobjects)], "%s",
	      link_map[link_index].linkname);

      if(!strcmp(link_map[link_index].linktype,"duplex") &&
	 !link_map[link_index].islan) {
        sprintf(&myobjects[strlen(myobjects)], ",%s",
		link_map[link_index].linkvnodes[1]);
      }
      link_index++;
    }
  }


  /* close the map-file*/
  fclose(mp);
    
  /* create a raw socket to configure Dummynet through setsockopt*/
  
#if REAL_WORLD
  s_dummy = socket( AF_INET, SOCK_RAW, IPPROTO_RAW );
  if ( s_dummy < 0 ){
    error("cant create raw socket\n");
    return 1;
  }

/* this gets the current pipe params from dummynet and populates
     the link map table
   */
  if (get_link_info() == 0){
    error("cant get the link pipe params from dummynet\n");
    return 1;
  }

  /* dump the link_map to log*/
  dump_link_map();
  
  
 /*
  * Get our IP address. Thats how we name ourselves to the
  * Testbed Event System. 
  */
  if (ipaddr == NULL) {
     struct hostent	*he;
     struct in_addr	myip;
	    
     if (gethostname(buf, sizeof(buf)) < 0) {
	    error("could not get hostname");
	    return 1;
	}

      if (! (he = gethostbyname(buf))) {
	   error("could not get IP address from hostname");
	   return 1;
       }
       memcpy((char *)&myip, he->h_addr, he->h_length);
       strcpy(ipbuf, inet_ntoa(myip));
       ipaddr = ipbuf;
   }
#endif
  /*
   * Write out a pidfile.
   */
  if (pid_file)
	  strcpy(buf, pid_file);
  else
	  sprintf(buf, "%s/delayagent.pid", _PATH_VARRUN);
  mp = fopen(buf, "w");
  if (mp != NULL) {
	  fprintf(mp, "%d\n", getpid());
	  (void) fclose(mp);
  }
  
  /* Convert server/port to elvin thing.
   */
   if (server) {
	    snprintf(buf, sizeof(buf), "elvin://%s%s%s",
		     server,
		     (port ? ":"  : ""),
		     (port ? port : ""));
	    server = buf;
    }
  
  /* register with the event system*/

  handle = event_register_withkeyfile(server, 0, keyfile);
   if (handle == NULL) {
       error("could not register with event system\n");
       return 1;
     }

  info("registered with the event server\n");
 /* allocate an address_tuple*/
  event_t = address_tuple_alloc();

  /* fill up the tuple, before calling event_subscribe*/
  fill_tuple(event_t);
      
  /*subscribe to the event*/
   if (event_subscribe(handle, agent_callback, event_t, NULL) == NULL) {
        error("could not subscribe to %d event\n",event_t->eventtype);
        return 1;
      }

  strcat(lanobjects, ",");
  strcat(lanobjects, ADDRESSTUPLE_ALL);
  event_t->objname   = lanobjects;
  event_t->objtype   = TBDB_OBJECTTYPE_LINK;
  event_t->eventtype = TBDB_EVENTTYPE_RESET;
  event_t->host      = ADDRESSTUPLE_ANY;
  event_t->expt      = myexp;

  if (event_subscribe(handle, reset_callback, event_t, NULL) == NULL) {
      error("could not subscribe to %d event\n",event_t->eventtype);
      return 1;
  }

  info("subscribed...\n");
  /* free the memory for the address tuple*/
  address_tuple_free(event_t);

  info("entering the main loop\n");
  /* enter the event loop */
   event_main(handle);
  
#ifdef DEBUG
  info("exiting function main\n");
#endif
  return 0;
}

/*********************** usage *********************************************
This function prints the usage to stderr and then exits with an error value

*********************** usage *********************************************/

/* prints the usage and exits with error*/
void usage(char *progname)
{
#ifdef DEBUG
  info("entering function usage\n");
#endif
  
  fprintf(stderr, "Usage: %s -s server [-p port] [-k keyfile] "
	  "-f link-map-file\n",
	  progname);
  exit(-1);
}


/************************* fill_tuple **********************************

************************** fill_tuple **********************************/

void fill_tuple(address_tuple_t at)
{
#ifdef DEBUG
  info("entering function fill_tuple\n");
#endif
  /* fill the objectname, objecttype and the eventtype from the file*/
  at->objname = myobjects;
  at->objtype = TBDB_OBJECTTYPE_LINK;
  at->eventtype = TBDB_EVENTTYPE_UP ","
	  TBDB_EVENTTYPE_DOWN "," TBDB_EVENTTYPE_MODIFY;
  at->expt = myexp;
  at->host = ADDRESSTUPLE_ANY;

  info("tuple: %s -- %s\n", myobjects, myexp);
  
  /*fill in other values, dont know what to fill in yet*/
  at->site = ADDRESSTUPLE_ANY;
  at->group= ADDRESSTUPLE_ANY;
  at->scheduler = 0;
#ifdef DEBUG
  info("leaving function fill_tuple\n");
#endif
  return;
}

/***************************dump_link_map******************************
 A debugging aid.*/
/***************************dump_link_map******************************/
void dump_link_map(){
  int i,j;
  for (i = 0; i < link_index; i++){
    info ("===============================================================\n");
    info("linkname = %s\n", link_map[i].linkname);
    info("linktype = %s\n", link_map[i].linktype);
    info("linkstatus = %d \n", link_map[i].stat);
    info("numpipes   = %d \n", link_map[i].numpipes);
    info("islan      = %d \n", link_map[i].islan);

    for (j = 0; j < link_map[i].numpipes; j++) {
      info("Pipe params:\n");
      info("interface = %s\n", link_map[i].interfaces[j]);
      info("pipe num  = %d\n", link_map[i].pipes[j]);
      info("vnode     = %s\n", link_map[i].vnodes[j]);

      info("delay = %d, bw = %d plr = %f\n",  link_map[i].params[j].delay,
	   link_map[i].params[j].bw, link_map[i].params[j].plr);
      info("q_size = %d buckets = %d n_qs = %d flags_p = %d\n",
	   link_map[i].params[j].q_size, link_map[i].params[j].buckets,
	   link_map[i].params[j].n_qs, link_map[i].params[j].flags_p);
      
      if(link_map[i].params[j].flags_p & PIPE_Q_IS_RED){
        info(" queue is RED min_th = %d max_th = %d w_q = %f max_p = %f\n",
	     link_map[i].params[j].red_gred_params.min_th,
	     link_map[i].params[j].red_gred_params.max_th,
	     link_map[i].params[j].red_gred_params.w_q,
	     link_map[i].params[j].red_gred_params.max_p);
      }
      else if(link_map[i].params[j].flags_p & PIPE_Q_IS_GRED){
	info(" queue is GRED min_th = %d max_th = %d w_q = %f max_p = %f\n",
	     link_map[i].params[j].red_gred_params.min_th,
	     link_map[i].params[j].red_gred_params.max_th,
	     link_map[i].params[j].red_gred_params.w_q,
	     link_map[i].params[j].red_gred_params.max_p);
      }
      else info("queue is droptail\n");
      info ("-----------------------------------------------------------\n");
    }
  }
}
/************************** FUNCTION DEFS *******************************/

void
reset_callback(event_handle_t handle,
		event_notification_t notification, void *data)
{
	char	buf1[BUFSIZ], buf2[BUFSIZ];
	char    *prog = "delaysetup";

	info("Got a RESET event!\n");

	if (myvnode) {
		sprintf(buf1, "%s -u -j %s", prog, myvnode);
		sprintf(buf2, "%s -i -j %s", prog, myvnode);
	}
	else {
		sprintf(buf1, "%s -u", prog);
		sprintf(buf2, "%s -i", prog);
	}
	system(buf1);
	system(buf2);
}
