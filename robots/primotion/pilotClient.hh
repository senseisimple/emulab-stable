/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file pilotClient.hh
 *
 * Header file for the pilotClient class.
 */

#ifndef _pilot_client_hh
#define _pilot_client_hh

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <list>

#include "mtp.h"

#include "wheelManager.hh"

/**
 * Class used to track clients of the garcia-pilot daemon.
 */
class pilotClient
{

public:

    /**
     * Convenience typedef for a list of pointers to clients.
     */
    typedef std::list<pilotClient *> list;

    /**
     * Convenience typedef for a list iterator.
     */
    typedef std::list<pilotClient *>::iterator iterator;

    /**
     * Reference to the RMC client that is controlling the robot's body.  Only
     * one RMC client can be control at a time.  This pointer will
     * automatically be cleared by the destructor.
     */
    static pilotClient *pc_rmc_client;

    /**
     * Construct a pilotClient with the given values.
     *
     * @param fd The file descriptor.
     * @param wm The reference to the wheelManager that should be used to drive
     * the robot.
     */
    pilotClient(int fd, wheelManager &wm);

    /**
     * Destruct a pilotClient object.  This method will close the client
     * connection and, if this was an RMC connection, it will stop any motion
     * in progress and clear the pc_rmc_client field.
     */
    virtual ~pilotClient();

    /**
     * @return A pointer to the mtp_handle that is connected to the peer.
     */
    mtp_handle_t getHandle(void) { return this->pc_handle; };

    /**
     * @return The client's role, or zero if an initialization packet has not
     * been received yet.
     */
    mtp_role_t getRole(void) { return this->pc_role; };

    /**
     * Check an fd_set to see if the client's fd is set.
     *
     * @param readyfds The fd_set to check against.
     * @return True if the client's fd is set, false otherwise.
     */
    bool isFDSet(fd_set *readyfds)
    { return FD_ISSET(this->pc_handle->mh_fd, readyfds); }

    /**
     * Set the bit for the client's fd in the given fd_set.
     *
     * @param fds The fd_set to mutate.
     */
    void setFD(fd_set *fds) { FD_SET(this->pc_handle->mh_fd, fds); }

    /**
     * Clear the bit for the client's fd in the given fd_set.
     *
     * @param fds The fd_set to mutate.
     */
    void clearFD(fd_set *fds) { FD_CLR(this->pc_handle->mh_fd, fds); }

    /**
     * Handle a packet received from the client.  Initially, an
     * MTP_CONTROL_INIT packet is expected, so the server can determine the
     * role of the client.  After the init packet, an RMC client can send
     * MTP_COMMAND_GOTO's and MTP_COMMAND_STOP's to move the robot.  Any other
     * packets are considered an error.
     *
     * @param mp The packet to dispatch.
     * @return True if the packet was handled successfully, false otherwise.
     */
    virtual bool handlePacket(mtp_packet_t *mp, list &notify_list);
    
private:

    /**
     * The MTP connection to the peer.
     */
    mtp_handle_t pc_handle;

    /**
     * The role of the client, or zero if an initialization packet has not yet
     * been received.
     */
    mtp_role_t pc_role;

    /**
     * The wheelManager the RMC client will use to drive the robot.
     */
    wheelManager &pc_wheel_manager;
    
};

#endif
