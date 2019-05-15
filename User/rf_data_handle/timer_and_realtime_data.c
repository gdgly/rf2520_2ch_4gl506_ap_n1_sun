#include "rf_data_handle.h"
#include "string.h"
#include "typedef_struct.h"
#include "debug.h"
#include "string.h"
#include "ap_param.h"
#include "timer_and_realtime_data.h"
#include "gprs_comm.h"
#include "rf_data_handle.h"
#include "stdlib.h"





int realtime_queue_write[2] = {0,0};               //дƫ��
int realtime_queue_read[2] = {0,0};								//��ƫ��

uint8_t realtime_data_queue[2][1024*32] __attribute__((at(0x10000000)));

uint8_t rtdata_send_buff[2][1450];   //���ݷ���buff

uint8_t rtdata_send_buff_count[2] = {0,0}; //buff�������������
uint32_t rtdata_resend_times[2] = {0,0};   //�ط����� 
uint32_t rtdata_send_time[2] = {0,0};      //��¼���͵�ʱ��  ���������ط����

RTC_TimeTypeDef rtc_time;
RTC_DateTypeDef rtc_date;

struct_qianfang_realtime_data qianfang_realtime_data;   //ǧ��ʵʱ����
struct_dezhou_realtime_data dezhou_realtime_data;   //����ʵʱ����

struct_sensor_data_timer *p_sensor_data_timer; //sensor ��ʱ����������ָ��

struct_ap_stat_timer ap_stat_timer;

struct_rp_stat_timer rp_stat_timer;


extern struct_gprs_4g_task gprs_4g_task[CLIENT_NUM];
extern uint8_t * get_4g_data_buff(int8_t client,uint16_t len);
extern uint8_t * get_4g_data_buff_max(int8_t client,uint16_t *len);
extern int32_t add_4g_data_len(int8_t client,int16_t len);
extern unsigned short crc16(unsigned short crc, unsigned char const *buffer,int len);
extern RTC_HandleTypeDef hrtc;
extern struct_systerm_info systerm_info;
extern struct_gprs_stat gprs_stat;
extern int8_t who_dtu_ack_r ;
extern struct_lane_to_sensor_info_and_result lane_to_sensor_info_and_result; 
extern RTC_HandleTypeDef hrtc;
extern char gprs_debug_buff[];
extern int32_t adc_getvalue_enable;


uint16_t qianfang_lane_to_his_lane(uint8_t lane);
uint8_t dz_car_len_adjust_car_zhou(uint8_t zhou1, uint8_t zhou2, uint8_t carlen);

int32_t dz_dorp_near_lane_error(uint8_t lane);


/*
 * ���ܣ���������д��һ���ֽ�
 * ʧ�ܣ�����-1
 * �ɹ�������0
*/
int32_t realtime_data_write_queue(int32_t whitch_client,uint8_t type,void *pdata)
{
	uint16_t queue_len;
	uint16_t data_len;

	if(whitch_client < 0 || whitch_client > 1)
		return -2;   //�޴�����
		
	if(type == QUEUE_QF)
	{
		queue_len = REALTIME_QUEUE_LENGH_QF;
		data_len = sizeof(struct_qianfang_realtime_data);
	}
	else if(type == QUEUE_DZ)
	{
		queue_len = REALTIME_QUEUE_LENGH_DZ;
		data_len = sizeof(struct_dezhou_realtime_data);
	}
	
	if((realtime_queue_write[whitch_client]+1)%queue_len==realtime_queue_read[whitch_client])
		return -1;
	memcpy(&realtime_data_queue[whitch_client][realtime_queue_write[whitch_client]*data_len],pdata,data_len);
	realtime_queue_write[whitch_client] = (realtime_queue_write[whitch_client]+1)%queue_len;
	return 0;
}

