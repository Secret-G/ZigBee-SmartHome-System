#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"


/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"

//#include "MT_UART.h"
#include "string.h"

#include "my_uart.h"
#include "hal_uart.h"
#include "stdio.h"

const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_FLASH_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

endPointDesc_t SampleApp_epDesc;

uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

uint8 SampleApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_Flash_DstAddr;
aps_Group_t SampleApp_Group;

uint8 SampleAppPeriodicCounter = 0;
uint8 SampleAppFlashCounter = 0;

extern uint8 uartBuf[32];
extern uint8 uartLen;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );


void SampleApp_Init( uint8 task_id )
{
  SampleApp_TaskID = task_id;
  zgDeviceLogicalType = ZG_DEVICETYPE_ENDDEVICE;
  SampleApp_NwkState = DEV_INIT;
  SampleApp_TransID = 0;
  
  //MT_UartInit();
  UART_Init();
  HalUARTWrite(0,"uart0 is ok\r\n",strlen("uart0 is ok\r\n"));


  // Setup for the periodic message's destination address
  // Broadcast to everyone
  SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
  SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;

  
  // Setup for the flash command's destination address - Group 1
  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Flash_DstAddr.addr.shortAddr = 0x0000;

  // Fill out the endpoint description.
  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_epDesc.task_id = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );

}


uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        // Received when a messages is received (OTA) for this endpoint
        case AF_INCOMING_MSG_CMD:
          SampleApp_MessageMSGCB( MSGpkt );
          break;

        // Received whenever the device changes state in the network
        case ZDO_STATE_CHANGE:
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next - if one is available
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  
 if(events & UART_RX_EVT)
{
    if(SampleApp_NwkState == DEV_END_DEVICE)
    {
        afStatus_t status;

        status = AF_DataRequest(&SampleApp_Flash_DstAddr,
                                &SampleApp_epDesc,
                                SAMPLEAPP_FLASH_CLUSTERID,
                                9,                // 固定9字节
                                uartBuf,
                                &SampleApp_TransID,
                                AF_DISCV_ROUTE,
                                AF_DEFAULT_RADIUS);

        if(status == afStatus_SUCCESS)
        {
            HalUARTWrite(0, "send ok\r\n", 9);
        }
        else
        {
            HalUARTWrite(0, "send fail\r\n", 11);
        }
    }

    uartLen = 0;   // 发送完成后清零

    return (events ^ UART_RX_EVT);
}
  return 0;
}


// 修改SampleApp_MessageMSGCB：仅校验下行8字节，不限制集群ID（确保接收）
void SampleApp_MessageMSGCB(afIncomingMSGPacket_t *pkt)
{
    uint8 *data = pkt->cmd.Data;
    uint8 len = pkt->cmd.DataLength;

    // 仅处理下行8字节帧（协调器→终端）
    if (len == 8)
    {
        // 1. 帧头帧尾校验
        if (data[0] == 0xAA && data[8-1] == 0x55)
        {
            // 2. 和校验（data[1]+data[2]+data[3]+data[4]+data[5] == data[6]）
            uint8 sum = 0;
            for(uint8 i = 1; i <= 5; i++)
            {
                sum += data[i];
            }
            
            if (sum == data[6])
            {
                // 校验通过，输出数据
                HalUARTWrite(0, data, 8);
            }
            else
            {
                HalUARTWrite(0, "sum err\r\n", 8); // 调试：和校验失败
            }
        }
        else
        {
            HalUARTWrite(0, "head/tail err\r\n", 13); // 调试：帧头帧尾错误
        }
    }
    // 调试：打印原始接收数据，确认是否收到但校验失败
    else
    {
        HalUARTWrite(0, "len err: ", 8);
    }
}



