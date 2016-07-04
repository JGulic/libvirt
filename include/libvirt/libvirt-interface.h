/*
 * libvirt-interface.h
 * Summary: APIs for management of interfaces
 * Description: Provides APIs for the management of interfaces
 * Author: Daniel Veillard <veillard@redhat.com>
 *
 * Copyright (C) 2006-2014 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __VIR_LIBVIRT_INTERFACE_H__
# define __VIR_LIBVIRT_INTERFACE_H__

# ifndef __VIR_LIBVIRT_H_INCLUDES__
#  error "Don't include this file directly, only use libvirt/libvirt.h"
# endif

/**
 * virInterface:
 *
 * a virInterface is a private structure representing a virtual interface.
 */
typedef struct _virInterface virInterface;

/**
 * virInterfacePtr:
 *
 * a virInterfacePtr is pointer to a virInterface private structure, this is the
 * type used to reference a virtual interface in the API.
 */
typedef virInterface *virInterfacePtr;

virConnectPtr           virInterfaceGetConnect    (virInterfacePtr iface);

int                     virConnectNumOfInterfaces (virConnectPtr conn);
int                     virConnectListInterfaces  (virConnectPtr conn,
                                                   char **const names,
                                                   int maxnames);

int                     virConnectNumOfDefinedInterfaces (virConnectPtr conn);
int                     virConnectListDefinedInterfaces  (virConnectPtr conn,
                                                          char **const names,
                                                          int maxnames);
/*
 * virConnectListAllInterfaces:
 *
 * Flags used to filter the returned interfaces.
 */
typedef enum {
    VIR_CONNECT_LIST_INTERFACES_INACTIVE      = 1 << 0,
    VIR_CONNECT_LIST_INTERFACES_ACTIVE        = 1 << 1,
} virConnectListAllInterfacesFlags;

int                     virConnectListAllInterfaces (virConnectPtr conn,
                                                     virInterfacePtr **ifaces,
                                                     unsigned int flags);

virInterfacePtr         virInterfaceLookupByName  (virConnectPtr conn,
                                                   const char *name);
virInterfacePtr         virInterfaceLookupByMACString (virConnectPtr conn,
                                                       const char *mac);

const char*             virInterfaceGetName       (virInterfacePtr iface);
const char*             virInterfaceGetMACString  (virInterfacePtr iface);

typedef enum {
    VIR_INTERFACE_XML_INACTIVE = 1 << 0 /* dump inactive interface information */
} virInterfaceXMLFlags;

char *                  virInterfaceGetXMLDesc    (virInterfacePtr iface,
                                                   unsigned int flags);
virInterfacePtr         virInterfaceDefineXML     (virConnectPtr conn,
                                                   const char *xmlDesc,
                                                   unsigned int flags);

int                     virInterfaceUndefine      (virInterfacePtr iface);

int                     virInterfaceCreate        (virInterfacePtr iface,
                                                   unsigned int flags);

int                     virInterfaceDestroy       (virInterfacePtr iface,
                                                   unsigned int flags);

int                     virInterfaceRef           (virInterfacePtr iface);
int                     virInterfaceFree          (virInterfacePtr iface);

int                     virInterfaceChangeBegin   (virConnectPtr conn,
                                                   unsigned int flags);
int                     virInterfaceChangeCommit  (virConnectPtr conn,
                                                   unsigned int flags);
int                     virInterfaceChangeRollback(virConnectPtr conn,
                                                   unsigned int flags);

int virInterfaceIsActive(virInterfacePtr iface);

/**
 * VIR_INTERFACE_EVENT_CALLBACK:
 *
 * Used to cast the event specific callback into the generic one
 * for use for virConnectInterfaceEventRegisterAny()
 */
# define VIR_INTERFACE_EVENT_CALLBACK(cb)((virConnectInterfaceEventGenericCallback)(cb))

/**
 * virInterfaceEventID:
 *
 * An enumeration of supported eventId parameters for
 * virConnectInterfaceEventRegisterAny(). Each event id determines which
 * signature of callback function will be used.
 */
typedef enum {
    VIR_INTERFACE_EVENT_ID_LIFECYCLE = 0, /* virConnectInterfaceEventLifecycleCallback */

# ifdef VIR_ENUM_SENTINELS
    VIR_INTERFACE_EVENT_ID_LAST
    /*
     * NB: this enum value will increase over time as new events are
     * added to the libvirt API. It reflects the last event ID supported
     * by this version of the libvirt API.
     */
# endif
} virInterfaceEventID;

/**
 * virConnectInterfaceEventGenericCallback:
 * @conn: the connection pointer
 * @iface: the interface pointer
 * @opaque: application specified data
 *
 * A generic storage iface event callback handler, for use with
 * virConnectInterfaceEventRegisterAny(). Specific events usually
 * have a customization with extra parameters, often with @opaque being
 * passed in a different parameter position; use
 * VIR_INTERFACE_EVENT_CALLBACK() when registering an appropriate handler.
 */
typedef void (*virConnectInterfaceEventGenericCallback)(virConnectPtr conn,
                                                        virInterfacePtr iface,
                                                        void *opaque);

/* Use VIR_INTERFACE_EVENT_CALLBACK() to cast the 'cb' parameter  */
int virConnectInterfaceEventRegisterAny(virConnectPtr conn,
                                        virInterfacePtr iface, /* optional, to filter */
                                        int eventID,
                                        virConnectInterfaceEventGenericCallback cb,
                                        void *opaque,
                                        virFreeCallback freecb);

int virConnectInterfaceEventDeregisterAny(virConnectPtr conn,
                                          int callbackID);

/**
 * virInterfaceEventLifecycleType:
 *
 * a virInterfaceEventLifecycleType is emitted during interface
 * lifecycle events
 */
typedef enum {
    VIR_INTERFACE_EVENT_DEFINED = 0,
    VIR_INTERFACE_EVENT_UNDEFINED = 1,
    VIR_INTERFACE_EVENT_STARTED = 2,
    VIR_INTERFACE_EVENT_STOPPED = 3,

# ifdef VIR_ENUM_SENTINELS
    VIR_INTERFACE_EVENT_LAST
# endif
} virInterfaceEventLifecycleType;

/**
 * virConnectInterfaceEventLifecycleCallback:
 * @conn: connection object
 * @iface: interface on which the event occurred
 * @event: The specific virInterfaceEventLifeCycleType which occurred
 * @detail: contains some details on the reason of the event.
 * @opaque: application specified data
 *
 * This callback is called when a interface lifecycle action is performed, like start
 * or stop.
 *
 * The callback signature to use when registering for an event of type
 * VIR_INTERFACE_EVENT_ID_LIFECYCLE with virConnectInterfaceEventRegisterAny()
 */
typedef void (*virConnectInterfaceEventLifecycleCallback)(virConnectPtr conn,
                                                          virInterfacePtr iface,
                                                          int event,
                                                          int detail,
                                                          void *opaque);


#endif /* __VIR_LIBVIRT_INTERFACE_H__ */