/*
 * ���ܣ��Ӷ����ж�ȡһ���ֽ�
 * ʧ�ܣ�����-1
 * �ɹ�������0
*/
int32_t realtime_data_read_queue(int32_t whitch_client,uint8_t type,void *pdata)
{
	uint16_t queue_len;
	uint16_t data_len;
	
	if(type == QUEUE_QF)
	{
		queue_len = REALTIME_QUEUE_LENGH_QF;
		data_len = sizeof(struct_qianfang_realtime_data);
	}
	else if(type == QUEUE_DZ)
	{
		queue_len = REALTIME_QUEUE_LENGH_DZ;
		data_len = sizeof(struct_dezhou_realtime_data);
	}
	
	if(realtime_queue_write[whitch_client]==realtime_queue_read[whitch_client])
		return -1;	
	
	memcpy(pdata,&realtime_data_queue[whitch_client][realtime_queue_read[whitch_client]*data_len],data_len);
	realtime_queue_read[whitch_client] = (realtime_queue_read[whitch_client]+1)%queue_len;
	return 0;
}


/***********************************************************
Func: ��ʼ��queue Ϊ��

param: whitch_client ָ���Ǹ�tcp����

return:
*************************************************************/
int32_t init_realtime_data_queue(int32_t whitch_client)
{ 
	if(whitch_client < 0 || whitch_client > 1)
		return -2;   //�޴�����
		
	realtime_queue_write[whitch_client] = realtime_queue_read[whitch_client] = 0;  //���32KB��ʵʱ���ݻ���
	rtdata_send_buff_count[whitch_client] = 0;  //������ͻ���
	rtdata_resend_times[whitch_client] = 0;     //����ش�����
}


/***********************************************************
Func: ��ȡʵʱ���ݻ�����������ݳ���  ���������ǲ��Ǹ������������� ���߼������ŵȴ���ɴ������

param: whitch_client ָ���Ǹ�tcp����

return:
*************************************************************/
uint32_t get_realtime_data_queue_hasdata_len(int32_t whitch_client)
{
	uint16_t queue_len;
	uint16_t data_len;
	int16_t queue_num;
	
	if(whitch_client < 0 || whitch_client > 1)
		return -2;   //�޴�����

	if(sys_flash_param.global_cfg_param.realtime_data_type[whitch_client] == QUEUE_QF)
	{
		queue_len = REALTIME_QUEUE_LENGH_QF;
		data_len = sizeof(struct_qianfang_realtime_data);
	}
	else if(sys_flash_param.global_cfg_param.realtime_data_type[whitch_client] == QUEUE_DZ)
	{
		queue_len = REALTIME_QUEUE_LENGH_DZ;
		data_len = sizeof(struct_dezhou_realtime_data);
	}
	
	queue_num = realtime_queue_write[whitch_client] - realtime_queue_read[whitch_client];  //���32KB��ʵʱ���ݻ���
	if(queue_num < 0)
		queue_num += queue_len;
	
	return queue_num*data_len;
}



