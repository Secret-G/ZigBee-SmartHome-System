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
#include "beep.h"


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

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SampleApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
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


uint8 g_alarmFlag = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void Send_Data_To_EndDevice(uint16 dstAddr, uint8 *data, uint8 dataLength);

void SampleApp_Init( uint8 task_id )
{
  SampleApp_TaskID = task_id;
  zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
  SampleApp_NwkState = DEV_INIT;
  SampleApp_TransID = 0;
  
  //MT_UartInit();
  UART_Init();
  HalUARTWrite(0,"uart0 is ok\r\n",strlen("uart0 is ok\r\n"));

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

  BEEP_Init();
}

static uint8 state = 0;
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
          if ( (SampleApp_NwkState == DEV_ZB_COORD)
              || (SampleApp_NwkState == DEV_ROUTER)
              || (SampleApp_NwkState == DEV_END_DEVICE) )
          {
            // Start sending the periodic message in a regular interval.
            osal_start_timerEx( SampleApp_TaskID,
                              SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                              SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
            osal_start_timerEx(SampleApp_TaskID, BEEP_EVT, 200);

          }
          else
          {
            // Device is no longer in the network
          }
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

  if (events & BEEP_EVT)
  {


    if (g_alarmFlag)
    {
        state = !state;

        if(state)
            BEEP_On();
        else
            BEEP_Off();
    }
    else
    {
        BEEP_Off();
    }

    osal_start_timerEx(SampleApp_TaskID, BEEP_EVT, 200);

     return (events ^ BEEP_EVT);
  }
    if (events & UART_RX_EVT)
    {
        // 确保数据长度为 8 字节
        if (uartLen == 8)
        {
            Send_Data_To_EndDevice(0XFFFF, uartBuf, DOWNLINK_FRAME_LEN); 
        }

        uartLen = 0;  // 清空缓冲区
        return (events ^ UART_RX_EVT);
    }
  // Discard unknown events
  return 0;
}



void SampleApp_MessageMSGCB(afIncomingMSGPacket_t *pkt)
{
    HalUARTWrite(0, pkt->cmd.Data, pkt->cmd.DataLength);
}

// 修改Send_Data_To_EndDevice函数，确保下行8字节、集群ID与终端匹配
void Send_Data_To_EndDevice(uint16 dstAddr, uint8 *data, uint8 dataLength)
{
    // 强制下行长度为8字节（避免传错）
    if(dataLength != DOWNLINK_FRAME_LEN)
    {
        HalUARTWrite(0, "downlink len err\r\n", 17);
        return;
    }

    // 改用终端侧也处理的SAMPLEAPP_FLASH_CLUSTERID(2)，确保集群ID匹配
    afStatus_t status = AF_DataRequest(&SampleApp_Periodic_DstAddr,
                                       &SampleApp_epDesc,
                                       SAMPLEAPP_FLASH_CLUSTERID,  // 统一为2
                                       dataLength,
                                       data,
                                       &SampleApp_TransID,
                                       AF_DISCV_ROUTE,
                                       AF_DEFAULT_RADIUS);

    if (status == afStatus_SUCCESS)
    {
        HalUARTWrite(0, "send ok\r\n", 9);
    }
    else
    {
        HalUARTWrite(0, "send fail\r\n", 11);
        // 打印错误码，定位协议栈发送失败原因
        /*char err[4];
        osal_int_to_str(err, status);
        HalUARTWrite(0, "err code: ", 9);
        HalUARTWrite(0, err, strlen(err));
        HalUARTWrite(0, "\r\n", 2);*/
    }
}
