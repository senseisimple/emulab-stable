// DummynetPipe.cc

#ifdef FREEBSD

#include "lib.hh"
#include "DummynetPipe.hh"

#include <sys/types.h>
#include <sys/socket.h>

using namespace std;

DummynetPipe::DummynetPipe(std::string const & pipeno)
{
        dummynetPipeNumber = stringToInt(pipeno);

        dummynetSocket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (dummynetSocket < 0) {
                cerr << "Can't create raw socket" << endl;
        }
}

DummynetPipe::~DummynetPipe()
{
        close(dummynetSocket);
}

DummynetPipe::reset(void)
{
        struct dn_pipe *pipe;

        pipe = getPipe();
        if (pipe == NULL) {
                cerr << "Couldn't find pipe for " << dummynetPipeNumber << endl;
                return;
        }

        map<Parameter::ParameterType, Parameter>::iterator pos = g::defaultParameters.begin();
        map<Parameter::ParameterType, Parameter>::iterator limit = g::defaultParameters.end();

        for (; pos != limit; ++pos)
        {
                updateParameter(pipe, pos->second);
        }

        setPipe(pipe);

        free(pipe);
}

DummynetPipe::updateParameter(struct dn_pipe* pipe, Parameter const & parameter)
{
        switch (parameter.getType())
        {
                case BANDWIDTH:
                                pipe->bandwidth = newParameter.getValue();
                                break;
                case DELAY:
                                pipe->delay = newParameter.getValue();
                                break;
#if 0
                case LOSS:
                                parameterString = "LOSS";
                                break;
#endif
        }
}

DummynetPipe::resetParameter(Parameter const & newParameter)
{
        struct dn_pipe *pipe;

        pipe = getPipe();
        if (pipe == NULL) {
                return;
        }

        updateParameter(pipe, newParameter);

        setPipe(pipe);

        free(pipe);
}

void *DummynetPipe::getPipe(void)
{
    struct dn_pipe *pipes;
    int fix_size;
    int num_bytes, num_alloc;
    void *data;
    void *next;
    struct dn_pipe *p, *pipe;
    struct dn_flow_queue *q ;
    int l ;

    data = NULL;

    fix_size = sizeof(*pipes);
    num_alloc = fix_size;
    num_bytes = num_alloc;

    data = malloc(num_bytes);
    if(data == NULL){
      cerr << "malloc: cant allocate memory" << endl;
      return 0;
    }

    while (num_bytes >= num_alloc) {
      num_alloc = num_alloc * 2 + 200;
      num_bytes = num_alloc ;
      if ((data = realloc(data, num_bytes)) == NULL){
        cerr << "cant alloc memory" << endl;
        return 0;
      }

      if (getsockopt(dummynetSocket, IPPROTO_IP, IP_DUMMYNET_GET,
             data, &num_bytes) < 0){
        cerr << "error in getsockopt" << endl;
        return 0;
      }

    }

    next = data;
    p = (struct dn_pipe *) data;
    pipe = NULL;

    for ( ; num_bytes >= sizeof(*p) ; p = (struct dn_pipe *)next ) {


       if ( DN_PIPE_NEXT(p) != (struct dn_pipe *)DN_IS_PIPE )
             break ;

       l = sizeof(*p) + p->fs.rq_elements * sizeof(*q) ;
       next = (void *)p  + l ;
       num_bytes -= l ;
       if (pipe_num != p->pipe_nr)
             continue;

       q = (struct dn_flow_queue *)(p+1) ;


#if 0
       /* grab pipe delay and bandwidth */
       link_map[l_index].getParams(p_index).setDelay(p->delay);
       link_map[l_index].getParams(p_index).setBandwidth(p->bandwidth);

       /* get flow set parameters*/
       get_flowset_params( &(p->fs), l_index, p_index);

       /* get dynamic queue parameters*/
       get_queue_params( &(p->fs), l_index, p_index);
#endif
    }

    if ((num_bytes < sizeof(*p) || (DN_PIPE_NEXT(p) != (struct dn_pipe *)DN_IS_PIPE))) {
            return NULL;
    }

    pipe = malloc(l);
    if (foo == NULL) {
        cerr << "can't allocate memory" << endl;
        return NULL;
    }

    memcpy(pipe, p, l);
    free(data);

    return pipe;
}



void DummynetPipe::setPipe(struct dn_pipe *pipe)
{
            if (setsockopt(dummynetSocket,IPPROTO_IP, IP_DUMMYNET_CONFIGURE, pipe,sizeof (*pipe))
                < 0)
              cerr << "IP_DUMMYNET_CONFIGURE setsockopt failed" << endl;
}

#endif