/***********************************************************
Func: ������ѵʵʱ����queue ���ݷ���buff�Ĵ�С  ��ɺ��ʵİ� ���ͣ���ʱ��������

param: whitch_client ָ���Ǹ�tcp����

return:
*************************************************************/
int32_t send_to_4g_realtime_data(int32_t whitch_client)
{
	uint8_t *pdata_buff;
	uint16_t size = 0;
	uint16_t buff_len = 0;
	uint16_t data_count = 0;
	uint16_t i = 0;
	static uint32_t timeout[2] = {0,0};
	
	if(whitch_client < 0 || whitch_client > 1)
		return -2;   //�޴�����

	
	if(rtdata_send_buff_count[whitch_client] !=0)   //buff�������� ˵��������ط���
	{	
		if(systerm_info.slot - rtdata_send_time[whitch_client] < (TO_4G_RE_SEND_SEC*512))  //��ʱʱ�䲻�� ���ܷ���
			return -3;
		
		if(rtdata_send_buff_count[whitch_client] > QF_ARRAY_MAX_COUNT)  //�����쳣  ֱ�����㻺����
		{
			rtdata_send_buff_count[whitch_client] = 0;	
			return -2;
		}
		else if(sys_flash_param.global_cfg_param.realtime_data_type[whitch_client] == RDT_QF) //ǧ�������ݳ���
		{
			size = rtdata_send_buff_count[whitch_client] * sizeof(struct_qianfang_realtime_data);
		}
		else if(sys_flash_param.global_cfg_param.realtime_data_type[whitch_client] == RDT_DZ)  //���ݵ����ݳ���
		{
			size = rtdata_send_buff_count[whitch_client] * sizeof(struct_dezhou_realtime_data);
		}		
		if(size > 0)   //������
		{
			pdata_buff = get_4g_data_buff(whitch_client,size);
			if(pdata_buff == (uint8_t *)0 || pdata_buff == (uint8_t *)0xffffffff)
				return -1;
			
			memcpy(pdata_buff,&rtdata_send_buff[whitch_client],size);
			add_4g_data_len(whitch_client,size);
			rtdata_send_time[whitch_client] = systerm_info.slot;		
			rtdata_resend_times[whitch_client]++;
			if(rtdata_resend_times[whitch_client] > 100)   //ͬһ�����ݷ��ͳ���100�� �����ð�����   �п��ܷ������޷������ð����� ����һֱ����ACK
			{
				rtdata_resend_times[whitch_client] = 0;
				rtdata_send_buff_count[whitch_client] = 0;	
			}
			return 0;
		}
	}
	else  //������
	{
		
		if(((i = systerm_info.slot - timeout[whitch_client]) < 5*512) && ((buff_len = get_realtime_data_queue_hasdata_len(whitch_client)) < 1300))
			return -1;
		
		timeout[whitch_client] = systerm_info.slot;
		
		pdata_buff = get_4g_data_buff_max(whitch_client,&buff_len);   //��ȡ��󻺳�
		if(pdata_buff == (uint8_t *)0 || pdata_buff == (uint8_t *)0xffffffff)
			return -1;

		if(sys_flash_param.global_cfg_param.realtime_data_type[whitch_client] == RDT_QF) //ǧ�������ݳ���
		{
			data_count = buff_len/sizeof(struct_qianfang_realtime_data);
			for(i=0;i<data_count;i++)
			{
				if(realtime_data_read_queue(whitch_client,RDT_QF,&rtdata_send_buff[whitch_client][0]+sizeof(struct_qianfang_realtime_data)*i) == 0) //���ɹ�
				{
					rtdata_send_buff_count[whitch_client]++;
				}
				else
					break;
			}
			size = rtdata_send_buff_count[whitch_client] * sizeof(struct_qianfang_realtime_data);
		}
		else if(sys_flash_param.global_cfg_param.realtime_data_type[whitch_client] == RDT_DZ)  //���ݵ����ݳ���
		{
			data_count = buff_len/sizeof(struct_dezhou_realtime_data);
			for(i=0;i<data_count;i++)
			{
				if(realtime_data_read_queue(whitch_client,RDT_DZ,&rtdata_send_buff[whitch_client][0]+sizeof(struct_dezhou_realtime_data)*i) == 0) //���ɹ�
				{
					rtdata_send_buff_count[whitch_client]++;
				}
				else
					break;
			}			
			size = rtdata_send_buff_count[whitch_client] * sizeof(struct_dezhou_realtime_data);
		}				
		if(size > 0)   //������
		{			
			memcpy(pdata_buff,&rtdata_send_buff[whitch_client],size);
			add_4g_data_len(whitch_client,size);
			rtdata_send_time[whitch_client] = systerm_info.slot;		
			return 0;
		}
		
	}
	
}

