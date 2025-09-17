#include <3ds/srv.h>

u32 srv_refcount;
Handle srv_session;

Result srvInit()
{
    if (srv_refcount++)
        return 0;

    Result res;

    if (!(R_SUCCEEDED(res = svcConnectToPort(&srv_session, "srv:")) && 
          R_SUCCEEDED(res = SRV_RegisterClient())))
        srvExit();

    return res;
}

void srvExit()
{
    if (--srv_refcount)
        return;
    
    if (srv_session != 0)
    {
        T(svcCloseHandle(srv_session));
        srv_session = 0;
    }
}

Result SRV_RegisterClient()
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(ID_SRV_RegisterClient, 0, 2);
    cmdbuf[1] = IPC_Desc_CurProcessId();

    BASIC_RET(srv_session)
}

Result SRV_EnableNotification(Handle *sempahore)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(ID_SRV_EnableNotification, 0, 0);
    
    CHECK_RET(srv_session)

    *sempahore = cmdbuf[3];
    return res;
}

Result SRV_RegisterService(Handle *service, const char *service_name, u32 service_name_len, u32 max_sessions)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(ID_SRV_RegisterService, 4, 0);
    cmdbuf[1] = *((u32 *)service_name);
    cmdbuf[2] = *((u32 *)(service_name + 4));
    cmdbuf[3] = service_name_len;
    cmdbuf[4] = max_sessions;

    CHECK_RET(srv_session)

    *service = cmdbuf[3];
    return res;
}

Result SRV_UnregisterService(const char *service_name, u32 service_name_length)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(ID_SRV_UnregisterService, 3, 0);
    cmdbuf[1] = *((u32 *)service_name);
    cmdbuf[2] = *((u32 *)(service_name + 4));
    cmdbuf[3] = service_name_length;

    BASIC_RET(srv_session)
}

Result SRV_GetServiceHandle(Handle *service, const char *service_name, u32 service_name_length, u32 flags)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(ID_SRV_GetServiceHandle, 4, 0);
    cmdbuf[1] = *((u32 *)service_name);
    cmdbuf[2] = *((u32 *)(service_name + 4));
    cmdbuf[3] = service_name_length;
    cmdbuf[4] = flags;

    CHECK_RET(srv_session)

    *service = cmdbuf[3];
    return res;
}

Result SRV_RegisterPort(const char *port_name, u32 port_name_length, Handle client_port)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(ID_SRV_RegisterPort, 3, 2);
    cmdbuf[1] = *((u32 *)port_name);
    cmdbuf[2] = *((u32 *)(port_name + 4));
    cmdbuf[3] = port_name_length;
    cmdbuf[4] = IPC_Desc_SharedHandles(1);
    cmdbuf[5] = client_port;

    BASIC_RET(srv_session)
}

Result SRV_UnregisterPort(const char *port_name, u32 port_name_length)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(ID_SRV_UnregisterPort, 3, 0);
    cmdbuf[1] = *((u32 *)port_name);
    cmdbuf[2] = *((u32 *)(port_name + 4));
    cmdbuf[3] = port_name_length;

    BASIC_RET(srv_session)
}

Result SRV_ReceiveNotification(u32 *notification_id)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(ID_SRV_ReceiveNotification, 0, 0);

    CHECK_RET(srv_session)

    *notification_id = cmdbuf[2];
    return res;
}

Result SRV_PublishToSubscriber(u32 notification_id, u32 flags)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = IPC_MakeHeader(ID_SRV_PublishToSubscriber, 2, 0);
	cmdbuf[1] = notification_id;
	cmdbuf[2] = flags;
	
	BASIC_RET(srv_session)
}