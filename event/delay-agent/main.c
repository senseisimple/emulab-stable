/*
 * agent-main.c --
 *
 *      Delay node agent main file
 *
 *
 * @COPYRIGHT@
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

static int debug = 0;

char buf_link [MAX_LINKS][MAX_LINE_LENGTH];
/************************GLOBALS*****************************************/


/************************** FUNCTION DEFS *******************************/


/************************* main **************************************

 ************************* main **************************************/

int main(int argc, char **argv)
{
  char c;
  char *server = NULL;
  char * port  = NULL;
  char *map_file = NULL;
  FILE *mp = NULL;
  //char *log = NULL;
  char buf[BUFSIZ];

#if REAL_WORLD
  char ipbuf[BUFSIZ];
#endif
  
  opterr = 0;

  /* get params from the optstring */
  while ((c = getopt(argc, argv, "s:p:f:d")) != -1) {
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
     loginit(0, "/tmp/agentlog");
  else
     loginit(1, "agent-thing");

  /* open the map file*/
  if(NULL == (mp = fopen(map_file,"r")))
    {
      error("cannot open %s \n", map_file);
      exit(-1);
    }

  {

    char * temp = NULL;
    char *sep = " \n";

    while(fgets(buf_link[link_index], 256, mp)){
      temp = buf_link[link_index];
      link_map[link_index].linkname = strsep(&temp, sep);
      link_map[link_index].linktype = strsep(&temp, sep);
      link_map[link_index].interfaces[0] = strsep(&temp, sep);
	
      if(!strcmp(link_map[link_index].linktype,"duplex"))
        {
          link_map[link_index].interfaces[1] = strsep(&temp,sep);
          link_map[link_index].pipes[0] = atoi(strsep(&temp,sep));
	  link_map[link_index].pipes[1] = atoi(strsep(&temp,sep));
	}
	else{
	  link_map[link_index].pipes[0] = atoi(strsep(&temp,sep));
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

  handle = event_register(server, 0);
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

  info("subscribed...\n");
  /* free the memory for the address tuple*/
  address_tuple_free(event_t);

  info("entering the main loop\n");
  /* enter the event loop */
   event_main(handle);
  
  /*now daemonise*/

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
  
  ERROR("Usage: %s -s server [-p port] -f link-map-file\n",
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
  at->objname = ADDRESSTUPLE_ANY;
  at->objtype = TBDB_OBJECTTYPE_LINK;
  at->eventtype = ADDRESSTUPLE_ANY;
  at->host = ipaddr;
  
  /*fill in other values, dont know what to fill in yet*/
  at->site = ADDRESSTUPLE_ANY;
  at->expt = ADDRESSTUPLE_ANY;
  at->group= ADDRESSTUPLE_ANY;
  at->scheduler = 0;
#ifdef DEBUG
  info("leaving function fill_tuple\n");
#endif
  return;
}

/***************************dump_link_map******************************/
/*A debugging aid.*/
/***************************dump_link_map******************************/
void dump_link_map(){
  int i;
  for (i = 0; i < link_index; i++){
    info("linkname = %s\n", link_map[i].linkname);
    info("interface[0] = %s \t interface[1] = %s\n", link_map[i].interfaces[0], link_map[i].interfaces[1]);
    info("linktype = %s\n", link_map[i].linktype);
    info("linkstatus = %d \n", link_map[i].stat);
    info("pipes[0] = %d \t pipes[1] = %d \n", link_map[i].pipes[0], link_map[i].pipes[1]);

    info ("--------------------------------------------------\n");
    info("Pipe params.....\n");

    info("Pipe 0 .....\n");

    info("delay = %d, bw = %d plr = %f q_size = %d buckets = %d n_qs = %d flags_p = %d \n",  link_map[i].params[0].delay, link_map[i].params[0].bw, link_map[i].params[0].plr, link_map[i].params[0].q_size, link_map[i].params[0].buckets, link_map[i].params[0].n_qs, link_map[i].params[0].flags_p);

    info("Pipe 1 .....\n");

    info("delay = %d, bw = %d plr = %f q_size = %d buckets = %d n_qs = %d flags_p = %d \n",  link_map[i].params[1].delay, link_map[i].params[1].bw, link_map[i].params[1].plr, link_map[i].params[1].q_size, link_map[i].params[1].buckets, link_map[i].params[1].n_qs, link_map[i].params[1].flags_p);

    info ("--------------------------------------------------\n");
  }
}

/************************** FUNCTION DEFS *******************************/