/****************************************************************
Func:  �ú������ַ��������������ã����ǧ��Э�飬���շ������·���ʵʱ����ack����Ӧ������ɾ��
param: seq �����  
			 sec_or_lane ������

return: ����0��ʾack�������  ��0��ʾ�쳣

*****************************************************************/
int32_t dtu_ack_r_fun(uint32_t seq,uint32_t sec_or_lane)
{
	struct_qianfang_realtime_data *p_qf;
	struct_dezhou_realtime_data *p_dz;
	uint32_t i;
	
	if(who_dtu_ack_r < 0 || who_dtu_ack_r > 1)
		return -2;   //�޴�����	
	
	if(sys_flash_param.global_cfg_param.realtime_data_type[who_dtu_ack_r] == RDT_QF) //��ǰ��������ǧ��������Э��
	{
		if((sec_or_lane & 0xff00) == 0xfe00)  //״̬�� ack ��ʱ
		{
			gprs_4g_task[who_dtu_ack_r].timer_stat_qianfang_task = 0;
			return 0;
		}
		if(rtdata_send_buff_count[who_dtu_ack_r]>0)
		{
			p_qf = (struct_qianfang_realtime_data *)rtdata_send_buff[who_dtu_ack_r];
			if(rtdata_send_buff_count[who_dtu_ack_r] > QF_ARRAY_MAX_COUNT)  //�����쳣  ֱ�����㻺����
			{
				rtdata_send_buff_count[who_dtu_ack_r] = 0;	
				return -2;
			}			
			for(i=0;i<rtdata_send_buff_count[who_dtu_ack_r];i++)
			{
				if(p_qf[i].serial_number1 == seq)
				{
					p_qf[i] = p_qf[rtdata_send_buff_count[who_dtu_ack_r]-1];
					rtdata_send_buff_count[who_dtu_ack_r]--;
					return 0;
				}
			}
		}
	}
	else if(sys_flash_param.global_cfg_param.realtime_data_type[who_dtu_ack_r] == RDT_DZ)  //���ݵ����ݳ���
	{

	}		
	
	
	
	
}
/**************************************************
Func:
		��ѵ������������ѵ����2��tcp������ʵʱ����


***************************************************/
void poll_to_4g_realtime_data()
{
	uint32_t i = 0;
	
	for(i=0;i<2;i++)
	{
		if(gprs_stat.con_client[i].connect_ok)
		{
			send_to_4g_realtime_data(i);
		}
	}
}



uint8_t Is_Leap_Year(uint16_t year)
{  
	if(year%4==0)
	{  
		return 1;
	}
	else return 0;
}  

const uint8_t mon_table[12]={31,28,31,30,31,30,31,31,30,31,30,31};     


/**************************************************
Func:
		��ȡRTCʱ�� �����ض�1970�������


***************************************************/
uint32_t get_sec_from_rtc()
{
	uint16_t t;
	uint32_t seccount=0;
	uint16_t year;
	uint8_t month;
	uint8_t day;
	
	HAL_RTC_GetTime(&hrtc,&rtc_time,RTC_FORMAT_BIN);	
	HAL_RTC_GetDate(&hrtc,&rtc_date,RTC_FORMAT_BIN);
	
	
	year = rtc_date.Year + 2000;
	
	if(year<1970||year>2099)              
		return 1;   
	
	for(t=1970;t<year;t++)                                     
	{
		if(Is_Leap_Year(t))
			seccount+=31622400;                     
		else 
			seccount+=31536000;                                  
	}
	month = rtc_date.Month-1; 
	day = rtc_date.Date-1;
	for(t=0;t<month;t++)                                               
	{
		seccount+=(uint32_t)mon_table[t]*86400;                                
		if(Is_Leap_Year(year)&&t==1)
			seccount+=86400;                     
	}
	seccount+=(uint32_t)(day-1)*86400;                           
	seccount+=(uint32_t)rtc_time.Hours*3600;                                        
	seccount+=(uint32_t)rtc_time.Minutes*60;                                       
	seccount+=rtc_time.Seconds;                                              
																						
	return seccount;    
}






