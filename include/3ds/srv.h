#ifndef _MCU_3DS_SRV_H
#define _MCU_3DS_SRV_H

#include <3ds/result.h>
#include <3ds/err.h>
#include <3ds/svc.h>
#include <3ds/ipc.h>

extern u32 srv_refcount;
extern Handle srv_session;

Result srvInit();
void srvExit();

// ipc cmd ids
enum
{
    ID_SRV_RegisterClient      = 0x0001,
    ID_SRV_EnableNotification  = 0x0002,
    ID_SRV_RegisterService     = 0x0003,
    ID_SRV_UnregisterService   = 0x0004,
    ID_SRV_GetServiceHandle    = 0x0005,
    ID_SRV_RegisterPort        = 0x0006,
    ID_SRV_UnregisterPort      = 0x0007,
    ID_SRV_ReceiveNotification = 0x000B,
    ID_SRV_PublishToSubscriber = 0x000C,
    // don't need any more
};

enum SRV_PublishNotificationFlags
{
	SRVNOTIF_ONLY_IF_NOT_PENDING    = BIT(0),
	SRVNOTIF_NO_ERROR_ON_QUEUE_FULL = BIT(1),
};

Result SRV_RegisterClient();
Result SRV_EnableNotification(Handle *sempahore);
Result SRV_RegisterService(Handle *service, const char *service_name, u32 service_name_len, u32 max_sessions);
Result SRV_UnregisterService(const char *service_name, u32 service_name_length);
Result SRV_GetServiceHandle(Handle *service, const char *service_name, u32 service_name_length, u32 flags);
Result SRV_RegisterPort(const char *port_name, u32 port_name_length, Handle client_port);
Result SRV_UnregisterPort(const char *port_name, u32 port_name_length);
Result SRV_ReceiveNotification(u32 *notification_id);
Result SRV_PublishToSubscriber(u32 notification_id, u32 flags);

#endif