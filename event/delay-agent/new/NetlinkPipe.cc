// NetlinkPipe.cc

#ifdef LINUX

#include "lib.hh"
#include "NetlinkPipe.hh"

extern "C"
{
#include <sys/types.h>
#include <sys/socket.h>

#include <netlink/list.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/route.h>
#include <netlink/route/link.h>
#include <netlink/route/class.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/sch/plr.h>
#include <netlink/route/sch/delay.h>
#include <netlink/route/sch/htb.h>
}

using namespace std;

NetlinkPipe::NetlinkPipe(std::string const & iface, std::string const & pipeno)
{
        interfaceName = iface;
        pipeNumber = pipeno;
        nl_handle = NULL;
        class_cache = NULL;
        qdisc_cache = NULL;

        init();
        /* test(); */
}

void NetlinkPipe::test(void)
{
        Parameter testParam(Parameter::LOSS, 50000);

        updateParameter(testParam);
}

static uint32_t strIntAdd(const char *s,int inc) {
    uint32_t retval;
    char *buf;
    int i = atoi(s);
    i += inc;
    buf = (char *)malloc(256);
    sprintf(buf,"%d",i);
    retval = strtoul(buf,NULL,16);
    free(buf);
    return retval;
}

int NetlinkPipe::init(void)
{
        struct nl_cache *link_cache;
        uint32_t handle;
        string str;

        link_cache = NULL;

        cerr << "Got pipe number " << pipeNumber << endl;
        handle = (uint32_t)hexStringToInt(pipeNumber);
        cerr << "handle: " << handle << endl;

        nl_handle = nl_handle_alloc();
        if (nl_handle == NULL) {
                cerr << "Unable to allocate nl_handle" << endl;
                return -1;
        }

        if (nl_connect(nl_handle, NETLINK_ROUTE) < 0) {
                cerr << "Unable to allocate Netlink socket" << endl;
                return -1;
        }
        link_cache = rtnl_link_alloc_cache(nl_handle);
        if (link_cache == NULL) {
                cerr << "Unable to allocate link cache" << endl;
                return -1;
        }

        ifindex = rtnl_link_name2i(link_cache, interfaceName.c_str());
        if (ifindex == RTNL_LINK_NOT_FOUND) {
                cerr << "Unable to translate link name to ifindex" << endl;
                return -1;
        }
        nl_cache_free(link_cache);

        qdisc_cache = rtnl_qdisc_alloc_cache(nl_handle);
        if (qdisc_cache == NULL) {
                cerr << "Unable to allocate qdisc cache" << endl;
                return -1;
        }

        plrHandle = handle << 16;
        delayHandle = strIntAdd(pipeNumber.c_str(),10) << 16;
        htbHandle = strIntAdd(pipeNumber.c_str(),20) << 16;

        class_cache = rtnl_class_alloc_cache(nl_handle, ifindex);
        if (class_cache == NULL) {
                cerr << "Unable to allocate class cache" << endl;
                return -1;
        }

        htbClassHandle = htbHandle + 1;

        cerr << "handles: (" << plrHandle << "," << delayHandle << "," << htbHandle << "," << htbClassHandle << ")" << endl;

        return 0;
}

NetlinkPipe::~NetlinkPipe()
{
        if (qdisc_cache)
                nl_cache_free(qdisc_cache);

        if (class_cache)
                nl_cache_free(class_cache);

        if (nl_handle)
                nl_close(nl_handle);
}

void NetlinkPipe::reset(void)
{
        map<Parameter::ParameterType, Parameter>::iterator pos = g::defaultParameters.begin();
        map<Parameter::ParameterType, Parameter>::iterator limit = g::defaultParameters.end();

        for (; pos != limit; ++pos)
        {
                updateParameter(pos->second);
        }
}

void NetlinkPipe::resetParameter(Parameter const & newParameter)
{
        updateParameter(newParameter);
}

/*
 * useful for debugging qdisc caches:
 *   nl_cache_foreach(qdisc_cache,print_qdisc,NULL);
 */
static void print_qdisc(struct nl_object *obj,void *arg) {
    struct rtnl_qdisc *q = (struct rtnl_qdisc *)obj;
    printf("qdisc handle %d, type %s\n",rtnl_qdisc_get_handle(q),
           rtnl_qdisc_get_kind(q));
}

void NetlinkPipe::updateParameter(Parameter const & newParameter)
{
        struct rtnl_qdisc *qdisc;
        struct rtnl_class *htbClass;

        qdisc = NULL;

        switch(newParameter.getType()) {
                case Parameter::BANDWIDTH:
                                htbClass = rtnl_class_get(class_cache, htbClassHandle);
                                if (htbClass == NULL) {
                                        cerr << "Couldn't find htb class " << htbClassHandle << endl;
                                        return;
                                }
                                rtnl_htb_set_rate(htbClass, floor(newParameter.getValue() * 1000 / 8.0));
                                rtnl_htb_set_ceil(htbClass, floor(newParameter.getValue() * 1000 / 8.0));
                                rtnl_class_change(nl_handle, htbClass, NULL);
                                rtnl_class_put(htbClass);
                                break;
                case Parameter::DELAY:
                                qdisc = rtnl_qdisc_get(qdisc_cache, ifindex, delayHandle);
                                if (qdisc == NULL) {
                                        cerr << "Couldn't find delay qdisc " << delayHandle << endl;
                                        return;
                                }
                                rtnl_delay_set_delay(qdisc, newParameter.getValue() * 1000);
                                rtnl_qdisc_change(nl_handle, qdisc, NULL);
                                rtnl_qdisc_put(qdisc);
                                break;
                case Parameter::LOSS:
                                qdisc = rtnl_qdisc_get(qdisc_cache, ifindex, plrHandle);
                                if (qdisc == NULL) {
                                        cerr << "Couldn't find plr qdisc " << plrHandle << endl;
                                        return;
                                }
                                rtnl_plr_set_plr(qdisc, newParameter.getValue());
                                rtnl_qdisc_change(nl_handle, qdisc, NULL);
                                rtnl_qdisc_put(qdisc);
                                break;
                case Parameter::LINK_UP:
                                uint32_t value;

                                if (newParameter.getValue()) {
                                        value = 0;
                                }
                                else {
                                        value = 0xffffffff;
                                }
                                qdisc = rtnl_qdisc_get(qdisc_cache, ifindex, plrHandle);
                                if (qdisc == NULL) {
                                        cerr << "Couldn't find plr qdisc " << plrHandle << endl;
                                        return;
                                }
                                rtnl_plr_set_plr(qdisc, value);
                                rtnl_qdisc_change(nl_handle, qdisc, NULL);
                                rtnl_qdisc_put(qdisc);
                                break;
                default:
                                break;
        }
}

#endif