/**************************************************
Func:
		��ʵʱ���ݰ��ն�Ӧ��Э�������Ž�����queue
			��ʵʱ���ݽ�����ɺ���ã�ָ����Ӧ�ĳ��� 
		�ú�������Ӧ���б��л�ȡ��Ӧ�Ĳ���

param: lane ������


***************************************************/
void write_realtime_data_to_queue(uint8_t lane)
{
	uint32_t i;
	uint8_t car_type = 0;
	
	for(i=0;i<2;i++)
	{
		if(sys_flash_param.global_cfg_param.realtime_data_switch[i] > 0)//ʵʱ���ݿ����Ѿ���
		{
			if(sys_flash_param.global_cfg_param.realtime_data_type[i] == RDT_QF) //ǧ��������Э��
			{
				HAL_RTC_GetTime(&hrtc,&rtc_time,RTC_FORMAT_BIN);				
				HAL_RTC_GetDate(&hrtc,&rtc_date,RTC_FORMAT_BIN);


				qianfang_realtime_data.head = 0xaaaa5555;
				qianfang_realtime_data.n1_id = sys_flash_param.ap_param.ap_id;
				qianfang_realtime_data.len = sizeof(qianfang_realtime_data);
				qianfang_realtime_data.cmd = 2;
				qianfang_realtime_data.serial_number++;
				qianfang_realtime_data.serial_number1++;
				qianfang_realtime_data.lane_number = qianfang_lane_to_his_lane(lane);
				qianfang_realtime_data.year = rtc_date.Year;
				qianfang_realtime_data.month = rtc_date.Month;
				qianfang_realtime_data.day = rtc_date.Date;
				qianfang_realtime_data.hour = rtc_time.Hours;
				qianfang_realtime_data.min = rtc_time.Minutes;
				qianfang_realtime_data.sec = rtc_time.Seconds;
				qianfang_realtime_data.speed = lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.speed*36/10000;
				//����0	
				if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length<sys_flash_param.global_cfg_param.m_arDelimiter[0])
					car_type = 1;
				//����1
				
				if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[0]
					&& lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length < sys_flash_param.global_cfg_param.m_arDelimiter[1])
					car_type = 2;
				//����2
				if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[1]
					&& lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length < sys_flash_param.global_cfg_param.m_arDelimiter[2])
					car_type = 3;
				//����3
				if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[2]
					&& lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length < sys_flash_param.global_cfg_param.m_arDelimiter[3])
					car_type = 4;
				//����4
				if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[3])
					car_type = 5;				
				qianfang_realtime_data.cartype = car_type;
				qianfang_realtime_data.car_len = lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length/100;
				qianfang_realtime_data.car_head_distance_sec = lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_head_time_distance/1000;
				qianfang_realtime_data.car_car_distance = lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_car_time_distance/1000;
				qianfang_realtime_data.car_on_time = lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.on_to_off_timeslot;
				realtime_data_write_queue(i,QUEUE_QF,&qianfang_realtime_data);
			}
			else if(sys_flash_param.global_cfg_param.realtime_data_type[i] == RDT_DZ) //���ݵ����ݳ���
			{	
				if(dz_dorp_near_lane_error(lane) != 0)
					return;
				dezhou_realtime_data.head = 0x584d5555;
				dezhou_realtime_data.ap_id = sys_flash_param.ap_param.ap_id;
				dezhou_realtime_data.seq++;
				dezhou_realtime_data.len = sizeof(dezhou_realtime_data) - 12;
				dezhou_realtime_data.sec = get_sec_from_rtc();
				dezhou_realtime_data.lane_number = lane+1;
				//����0	
				if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length<sys_flash_param.global_cfg_param.m_arDelimiter[0])
					car_type = 1;
				//����1
				if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[0]
					&& lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length < sys_flash_param.global_cfg_param.m_arDelimiter[1])
					car_type = 2;
				//����2
				if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[1]
					&& lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length < sys_flash_param.global_cfg_param.m_arDelimiter[2])
					car_type = 3;
				//����3
				if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[2]
					&& lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length < sys_flash_param.global_cfg_param.m_arDelimiter[3])
					car_type = 4;
				//����4
				if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[3])
					car_type = 5;							
				dezhou_realtime_data.car_type = car_type;
				dezhou_realtime_data.car_len = lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_length/100;
				dezhou_realtime_data.car_len = dezhou_realtime_data.car_len*10/9;
				if(dezhou_realtime_data.car_len>170)
					dezhou_realtime_data.car_len = 170;
				if(dezhou_realtime_data.car_len<30 && dezhou_realtime_data.car_len!=0)
					dezhou_realtime_data.car_len = 30;				
				dezhou_realtime_data.car_speed = lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.speed*36/10000;
				dezhou_realtime_data.car_zhou = dz_car_len_adjust_car_zhou(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_zhou,
					lane_to_sensor_info_and_result.lane_and_sensor[lane].before.sensor_event.car_zhou,dezhou_realtime_data.car_len);									
				dezhou_realtime_data.crc = crc16(0,(uint8_t *)&dezhou_realtime_data.ap_id,dezhou_realtime_data.len+6);				
				realtime_data_write_queue(i,QUEUE_DZ,&dezhou_realtime_data);
			}
		}
	}
	
}


 
/**************************************************
Func:
			//��ʱ���㳵������Ϣ��sensor״̬��Ϣ


***************************************************/
void sensor_data_and_stat_timer_task()
{
	uint32_t i;
	static uint32_t time_slot = 0;
	static uint32_t sec_last = 0;
	uint32_t sec;
	
	if(systerm_info.slot - time_slot < 250 )
		return;
	
	time_slot = systerm_info.slot;

	HAL_RTC_GetTime(&hrtc,&rtc_time,RTC_FORMAT_BIN);	
	HAL_RTC_GetDate(&hrtc,&rtc_date,RTC_FORMAT_BIN);
	
	if(rtc_time.Hours == sys_flash_param.global_cfg_param.dev_reset_hour && 
		rtc_time.Minutes == sys_flash_param.global_cfg_param.dev_reset_min && 
		sys_flash_param.global_cfg_param.dev_reset_switch > 0              &&
		systerm_info.slot > 100000)
		HAL_NVIC_SystemReset();
	
	sec = rtc_time.Minutes*60 + rtc_time.Seconds;
	
//	sprintf(gprs_debug_buff,"rtc: %d:%d:%d\r\n",rtc_time.Hours,rtc_time.Minutes,rtc_time.Seconds);
//	copy_string_to_double_buff(gprs_debug_buff);	
	if(sec%sys_flash_param.global_cfg_param.data_save_timer_time == 0 && sec_last != sec)
	{
		sec_last = sec;
		adc_getvalue_enable = 1;
		make_timer_statistics_data(sys_flash_param.global_cfg_param.data_save_timer_time*1000);
		
		for(i=0;i<2;i++)
		{
			if(sys_flash_param.global_cfg_param.realtime_data_switch[i] > 0)
			{
				if(sys_flash_param.global_cfg_param.realtime_data_type[i] == RDT_QF) //ǧ�������ݳ���
				{
					gprs_4g_task[i].timer_stat_qianfang_task++;
				}
			}
			if(sys_flash_param.global_cfg_param.timer_data_switch[i] > 0)
			{
				gprs_4g_task[i].timer_data_task++;
			}
			if(sys_flash_param.global_cfg_param.timer_stat_switch[i] > 0)
			{
				gprs_4g_task[i].timer_stat_task++;
			}
		}
	}
	
}

