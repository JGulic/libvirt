/*
 * interface_event.c: interface event queue processing helpers
 *
 * Copyright (C) 2010-2014 Red Hat, Inc.
 * Copyright (C) 2008 VirtualIron
 * Copyright (C) 2013 SUSE LINUX Products GmbH, Nuernberg, Germany.
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

#include <config.h>

#include "interface_event.h"
#include "object_event.h"
#include "object_event_private.h"
#include "datatypes.h"
#include "virlog.h"

VIR_LOG_INIT("conf.interface_event");

struct _virInterfaceEvent {
    virObjectEvent parent;

    /* Unused attribute to allow for subclass creation */
    bool dummy;
};
typedef struct _virInterfaceEvent virInterfaceEvent;
typedef virInterfaceEvent *virInterfaceEventPtr;

struct _virInterfaceEventLifecycle {
    virInterfaceEvent parent;

    int type;
    int detail;
};
typedef struct _virInterfaceEventLifecycle virInterfaceEventLifecycle;
typedef virInterfaceEventLifecycle *virInterfaceEventLifecyclePtr;

static virClassPtr virInterfaceEventClass;
static virClassPtr virInterfaceEventLifecycleClass;
static void virInterfaceEventDispose(void *obj);
static void virInterfaceEventLifecycleDispose(void *obj);

static int
virInterfaceEventsOnceInit(void)
{
    if (!(virInterfaceEventClass =
          virClassNew(virClassForObjectEvent(),
                      "virInterfaceEvent",
                      sizeof(virInterfaceEvent),
                      virInterfaceEventDispose)))
        return -1;
    if (!(virInterfaceEventLifecycleClass =
          virClassNew(virInterfaceEventClass,
                      "virInterfaceEventLifecycle",
                      sizeof(virInterfaceEventLifecycle),
                      virInterfaceEventLifecycleDispose)))
        return -1;
    return 0;
}

VIR_ONCE_GLOBAL_INIT(virInterfaceEvents)

static void
virInterfaceEventDispose(void *obj)
{
    virInterfaceEventPtr event = obj;
    VIR_DEBUG("obj=%p", event);
}


static void
virInterfaceEventLifecycleDispose(void *obj)
{
    virInterfaceEventLifecyclePtr event = obj;
    VIR_DEBUG("obj=%p", event);
}


static void
virInterfaceEventDispatchDefaultFunc(virConnectPtr conn,
                                     virObjectEventPtr event,
                                     virConnectObjectEventGenericCallback cb,
                                     void *cbopaque)
{
    virInterfacePtr iface = virGetInterface(conn, event->meta.name, NULL);
    if (!iface)
        return;

    switch ((virInterfaceEventID)event->eventID) {
    case VIR_INTERFACE_EVENT_ID_LIFECYCLE:
        {
            virInterfaceEventLifecyclePtr interfaceLifecycleEvent;

            interfaceLifecycleEvent = (virInterfaceEventLifecyclePtr)event;
            ((virConnectInterfaceEventLifecycleCallback)cb)(conn,
                                                            iface,
                                                            interfaceLifecycleEvent->type,
                                                            interfaceLifecycleEvent->detail,
                                                            cbopaque);
            goto cleanup;
        }

    case VIR_INTERFACE_EVENT_ID_LAST:
        break;
    }
    VIR_WARN("Unexpected event ID %d", event->eventID);

 cleanup:
    virObjectUnref(iface);
}


/**
 * virInterfaceEventStateRegisterID:
 * @conn: connection to associate with callback
 * @state: object event state
 * @iface: interface to filter on or NULL for all interfaces
 * @eventID: ID of the event type to register for
 * @cb: function to invoke when event occurs
 * @opaque: data blob to pass to @callback
 * @freecb: callback to free @opaque
 * @callbackID: filled with callback ID
 *
 * Register the function @cb with connection @conn, from @state, for
 * events of type @eventID, and return the registration handle in
 * @callbackID.
 *
 * Returns: the number of callbacks now registered, or -1 on error
 */
int
virInterfaceEventStateRegisterID(virConnectPtr conn,
                                 virObjectEventStatePtr state,
                                 virInterfacePtr iface,
                                 int eventID,
                                 virConnectInterfaceEventGenericCallback cb,
                                 void *opaque,
                                 virFreeCallback freecb,
                                 int *callbackID)
{
    if (virInterfaceEventsInitialize() < 0)
        return -1;

    return virObjectEventStateRegisterID(conn, state, iface ? iface->name : NULL,
                                         NULL, NULL,
                                         virInterfaceEventClass, eventID,
                                         VIR_OBJECT_EVENT_CALLBACK(cb),
                                         opaque, freecb,
                                         false, callbackID, false);
}


/**
 * virInterfaceEventStateRegisterClient:
 * @conn: connection to associate with callback
 * @state: object event state
 * @iface: interface to filter on or NULL for all interfaces
 * @eventID: ID of the event type to register for
 * @cb: function to invoke when event occurs
 * @opaque: data blob to pass to @callback
 * @freecb: callback to free @opaque
 * @callbackID: filled with callback ID
 *
 * Register the function @cb with connection @conn, from @state, for
 * events of type @eventID, and return the registration handle in
 * @callbackID.  This version is intended for use on the client side
 * of RPC.
 *
 * Returns: the number of callbacks now registered, or -1 on error
 */
int
virInterfaceEventStateRegisterClient(virConnectPtr conn,
                                     virObjectEventStatePtr state,
                                     virInterfacePtr iface,
                                     int eventID,
                                     virConnectInterfaceEventGenericCallback cb,
                                     void *opaque,
                                     virFreeCallback freecb,
                                     int *callbackID)
{
    if (virInterfaceEventsInitialize() < 0)
        return -1;

    return virObjectEventStateRegisterID(conn, state, iface ? iface->name : NULL,
                                         NULL, NULL,
                                         virInterfaceEventClass, eventID,
                                         VIR_OBJECT_EVENT_CALLBACK(cb),
                                         opaque, freecb,
                                         false, callbackID, true);
}


/**
 * virInterfaceEventLifecycleNew:
 * @name: name of the interface object the event describes
 * @type: type of lifecycle event
 * @detail: more details about @type
 *
 * Create a new interface lifecycle event.
 */
virObjectEventPtr
virInterfaceEventLifecycleNew(const char *name,
                              int type,
                              int detail)
{
    virInterfaceEventLifecyclePtr event;

    if (virInterfaceEventsInitialize() < 0)
        return NULL;

    if (!(event = virObjectEventNew(virInterfaceEventLifecycleClass,
                                    virInterfaceEventDispatchDefaultFunc,
                                    VIR_INTERFACE_EVENT_ID_LIFECYCLE,
                                    0, name, NULL, name)))
        return NULL;

    event->type = type;
    event->detail = detail;

    return (virObjectEventPtr)event;
}