/**************************************************
Func:
		ǧ��Э����Ҫ���㳵�� ���ݶ�����������Ϣ


***************************************************/
uint16_t qianfang_lane_to_his_lane(uint8_t lane)
{
	uint32_t lane_direction;
	uint32_t lane_count = 0;
	int32_t index;
	
	if(lane >= LANE_SENSOR_MAX_COUNT)
		return 0;

	lane_direction = lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_cfg.lane_direction;
	for(index=lane;index>=0;index--)
	{	
	
		if(lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_cfg.lane_direction == lane_direction)
		{	
			lane_count++;
		}	
	}
	if(lane_direction == 0 || lane_direction == 3)
		return lane_count+100;
	else if(lane_direction == 1 || lane_direction == 2)
		return lane_count+200;	
}






uint8_t dz_car_len_adjust_car_zhou(uint8_t zhou1, uint8_t zhou2, uint8_t carlen)
{
	uint8_t zhou;
	
	
	if(carlen > 160)  //����16�׵ĳ�  ���sensor�ж��� >5�� ѡsensor�д��  ����У׼Ϊ6��
	{
		if(zhou1>=5 || zhou2>=5)
			return zhou1>zhou2?zhou1:zhou2;
		else
			return 6;
	}
	else if(carlen<160 && carlen>120)
	{
		zhou = zhou1>zhou2?zhou1:zhou2;
		if(zhou > 6)
			zhou = 6;
		if(zhou <= 2)
			zhou = 2;		
		return zhou;
	}
	else if(carlen<120 && carlen>90)
	{
		zhou = zhou1>zhou2?zhou1:zhou2;
		if(zhou > 3)
			zhou = 3;
		if(zhou <= 2)
			zhou = 2;			
		return zhou;
	}	
	else
		return 2;
}


int32_t dz_dorp_near_lane_error(uint8_t lane)
{
	int32_t ms_before_1,ms_before_2;
	int32_t ms_after_1,ms_after_2;	
	
	if(lane>0)//����г���
	{
		//������������һ��
		if(lane_to_sensor_info_and_result.lane_and_sensor[lane-1].after.sensor_cfg.lane_direction == lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_cfg.lane_direction)
		{
			ms_before_1 = lane_to_sensor_info_and_result.lane_and_sensor[lane].before.sensor_event.car_onoff_event[0].bmMs;
			ms_before_2 = lane_to_sensor_info_and_result.lane_and_sensor[lane-1].before.sensor_event.car_onoff_event[0].bmMs;
			
			ms_after_1 = lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_onoff_event[0].bmMs;
			ms_after_2 = lane_to_sensor_info_and_result.lane_and_sensor[lane-1].after.sensor_event.car_onoff_event[0].bmMs;

			if(lane_to_sensor_info_and_result.lane_and_sensor[lane].before.sensor_event.car_onoff_event[0].bmSec == lane_to_sensor_info_and_result.lane_and_sensor[lane-1].before.sensor_event.car_onoff_event[0].bmSec && 
					lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_onoff_event[0].bmSec == lane_to_sensor_info_and_result.lane_and_sensor[lane-1].after.sensor_event.car_onoff_event[0].bmSec)
			{
				if(abs(ms_before_1-ms_before_2)<=128 && abs(ms_after_1-ms_after_2)<=128)
				{
					if(ms_before_1>ms_before_2)
						return -1;
				}
			}
		}	
	}
	if(lane<LANE_SENSOR_MAX_COUNT-2)//����г���
	{
		//������������һ��
		if(lane_to_sensor_info_and_result.lane_and_sensor[lane+1].after.sensor_cfg.lane_direction == lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_cfg.lane_direction)
		{
			ms_before_1 = lane_to_sensor_info_and_result.lane_and_sensor[lane].before.sensor_event.car_onoff_event[0].bmMs;
			ms_before_2 = lane_to_sensor_info_and_result.lane_and_sensor[lane+1].before.sensor_event.car_onoff_event[0].bmMs;
			
			ms_after_1 = lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_onoff_event[0].bmMs;
			ms_after_2 = lane_to_sensor_info_and_result.lane_and_sensor[lane+1].after.sensor_event.car_onoff_event[0].bmMs;

			if(lane_to_sensor_info_and_result.lane_and_sensor[lane].before.sensor_event.car_onoff_event[0].bmSec == lane_to_sensor_info_and_result.lane_and_sensor[lane+1].before.sensor_event.car_onoff_event[0].bmSec && 
					lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_event.car_onoff_event[0].bmSec == lane_to_sensor_info_and_result.lane_and_sensor[lane+1].after.sensor_event.car_onoff_event[0].bmSec)
			{
				if(abs(ms_before_1-ms_before_2)<=128 && abs(ms_after_1-ms_after_2)<=128)
				{
					if(ms_before_1>ms_before_2)
						return -1;
				}
			}
		}	
	}
	return 0;
}





