#include "rf_data_handle.h"
#include "string.h"
#include "typedef_struct.h"
#include "debug.h"
#include "string.h"
#include "ap_param.h"
#include "timer_and_realtime_data.h"



int32_t print_event_handle_guocheng = 0;     //��ӡON OFF������ϸ����

int32_t print_one_car =  0;   //��ӡʵʱ�������



extern uint8_t get_slot_num();
extern struct_systerm_info systerm_info;   //ϵͳʱ��


struct_lane_to_sensor_info_and_result lane_to_sensor_info_and_result;  //���г������� �� RP S ���ݰ����� ���� ����

char event_handle_debug_buff[1024];

char list_sensor_debug_buff[ROW_MAX][LINE_MAX];

extern struct_sys_flash_param sys_flash_param ;


/*
	���� sensor ��lane_to_sensor_info_and_result�е�������


*/
uint32_t find_sensor_index_from_lane_to_xx(uint16_t sensor_id)
{
	uint32_t index = 0;
	struct{
		uint32_t index;
		uint32_t time_slot;
	}it;
	
	
	//�����г�����sensor�б�
	for(index=0;index<LANE_SENSOR_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_id == sensor_id ||
			lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_id == sensor_id)
		{
			return index;
		}
	}

	//�����޳�����sensor�б��ж�Ӧ��sensorλ��
	for(index=0;index<NOLANE_SENSOR_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id == sensor_id)
		{
			return (index + LANE_SENSOR_MAX_COUNT);
		}
	}	
	
	it.time_slot = 0xffffffff;
	//�����޳�����sensor�б� ��λ�� ���� ����һ��������ʱ�����λ��
	for(index=0;index<NOLANE_SENSOR_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id == 0)
		{
			return (index + LANE_SENSOR_MAX_COUNT);
		}
		if(lane_to_sensor_info_and_result.no_lane_sensor[index].last_packet_time_slot < it.time_slot)
		{
			it.time_slot = lane_to_sensor_info_and_result.no_lane_sensor[index].last_packet_time_slot;
			it.index = index;
		}
	}		
	
	memset(&lane_to_sensor_info_and_result.no_lane_sensor[it.index],0,sizeof(lane_to_sensor_info_and_result.no_lane_sensor[0]));
	if(lane_to_sensor_info_and_result.no_lane_sensor_num > 0)
		lane_to_sensor_info_and_result.no_lane_sensor_num--;
	return (it.index + LANE_SENSOR_MAX_COUNT);
	
}


/*
	RF �յ� sensor event �� stat ��֮��  ���øú����� �����ظ����ݹ��˵Ȳ��� ����Ч���ݲ����Ӧ���ݽṹ��



*/
int insert_sensor_event(SNP_SEN_MODE_B_PACKET_t *ptr_s_event,int8_t rssi,uint8_t luzl,uint8_t slot)
{
	uint32_t index,i,k,j;
	
	struct_lane_and_sensor *p_ls = 0;
	int32_t event_count = 0;  //����ʵ���¼���
	int32_t calc_event_count = 0;   //�����ʵ�ʲ������¼���
	int32_t get_event_count = 0;   //Ҫȡ���¼���

	uint8_t resend_times = 0;
	char *ptr = (char *)ptr_s_event;
	uint16_t sid = ((ptr_s_event->sPhr.uiDevId>>8)&0xff)|((ptr_s_event->sPhr.uiDevId<<8)&0xff00);
	
	index = find_sensor_index_from_lane_to_xx(ptr_s_event->sPhr.uiDevId);
//	sprintf(event_handle_debug_buff,"event:%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n",
//	ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7],ptr[8],ptr[9],ptr[10],ptr[11],ptr[12],ptr[13],ptr[14],ptr[15],ptr[16],ptr[17],ptr[18],ptr[19]);
//	copy_string_to_double_buff(event_handle_debug_buff);	
	
	if(index >= LANE_SENSOR_MAX_COUNT) //�����б���û���ҵ���sensor
	{
		index -= LANE_SENSOR_MAX_COUNT;
		
		lane_to_sensor_info_and_result.no_lane_sensor[index].last_packet_time_slot = systerm_info.slot;
		
		if(lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id == 0 || lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.all_rev_event_count == 0)   //��һ���¼���
		{
			event_count = (ptr_s_event->sPhr.ucSize - SENSOR_EVENT_PACKET_HEAD_SIZE)/2;
			if(event_count<1)
				return -1;
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id = ptr_s_event->sPhr.uiDevId;
			
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.rssi = rssi;
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.luzl = luzl;
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.slot = slot;
			
			
			lane_to_sensor_info_and_result.no_lane_sensor_num++;
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.all_rev_event_count++;
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_event_packet_seq = ptr_s_event->sPhr.ucSerNr;
			
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_packet_event_count = event_count;
			memcpy(&lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_pcket_event[0],&ptr_s_event->asEvent[0],event_count*sizeof(ptr_s_event->asEvent[0]));
			for(i=0;i<event_count;i++)
			{
				if(ptr_s_event->asEvent[event_count-1-i].uiAll == 0x7fff || ptr_s_event->asEvent[event_count-1-i].uiAll == 0xffff)  //����ͳ���¼�
				{
					lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.new_7fff_flag++;
					lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.timeslot_7fff_event = systerm_info.slot;
					continue;
				}
				if(lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count < EVENT_MAX)   //������ �������һ��
				{
					lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count++;
					lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.has_event_no_handle++;
				}					
				lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event[lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count-1].event.uiAll = ptr_s_event->asEvent[event_count-1-i].uiAll;
				lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event[lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count-1].event_rev_time_slot = systerm_info.slot;
				lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event[lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count-1].event_valid = 0;
				
			}
		}
		else     //�Ѿ��й��¼���
		{
			//��ǰ�����-��һ������� 
			calc_event_count = ptr_s_event->sPhr.ucSerNr - lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_event_packet_seq;
			if(calc_event_count<0)
				calc_event_count += 256;
			else if(calc_event_count == 0) //�ظ���
				return -2;
			
			//����ʵ���м����¼���
			event_count = (ptr_s_event->sPhr.ucSize - SENSOR_EVENT_PACKET_HEAD_SIZE)/2;
			
			for(j=0;j<event_count;j++)
			{
				
				for(k=0;k<lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_packet_event_count;k++)
				{
					if(lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_pcket_event[k].uiAll == ptr_s_event->asEvent[j].uiAll)
						goto first1;
				}
				get_event_count++;
				first1:continue;
			}
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_packet_event_count = event_count;
			memcpy(&lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_pcket_event[0],&ptr_s_event->asEvent[0],event_count*sizeof(ptr_s_event->asEvent[0]));
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.rssi = rssi;
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.luzl = luzl;
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.slot = slot;
			
			resend_times = (ptr_s_event->slot>>8)&0x1f;
			if(resend_times <= lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_resend_times && lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_resend_times!=0)
			{
				if(lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_resend_times>=9)
					lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.resend[9]++;
				else
					lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.resend[lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_resend_times]++;
			}
			if(resend_times == 0)
				lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.resend[0]++;
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_resend_times = resend_times;
			

			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.all_rev_event_count ++;
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.all_lost_event_count += calc_event_count-get_event_count;
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.last_event_packet_seq = ptr_s_event->sPhr.ucSerNr;
			
			for(i=0;i<get_event_count;i++)
			{
				if(ptr_s_event->asEvent[get_event_count-1-i].uiAll == 0x7fff)  //����ͳ���¼�
				{
					lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.new_7fff_flag++;
					lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.timeslot_7fff_event = systerm_info.slot;
					continue;
				}
				if(lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count < EVENT_MAX)   //������ �������һ��
				{
					
					lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count++;		
					lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.has_event_no_handle++;		
				}	
#if PRINT_EVENT_HANDLE_ALL == 1				
				sprintf(event_handle_debug_buff,"Insert event:%04X [%d %d.%d] \r\n",sid,ptr_s_event->asEvent[get_event_count-1-i].blIsOn,ptr_s_event->asEvent[get_event_count-1-i].bmSec,ptr_s_event->asEvent[get_event_count-1-i].bmMs);
				copy_string_to_double_buff(event_handle_debug_buff);		
#endif				
				lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event[lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count-1].event.uiAll = ptr_s_event->asEvent[get_event_count-1-i].uiAll;
				lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event[lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count-1].event_rev_time_slot = systerm_info.slot;
				lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event[lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count-1].event_valid = 0;
				
			}			
			
		}
	}
	else     //sensor �������б���
	{
		if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_id == ptr_s_event->sPhr.uiDevId)  
		{
			p_ls = &lane_to_sensor_info_and_result.lane_and_sensor[index].after;
		}
		else if(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_id == ptr_s_event->sPhr.uiDevId)
		{
			p_ls = &lane_to_sensor_info_and_result.lane_and_sensor[index].before;
		}
		else
			return -3;
		
		//�������ݰ��е��¼�����
		event_count = (ptr_s_event->sPhr.ucSize - SENSOR_EVENT_PACKET_HEAD_SIZE)/2;
		if(event_count<1)
			return -1;		
		
		p_ls->last_packet_time_slot = systerm_info.slot;
		p_ls->sensor_stat.rssi = rssi;
		p_ls->sensor_stat.luzl = luzl;
		p_ls->sensor_stat.slot = slot;
		p_ls->sensor_event.car_zhou = (ptr_s_event->slot>>13) & 0x07;
		
		if(p_ls->sensor_event.all_rev_event_count == 0)   //��һ���¼���
		{
			p_ls->sensor_event.all_rev_event_count ++;
			p_ls->sensor_event.last_event_packet_seq = ptr_s_event->sPhr.ucSerNr;
			p_ls->sensor_event.last_packet_event_count = event_count;

			memcpy(&p_ls->sensor_event.last_pcket_event[0],&ptr_s_event->asEvent[0],event_count*sizeof(ptr_s_event->asEvent[0]));
				
			for(i=0;i<event_count;i++)
			{
				if(ptr_s_event->asEvent[event_count-1-i].uiAll == 0x7fff || ptr_s_event->asEvent[event_count-1-i].uiAll == 0xffff)  //����ͳ���¼�
				{
					p_ls->sensor_stat.new_7fff_flag++;
					p_ls->sensor_stat.timeslot_7fff_event = systerm_info.slot;
					continue;
				}
				if(p_ls->sensor_event.event_count < EVENT_MAX)   //������ �������һ��
				{
					p_ls->sensor_event.event_count++;	
					p_ls->sensor_event.has_event_no_handle++;
				}
				p_ls->sensor_event.event[p_ls->sensor_event.event_count-1].event.uiAll = ptr_s_event->asEvent[event_count-1-i].uiAll;
				p_ls->sensor_event.event[p_ls->sensor_event.event_count-1].event_rev_time_slot = systerm_info.slot;
				p_ls->sensor_event.event[p_ls->sensor_event.event_count-1].event_valid = 0;
							
			}
		}
		else
		{
			//��ǰ�����-��һ������� �õ�����Ӧ���м����¼���
			calc_event_count = ptr_s_event->sPhr.ucSerNr - p_ls->sensor_event.last_event_packet_seq;
			if(calc_event_count<0)
				calc_event_count += 256;
			if(calc_event_count == 0) //�ظ���
				return -2;
			
			
			resend_times = (ptr_s_event->slot>>8)&0x1f;
			if(resend_times <=p_ls->sensor_event.last_resend_times &&
												p_ls->sensor_event.last_resend_times!=0)
			{
				if(p_ls->sensor_event.last_resend_times>=9)
				{
					p_ls->sensor_event.resend[9]++;
					p_ls->sensor_stat.t_resend[9]++;
				}
				else
				{
					p_ls->sensor_event.resend[p_ls->sensor_event.last_resend_times]++;
					p_ls->sensor_stat.t_resend[p_ls->sensor_event.last_resend_times]++;
				}
			}
			if(resend_times == 0)
				p_ls->sensor_stat.t_resend[0]++;
			
			p_ls->sensor_event.last_resend_times = resend_times;
			
			//����ʵ���м����¼���
			event_count = (ptr_s_event->sPhr.ucSize - SENSOR_EVENT_PACKET_HEAD_SIZE)/2;
			for(j=0;j<event_count;j++)
			{				
				for(k=0;k<p_ls->sensor_event.last_packet_event_count;k++)
				{
					if(p_ls->sensor_event.last_pcket_event[k].uiAll == ptr_s_event->asEvent[j].uiAll)
						goto first2;
				}				
				get_event_count++;
				first2:continue;
			}		
			
			p_ls->sensor_event.last_packet_event_count = event_count;
			memcpy(&p_ls->sensor_event.last_pcket_event[0],&ptr_s_event->asEvent[0],event_count*sizeof(ptr_s_event->asEvent[0]));			

			p_ls->sensor_event.all_rev_event_count++;
			p_ls->sensor_event.last_event_packet_seq = ptr_s_event->sPhr.ucSerNr;
			p_ls->sensor_event.all_lost_event_count += (calc_event_count - get_event_count)>0?(calc_event_count - get_event_count):0;
		
			p_ls->sensor_stat.avg_rssi += (int32_t)rssi;
			p_ls->sensor_stat.timer_packet_num ++;   //Ӧ�ý��յ��İ���
			p_ls->sensor_stat.timer_lost_packet_num += (calc_event_count - get_event_count)>0?(calc_event_count - get_event_count):0;  //��¼���˶��ٰ�
			
			for(i=0;i<get_event_count;i++)
			{
				if(ptr_s_event->asEvent[get_event_count-1-i].uiAll == 0x7fff)  //����ͳ���¼�
				{
					p_ls->sensor_stat.new_7fff_flag++;
					p_ls->sensor_stat.timeslot_7fff_event = systerm_info.slot;
					continue;
				}
				if(p_ls->sensor_event.event_count < EVENT_MAX)   //������ �������һ��
				{	
					p_ls->sensor_event.event_count++;	
					p_ls->sensor_event.has_event_no_handle++;	
				}					
				p_ls->sensor_event.event[p_ls->sensor_event.event_count-1].event.uiAll = ptr_s_event->asEvent[get_event_count-1-i].uiAll;
				p_ls->sensor_event.event[p_ls->sensor_event.event_count-1].event_rev_time_slot = systerm_info.slot;
				p_ls->sensor_event.event[p_ls->sensor_event.event_count-1].event_valid = 0;
				
			}						
		}
	}
}

int32_t calc_event_time_ms(struct_event_and_info e1, struct_event_and_info e2)
{
	int32_t m_sec = (e2.event.bmSec*1024+e2.event.bmMs) - (e1.event.bmSec*1024+e1.event.bmMs);
	if(m_sec<0)
	{
		m_sec += 30*1024;
	}
	if((e2.event_rev_time_slot - e1.event_rev_time_slot) >(30*1024/2)) //ʵ���յ���ʱ����Ѿ�����30s
	{
		m_sec += (e2.event_rev_time_slot - e1.event_rev_time_slot)/(30*1024/2);
	}
	return (m_sec*1000/1024);
}

int32_t calc_sensor_event(struct_sensor_event *p_events,uint16_t sensor_id)
{
	int32_t i=0;
	int32_t calc_ms = 0;
	uint16_t sid = ((sensor_id>>8)&0xff)|((sensor_id<<8)&0xff00);
	
	for(i=0;i<p_events->event_count;i++)
	{
		if(p_events->event[i].event_valid == 0)//�ҵ�һ��δ������¼�
		{
			if(i == 0) //��һ���¼�
			{
				if(p_events->event[i].event.blIsOn == 0) //û��ON��OFF ����
				{
					memcpy(&p_events->event[0],&p_events->event[1],(p_events->event_count-1)*sizeof(p_events->event[0]));
					p_events->event_count--;
					i--;
					p_events->has_event_no_handle--;
					continue;
				}
				else  //ON �¼�
				{
					if((i+1)<p_events->event_count && p_events->event[i+1].event.blIsOn == 0)  //���滹��OFF�¼�  �Ա�OFF�¼�
					{
						p_events->event[i].event_valid = EVENT_HANDLE_END;
						// ON����
						p_events->now_on_off = CAR_STAT_ON;
						p_events->off_to_on_timeslot = 0;
						p_events->on_to_off_timeslot = 0;
						p_events->has_event_no_handle--;
if (print_event_handle_guocheng == 1)	{						
						sprintf(event_handle_debug_buff,"EH:%04X 1ON->[%d %d.%d] \r\n",sid,p_events->event[i].event.blIsOn,p_events->event[i].event.bmSec,p_events->event[i].event.bmMs);
						copy_string_to_double_buff(event_handle_debug_buff);	
}
						p_events->car_onoff_event[0] = p_events->event[i].event;
						return CAR_STAT_ON;
					}
					else if(systerm_info.slot - p_events->event[i].event_rev_time_slot > 1)  //�յ����ON�¼� ����2MS
					{
						p_events->event[i].event_valid = EVENT_HANDLE_END;
						// ON����
						p_events->now_on_off = CAR_STAT_ON;
						p_events->off_to_on_timeslot = 0;
						p_events->on_to_off_timeslot = 0;
						p_events->has_event_no_handle--;
if (print_event_handle_guocheng == 1)	{						
						sprintf(event_handle_debug_buff,"EH:%04X 1ON->[%d %d.%d] \r\n",sid,p_events->event[i].event.blIsOn,p_events->event[i].event.bmSec,p_events->event[i].event.bmMs);
						copy_string_to_double_buff(event_handle_debug_buff);	
}						
						p_events->car_onoff_event[0] = p_events->event[i].event;
						return CAR_STAT_ON;
					}
				}
			}
			else //�Ѿ���һ���ȶ���ON or OFF״̬
			{
				if(p_events->event[i].event.blIsOn == 0) //OFF �¼�
				{
					if(p_events->event[0].event.blIsOn == 0)//ǰһ���¼�Ҳ��OFF
					{
						//��ʧһ��ON�¼�
						memcpy(&p_events->event[i],&p_events->event[i+1],(p_events->event_count-1-i)*sizeof(p_events->event[0]));
						p_events->event_count--;
						i--;
						p_events->has_event_no_handle--;
if (print_event_handle_guocheng == 1)	{						
						sprintf(event_handle_debug_buff,"EH:%04X ����OFF ǰһ��Ҳ��OFF->last:[%d %d.%d] now:[%d %d.%d] \r\n",sid,p_events->event[0].event.blIsOn,p_events->event[0].event.bmSec,p_events->event[0].event.bmMs,
											p_events->event[i].event.blIsOn,p_events->event[i].event.bmSec,p_events->event[i].event.bmMs);
						copy_string_to_double_buff(event_handle_debug_buff);		
}						
						continue;
					}
					else //ǰһ���¼���ON
					{
						if((i+1)<p_events->event_count && p_events->event[i+1].event.blIsOn == 1)  //���滹��ON�¼�  �Ա�OFF�¼�
						{
							calc_ms = calc_event_time_ms(p_events->event[i],p_events->event[i+1]);
							if( calc_ms > sys_flash_param.global_cfg_param.m_usCarLimit)
							{
								//����ֳ���ֵ�� OFF����
							  p_events->event[i].event_valid = EVENT_HANDLE_END;
								p_events->car_onoff_event[1] = p_events->event[i].event;
								p_events->now_on_off = CAR_STAT_OFF;
								p_events->car_count++;
								p_events->off_to_on_timeslot = calc_event_time_ms(p_events->event[i-1],p_events->event[i]);
if (print_event_handle_guocheng == 1)	{									
								sprintf(event_handle_debug_buff,"EH:%04X OFF->[%d %d.%d]%04X  - ON->[%d %d.%d]%04X = %d ��һ��ON�Ա�����ֳ���ֵ\r\n",sid,p_events->event[i].event.blIsOn,p_events->event[i].event.bmSec,p_events->event[i].event.bmMs,p_events->event[i].event.uiAll,
								p_events->event[i-1].event.blIsOn,p_events->event[i-1].event.bmSec,p_events->event[i-1].event.bmMs,p_events->event[i-1].event.uiAll,p_events->off_to_on_timeslot);
								copy_string_to_double_buff(event_handle_debug_buff);	
}								
								memcpy(&p_events->event[0],&p_events->event[1],(p_events->event_count-1)*sizeof(p_events->event[0]));
								p_events->event_count--;	
								p_events->has_event_no_handle--;								
								return CAR_STAT_OFF;
							}
							else
							{
								//������ֳ���ֵ  OFF����   ��һ��ON Ҳ����
								memcpy(&p_events->event[i],&p_events->event[i+2],(p_events->event_count-2-i)*sizeof(p_events->event[0]));
								p_events->event_count -= 2;
								i--;		
								p_events->has_event_no_handle--;
								p_events->has_event_no_handle--;
if (print_event_handle_guocheng == 1)	{										
								sprintf(event_handle_debug_buff,"EH:%04X ����OFF->[%d %d.%d] ����ON->[%d %d.%d] �ԱȲ�����ֳ���ֵ\r\n",sid,p_events->event[i].event.blIsOn,p_events->event[i].event.bmSec,p_events->event[i].event.bmMs
								,p_events->event[i+1].event.blIsOn,p_events->event[i+1].event.bmSec,p_events->event[i+1].event.bmMs);
								copy_string_to_double_buff(event_handle_debug_buff);		
}							
								continue;
							}
						}
						else
						{
							if(systerm_info.slot - p_events->event[i].event_rev_time_slot > sys_flash_param.global_cfg_param.m_usCarLimit/2)   //���˷ֳ���ֵʱ��  
							{
								//����ֳ���ֵ�� OFF����
								p_events->event[i].event_valid = EVENT_HANDLE_END;
								p_events->car_onoff_event[1] = p_events->event[i].event;
								p_events->now_on_off = CAR_STAT_OFF;
								p_events->car_count++;
								p_events->off_to_on_timeslot = calc_event_time_ms(p_events->event[i-1],p_events->event[i]);
if (print_event_handle_guocheng == 1)	{	
							sprintf(event_handle_debug_buff,"EH:%04X OFF->[%d %d.%d]%04X  - ON->[%d %d.%d]%04X = %d �����ֳ���ֵ\r\n",sid,p_events->event[i].event.blIsOn,p_events->event[i].event.bmSec,p_events->event[i].event.bmMs,p_events->event[i].event.uiAll,
								p_events->event[i-1].event.blIsOn,p_events->event[i-1].event.bmSec,p_events->event[i-1].event.bmMs,p_events->event[i-1].event.uiAll,p_events->off_to_on_timeslot);
						copy_string_to_double_buff(event_handle_debug_buff);
}							
								memcpy(&p_events->event[0],&p_events->event[1],(p_events->event_count-1)*sizeof(p_events->event[0]));
								p_events->event_count--;
								p_events->has_event_no_handle--;									
								return CAR_STAT_OFF;
							}
							else 
							{
								//�ֳ���ֵʱ��û�� �����ȴ�
								return -1;
							}
						}
					}
				}
				else  //ON �¼�
				{
					if(p_events->event[0].event.blIsOn == 0)//ǰһ���¼���OFF
					{
						//ON����
						p_events->event[i].event_valid = EVENT_HANDLE_END;
						p_events->car_onoff_event[0] = p_events->event[i].event;
						p_events->now_on_off = CAR_STAT_ON;
						p_events->on_to_off_timeslot = calc_event_time_ms(p_events->event[i-1],p_events->event[i]);
						memcpy(&p_events->event[0],&p_events->event[1],(p_events->event_count-1)*sizeof(p_events->event[0]));
						p_events->event_count--;	
						p_events->has_event_no_handle--;	
if (print_event_handle_guocheng == 1)	{							
						sprintf(event_handle_debug_buff,"EH:%04X ON->[%d %d.%d] \r\n",sid,p_events->event[i].event.blIsOn,p_events->event[i].event.bmSec,p_events->event[i].event.bmMs);
						copy_string_to_double_buff(event_handle_debug_buff);
}					
						
						return CAR_STAT_ON;
					}
					else //ǰһ���¼�Ҳ��ON
					{
						//��ʧһ��OFF�¼�
						//ON����
						p_events->event[i].event_valid = EVENT_HANDLE_END;
						p_events->car_onoff_event[0] = p_events->event[i].event;
						p_events->now_on_off = CAR_STAT_ON;
						p_events->on_to_off_timeslot = calc_event_time_ms(p_events->event[i-1],p_events->event[i]);
						memcpy(&p_events->event[0],&p_events->event[1],(p_events->event_count-1)*sizeof(p_events->event[0]));
						p_events->event_count--;	
						p_events->has_event_no_handle--;
if (print_event_handle_guocheng == 1)	{						
						sprintf(event_handle_debug_buff,"EH:%04X ǰһ��Ҳ��ON ON->[%d %d.%d] \r\n",sid,p_events->event[i].event.blIsOn,p_events->event[i].event.bmSec,p_events->event[i].event.bmMs);
						copy_string_to_double_buff(event_handle_debug_buff);		
}						

						return CAR_STAT_ON;
					}					
				}
			}
			return -100;
		}
	}
}



int32_t calc_speed(SNP_EVENT_t e1_first,SNP_EVENT_t e2_next,uint32_t distance)
{
	int32_t time;
	int32_t speed;
	
	time = (e2_next.bmSec*1024+e2_next.bmMs) - (e1_first.bmSec*1024+e1_first.bmMs);
	if(time < 0)
		time += 30*1024;
	
	speed = (distance*1024)/time;   //mm/S
	return speed;
}

int32_t car_len_calc(int32_t len1,int32_t len2)
{
	return len1;
}


/*
	main ��ѵ���ã�����sensor �¼��� ��ͳ��״̬��



*/
void sensor_event_and_stat_hanle()
{
	int32_t index;
	struct_sensor_event *p_ls = 0;
	int32_t on_or_off = 0;
	uint32_t distance = 4000;
	uint32_t before_car_len;
	uint32_t after_car_len;
	uint32_t before_car_car_len;
	uint32_t after_car_car_len;	
	uint16_t sid;
	
	for(index=0;index<LANE_SENSOR_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.has_event_no_handle || 
			lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.event_count >1)   //ǰ��sensor
		{
			on_or_off = calc_sensor_event(&lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event,lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_id);
				
			if(on_or_off == CAR_STAT_ON)  //��ON�¼������
			{//���㳵ͷʱ��
				if(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.on_event_info.event_valid == EVENT_HANDLE_END)
				{
					lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.car_head_time_distance = calc_event_time_ms(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.on_event_info,
					lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.event[0]);
				}	
				lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.on_event_info = lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.event[0];
			}
		}
		else if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.has_event_no_handle || 
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.event_count >1)  //����sensor
		{
			on_or_off = calc_sensor_event(&lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event,lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_id);
			if(on_or_off == CAR_STAT_ON)  //��ON�¼������
			{
				//�������
				if(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_id !=0 && lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.car_onoff_event[0].uiAll!=0)
				{
					if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_cfg.sensor_before_to_after_distance)
						distance = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_cfg.sensor_before_to_after_distance;
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed = \
								calc_speed(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.car_onoff_event[0],lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_onoff_event[0],distance);
				
				}
				else
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed = 0;
				
				//���㳵ͷʱ��
				if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.on_event_info.event_valid == EVENT_HANDLE_END)
				{
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_head_time_distance = calc_event_time_ms(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.on_event_info,
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.event[0]);
				}	
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.on_event_info = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.event[0];
			}
			else if(on_or_off == CAR_STAT_OFF)
			{
				//ǰ��sensor����ĳ���  ms
				before_car_len = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed * lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.off_to_on_timeslot/1000;
				//����sensor����ĳ���  ms
				after_car_len = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed * lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.off_to_on_timeslot/1000;
				//���˺�ĳ���
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length = car_len_calc(before_car_len,after_car_len);
				
				//ǰ��sensor�������� ms
				before_car_car_len = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed * lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_event.on_to_off_timeslot/1000;
				//����sensor�������� ms
				after_car_car_len = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed * lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.on_to_off_timeslot/1000;
				
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_car_time_distance = (before_car_car_len + after_car_car_len)/2;

if (print_one_car == 1)	
{		
						sid = ((lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_id>>8)&0xff)|((lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_id<<8)&0xff00);	
						sprintf(event_handle_debug_buff,"����:%04X on_time=%d speed=%.1f��/�� len=%.1f�� car_head_dis=%.1f�� car_to_car=%.1f��  \r\n",sid,
								lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.off_to_on_timeslot,
								((double)lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed/(double)1000),
								((double)lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length/(double)1000),
								((double)lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_head_time_distance/(double)1000),
								((double)lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_car_time_distance/(double)1000));
						copy_string_to_double_buff(event_handle_debug_buff);		
}				
				
/*  ����ͳ������ */				
				//�ܳ���
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[5]++;
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].avg_car_car_time_distance += lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_car_time_distance;
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].avg_car_head_time_distance += lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_head_time_distance;
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].avg_car_length += lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length;
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].avg_speed += lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed;
				
				//����ٶ�
				if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed > lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].max_speed)
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].max_speed = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed;
				//��С�ٶ�
				if(((lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed < lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].min_speed) &&
									lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed!=0) || lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].min_speed == 0)
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].min_speed = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.speed;
				//����0	
				if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length<sys_flash_param.global_cfg_param.m_arDelimiter[0])
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[0]++;
				//����1
				if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[0]
					&& lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length < sys_flash_param.global_cfg_param.m_arDelimiter[1])
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[1]++;
				//����2
				if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[1]
					&& lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length < sys_flash_param.global_cfg_param.m_arDelimiter[2])
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[2]++;
				//����3
				if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[2]
					&& lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length < sys_flash_param.global_cfg_param.m_arDelimiter[3])
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[3]++;
				//����4
				if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.car_length > sys_flash_param.global_cfg_param.m_arDelimiter[3])
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[4]++;				
				//ռ���ʣ� �����ʱ����Ҫ/��ʱ��    ����һ�½���ʱ����ON�����
				if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.off_to_on_timeslot > get_sec_to_timer_begin()*1000)
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].occupancy += get_sec_to_timer_begin()*1000;
				else
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].occupancy += lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.off_to_on_timeslot;
					
				
/*  ����ͳ������ */	

			write_realtime_data_to_queue(index);
			}
		}
	}

	for(index=0;index<NOLANE_SENSOR_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.has_event_no_handle || 
			lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event.event_count >1)
		{
			on_or_off = calc_sensor_event(&lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event,lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id);
		}
	}	
	
}




void insert_sensor_stat_packet(SNP_STATE_PACKET_SENSOR_t *pstat,int8_t rssi,uint8_t luzl,uint8_t slot)
{
	uint32_t i=0;
	int32_t index;
	struct_lane_and_sensor *p_las;
	
	index = find_sensor_index_from_lane_to_xx(pstat->sPhr.uiDevId);
	
	if(index < LANE_SENSOR_MAX_COUNT) //�����б����ҵ���sensor
	{
		if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_id == pstat->sPhr.uiDevId)
		{
			p_las = &lane_to_sensor_info_and_result.lane_and_sensor[index].after;
		}
		else if(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_id == pstat->sPhr.uiDevId)
		{
			p_las = &lane_to_sensor_info_and_result.lane_and_sensor[index].before;
		}
		
		p_las->last_packet_time_slot = systerm_info.slot;
		p_las->sensor_stat.luzl = luzl;
		p_las->sensor_stat.rssi = rssi;
		p_las->sensor_stat.slot = slot;		
		memcpy(&p_las->sensor_stat.sensor_stat_packet,pstat,sizeof(SNP_STATE_PACKET_SENSOR_t));
	}
	else
	{
		index -= LANE_SENSOR_MAX_COUNT;
		lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id = pstat->sPhr.uiDevId;
		lane_to_sensor_info_and_result.no_lane_sensor[index].last_packet_time_slot = systerm_info.slot;
		lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.luzl = luzl;
		lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.rssi = rssi;
		lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.slot = slot;
		memcpy(&lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat.sensor_stat_packet,pstat,sizeof(SNP_STATE_PACKET_SENSOR_t));
	}
	
	
	
}



int32_t insert_rp_stat_packet(SNP_STATE_PACKET_RP_t *pstat,int8_t rssi,uint8_t luzl,uint8_t slot)
{
	int32_t index = 0;
	int32_t index_idis0 = -1;
	int32_t index_timeout_biggest = -1;
	
	uint32_t time_slot = 0xffffffff;
	
	for(index=0;index<RP_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_id == pstat->sPhr.uiDevId)
		{
			goto work;
		}
		
		if(lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_id == 0 && index_idis0 == -1)
			index_idis0 = index;
		
		if(lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_cfg.lane_direction == 0 && lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_id != 0) //�����������rp
		{
			if(lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.event_rev_time_slot < time_slot)
			{
				index_timeout_biggest = index;
				time_slot = lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.event_rev_time_slot;				
			}
		}
	}
	
	if(index_idis0 != -1)
	{
		index = index_idis0;
		goto work;
	}
	
	if(index_timeout_biggest != -1)
	{
		index = index_timeout_biggest;
		goto work;		
	}
	
	return -100;
	
	work:	
	lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_id = pstat->sPhr.uiDevId;
	lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.event_rev_time_slot = systerm_info.slot%get_slot_num();
	lane_to_sensor_info_and_result.rp_cfg_and_stat[index].last_packet_time_slot = systerm_info.slot;
	lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.luzl = luzl;
	lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.rssi = rssi;
	lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.slot = slot;
	lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.avg_rssi += (int32_t)rssi;
	lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.avg_volt += pstat->ucVolt*2;
	lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.timer_packet_num++;
	memcpy(&lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.rp_stat_packet,pstat,sizeof(SNP_STATE_PACKET_RP_t));
}



//��ʱ������ �����ݽ���ʹ洢  �洢ʱ�䵽�˵���
void make_timer_statistics_data(uint32_t timer_time_ms)
{
	int32_t index;
	int32_t on_time = 0;
	
	for(index=0;index<LANE_SENSOR_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_id != 0)
		{
				
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.avg_rssi1 = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.avg_rssi/(int32_t)lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.timer_packet_num; 
			if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.avg_rssi1 == 0)
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.avg_rssi1 = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.rssi;
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.lost_rate = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.timer_lost_packet_num*100/(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.timer_lost_packet_num+lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.timer_packet_num);
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.packet_count = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.timer_packet_num;
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.timer_packet_num = 0; 
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.timer_lost_packet_num = 0;
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_stat.avg_rssi = 0;

			memset(&lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1],0,sizeof(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1]));
			
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].car_len_count[0] = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[0];
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].car_len_count[1] = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[1];		
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].car_len_count[2] = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[2];		
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].car_len_count[3] = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[3];		
			lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].car_len_count[4] = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[4];		
			
			//����һЩƽ����
			if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[5] != 0)
			{
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].avg_car_car_time_distance =
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].avg_car_car_time_distance/lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[5]/1000;
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].avg_car_head_time_distance =
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].avg_car_head_time_distance/lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[5]/1000;	
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].avg_car_length =
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].avg_car_length/lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[5];	
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].avg_speed = 
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].avg_speed/lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].car_len_count[5]*36/10000;	
			
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].max_speed = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].max_speed*36/10000;
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].min_speed = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].min_speed*36/10000;
											
			}
			//����ռ����
			if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.now_on_off == CAR_STAT_ON)
			{
				on_time = (systerm_info.slot - lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.on_event_info.event_rev_time_slot)*2;
				//�����ʱ���� ON״̬   ONʱ���Ѿ���������ͳ�ƶ�ʱʱ��
				if(on_time > timer_time_ms)
				{
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].occupancy = 1;
				}
				else
				{
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].occupancy += on_time;
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].occupancy *= 100;
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].occupancy /= timer_time_ms;
					lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].occupancy = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].occupancy ;
				}
			}
			else
			{
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].occupancy *= 100;
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].occupancy /= timer_time_ms;	
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].occupancy = lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0].occupancy ;			
			}
			if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].occupancy>100)
				lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[1].occupancy = 100;
			//�����ʱ����
			memset(&lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0],0,sizeof(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_event.t[0]));
		}
		if(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_id != 0)
		{
			lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.avg_rssi1 = lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.avg_rssi/(int32_t)lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.timer_packet_num; 
			if(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.avg_rssi1 == 0)
				lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.avg_rssi1 = lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.rssi;			
			lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.lost_rate = lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.timer_lost_packet_num*100/(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.timer_lost_packet_num+lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.timer_packet_num);
			lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.packet_count = lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.timer_packet_num;			
			lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.timer_packet_num = 0; 		
			lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.timer_lost_packet_num = 0;		
			lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_stat.avg_rssi = 0;
		}			
		
	}//�������
	
	
}

void make_list_sensor_debug_data()
{
	
	uint32_t index = 0;
	uint32_t line = 0;
	struct_lane_and_sensor * p_sensor;
	struct_sensor_event* p_sensor_event;
	struct_sensor_stat* p_sensor_stat;
	uint16_t sid; 
	uint16_t volt;
	uint16_t time_flush;
	uint8_t mode;
	char* onoff[2] = {"OFF","ON "};
	char* dirction[8] = {"��  ","��  ","��  ","��  ","����","����","����","����"};
  char* str_m_ucmode[29] = {"ģʽA","ģʽB","ģʽC","ģʽD","ģʽE","ģʽF","ģʽG","ģʽH", "��� ", "��� ", "��� ", "��� "
	,"ģʽ0","ģʽ1","ģʽ2","ģʽ3","ģʽ4","ģʽ5","ģʽ6","ģʽ7","ģʽ8","ģʽ9","ģʽ10","ģʽ11","ģʽ12","ģʽ13","ģʽ14","ģʽ15","��� "};	
	
	memset(list_sensor_debug_buff,0,ROW_MAX*LINE_MAX);
	
	memset(list_sensor_debug_buff[line],' ',LINE_MAX);
	list_sensor_debug_buff[line][LINE_MAX-1] = '\n';
	list_sensor_debug_buff[line][LINE_MAX-2] = '\r';
	sprintf(list_sensor_debug_buff[line],"033sunap:version=%d.%d apid=%08X Bandid=%04X GPSN=%f:%f ch=%d %d %d %d serverip=%d.%d.%d.%d port=%d live=%d",
	AP_VERSION>>8,AP_VERSION&0xff,sys_flash_param.ap_param.ap_id,sys_flash_param.ap_param.band_id,sys_flash_param.ap_param.gps_n_e[0], sys_flash_param.ap_param.gps_n_e[1],(sys_flash_param.ap_param.ap_channel>>0)&0XFF,(sys_flash_param.ap_param.ap_channel>>8)&0XFF,(sys_flash_param.ap_param.ap_channel>>16)&0XFF
	,(sys_flash_param.ap_param.ap_channel>>24)&0XFF,sys_flash_param.global_cfg_param.server_ip[0]&0xff,(sys_flash_param.global_cfg_param.server_ip[0]>>8)&0xff,
	(sys_flash_param.global_cfg_param.server_ip[0]>>16)&0xff,sys_flash_param.global_cfg_param.server_ip[0]>>24,sys_flash_param.global_cfg_param.server_port[0],systerm_info.slot/512);



	
	line++;
	for(index=0;index<LANE_SENSOR_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_id != 0)
		{
		
			p_sensor = &lane_to_sensor_info_and_result.lane_and_sensor[index].before;
			
			if(p_sensor->sensor_stat.sensor_stat_packet.sPhr.ucSensorMode<100 || p_sensor->sensor_stat.sensor_stat_packet.sPhr.ucSensorMode>127)
				mode = 28;
			else
				mode = p_sensor->sensor_stat.sensor_stat_packet.sPhr.ucSensorMode -100;
			sid = ((p_sensor->sensor_id>>8)&0xff)|((p_sensor->sensor_id<<8)&0xff00);
			volt = p_sensor->sensor_stat.sensor_stat_packet.ucVolt*2;
			time_flush = (systerm_info.slot - p_sensor->last_packet_time_slot)/1000;
			memset(list_sensor_debug_buff[line],' ',LINE_MAX-2);
			list_sensor_debug_buff[line][LINE_MAX-1] = '\n';
			list_sensor_debug_buff[line][LINE_MAX-2] = '\r';
			sprintf(list_sensor_debug_buff[line],"[%02d]_%02d SID=%04X %d.%02dV T=%5ds car=%5d RSSI=%3d:%3d L=%3d %s Mode=%s Slot=%3d RPid=%04X CH=%02d F=%s V=%3d.%d ALL=%4d lost=%4d resend=%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d",
				line,index+1,sid,volt/100,volt%100,time_flush,p_sensor->sensor_event.car_count,p_sensor->sensor_stat.rssi,(int8_t)p_sensor->sensor_stat.sensor_stat_packet.uiRssi-76,p_sensor->sensor_stat.luzl,
				onoff[p_sensor->sensor_event.now_on_off],str_m_ucmode[mode],p_sensor->sensor_stat.slot,p_sensor->sensor_stat.sensor_stat_packet.uiRpId,p_sensor->sensor_cfg.rf_ch,
				dirction[p_sensor->sensor_cfg.lane_direction],p_sensor->sensor_stat.sensor_stat_packet.uiFwVer,p_sensor->sensor_stat.sensor_stat_packet.uiHwVer,p_sensor->sensor_event.all_rev_event_count,p_sensor->sensor_event.all_lost_event_count,
			p_sensor->sensor_event.resend[0],p_sensor->sensor_event.resend[1],p_sensor->sensor_event.resend[2],p_sensor->sensor_event.resend[3],p_sensor->sensor_event.resend[4],p_sensor->sensor_event.resend[5],
			p_sensor->sensor_event.resend[6],p_sensor->sensor_event.resend[7],p_sensor->sensor_event.resend[8],p_sensor->sensor_event.resend[9]);
			line++;
		}
		if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_id != 0)
		{

			p_sensor = &lane_to_sensor_info_and_result.lane_and_sensor[index].after;
			if(p_sensor->sensor_stat.sensor_stat_packet.sPhr.ucSensorMode<100 || p_sensor->sensor_stat.sensor_stat_packet.sPhr.ucSensorMode>127)
				mode = 28;
			else
				mode = p_sensor->sensor_stat.sensor_stat_packet.sPhr.ucSensorMode -100;			
			sid = ((p_sensor->sensor_id>>8)&0xff)|((p_sensor->sensor_id<<8)&0xff00);
			volt = p_sensor->sensor_stat.sensor_stat_packet.ucVolt*2;
			time_flush = (systerm_info.slot - p_sensor->last_packet_time_slot)/1000;
			memset(list_sensor_debug_buff[line],' ',LINE_MAX-2);
			list_sensor_debug_buff[line][LINE_MAX-1] = '\n';
			list_sensor_debug_buff[line][LINE_MAX-2] = '\r';
			sprintf(list_sensor_debug_buff[line],"[%02d]_%02d SID=%04X %d.%02dV T=%5ds car=%5d RSSI=%3d:%3d L=%3d %s Mode=%s Slot=%3d RPid=%04X CH=%02d F=%s V=%3d.%d ALL=%4d lost=%4d resend=%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d",
				line,index+1,sid,volt/100,volt%100,time_flush,p_sensor->sensor_event.car_count,p_sensor->sensor_stat.rssi,(int8_t)p_sensor->sensor_stat.sensor_stat_packet.uiRssi-76,p_sensor->sensor_stat.luzl,
				onoff[p_sensor->sensor_event.now_on_off],str_m_ucmode[mode],p_sensor->sensor_stat.slot,p_sensor->sensor_stat.sensor_stat_packet.uiRpId,p_sensor->sensor_cfg.rf_ch,
				dirction[p_sensor->sensor_cfg.lane_direction],p_sensor->sensor_stat.sensor_stat_packet.uiFwVer,p_sensor->sensor_stat.sensor_stat_packet.uiHwVer,p_sensor->sensor_event.all_rev_event_count,p_sensor->sensor_event.all_lost_event_count,
			p_sensor->sensor_event.resend[0],p_sensor->sensor_event.resend[1],p_sensor->sensor_event.resend[2],p_sensor->sensor_event.resend[3],p_sensor->sensor_event.resend[4],p_sensor->sensor_event.resend[5],
			p_sensor->sensor_event.resend[6],p_sensor->sensor_event.resend[7],p_sensor->sensor_event.resend[8],p_sensor->sensor_event.resend[9]);
			line++;
		}		
		if(line>=ROW_MAX)
			return;
	}
	
	for(index=0;index<RP_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_id != 0)
		{
			p_sensor_event = &lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event;
			p_sensor_stat = &lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat;

			sid = ((lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_id>>8)&0xff)|((lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_id<<8)&0xff00);
			volt = lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.rp_stat_packet.ucVolt*2;
			time_flush = (systerm_info.slot - lane_to_sensor_info_and_result.rp_cfg_and_stat[index].last_packet_time_slot)/1000;
			memset(list_sensor_debug_buff[line],' ',LINE_MAX-2);
			list_sensor_debug_buff[line][LINE_MAX-1] = '\n';
			list_sensor_debug_buff[line][LINE_MAX-2] = '\r';
			sprintf(list_sensor_debug_buff[line],"[%02d]___RPID=%04X %d.%02dV T=%5ds RSSI=%3d:%3d L=%3d Slot=%3d RPid=%04X CH=%02d->%02d V=%3d.%d",
				line,sid,volt/100,volt%100,time_flush,lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.rssi,(int8_t)lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.rp_stat_packet.uiRssi-76,lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.luzl,
				lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.rp_stat_packet.uiSlot & 0xff,lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.rp_stat_packet.uiRpId,
				lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.rp_stat_packet.uiChannel,lane_to_sensor_info_and_result.rp_cfg_and_stat[index].rp_stat.rp_stat_packet.sPhr.ucSensorMode, p_sensor_stat->sensor_stat_packet.uiFwVer,
			p_sensor_stat->sensor_stat_packet.uiHwVer);
			line++;
			if(line>=ROW_MAX)
				return;
		}
	}		
	
	for(index=0;index<NOLANE_SENSOR_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id != 0)
		{
			p_sensor_event = &lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event;
			p_sensor_stat = &lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_stat;

			if(p_sensor->sensor_stat.sensor_stat_packet.sPhr.ucSensorMode<100 || p_sensor->sensor_stat.sensor_stat_packet.sPhr.ucSensorMode>127)
				mode = 28;
			else
				mode = p_sensor->sensor_stat.sensor_stat_packet.sPhr.ucSensorMode -100;
			sid = ((lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id>>8)&0xff)|((lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id<<8)&0xff00);
			volt = p_sensor_stat->sensor_stat_packet.ucVolt*2;
			time_flush = (systerm_info.slot - lane_to_sensor_info_and_result.no_lane_sensor[index].last_packet_time_slot)/1000;
			memset(list_sensor_debug_buff[line],' ',LINE_MAX-2);
			list_sensor_debug_buff[line][LINE_MAX-1] = '\n';
			list_sensor_debug_buff[line][LINE_MAX-2] = '\r';
			sprintf(list_sensor_debug_buff[line],"[%02d]_xx SID=%04X %d.%02dV T=%5ds car=%5d RSSI=%3d:%3d L=%3d %s Mode=%s Slot=%3d RPid=%04X CH=%02d F=%s V=%3d.%d ALL=%4d lost=%4d resend=%3d %3d %3d %3d %3d %3d %3d %3d %3d %3d",
				line,sid,volt/100,volt%100,time_flush,p_sensor_event->car_count,p_sensor_stat->rssi,(int8_t)p_sensor->sensor_stat.sensor_stat_packet.uiRssi-76,p_sensor_stat->luzl,
				onoff[p_sensor_event->now_on_off],str_m_ucmode[mode],p_sensor_stat->slot,p_sensor->sensor_stat.sensor_stat_packet.uiRpId,0,
				"��  ",p_sensor_stat->sensor_stat_packet.uiFwVer,p_sensor_stat->sensor_stat_packet.uiHwVer,p_sensor_event->all_rev_event_count,p_sensor_event->all_lost_event_count,
			p_sensor_event->resend[0],p_sensor_event->resend[1],p_sensor_event->resend[2],p_sensor_event->resend[3],p_sensor_event->resend[4],p_sensor_event->resend[5],
			p_sensor_event->resend[6],p_sensor_event->resend[7],p_sensor_event->resend[8],p_sensor_event->resend[9]);
			line++;
			if(line>=ROW_MAX)
				return;
		}
	}


}


/*
	���ճ�����ʾһ�м���

*/
uint32_t make_sensor_car_count_debug_data()
{
	
	uint32_t index = 0;
	uint32_t line = 0;
	struct_lane_and_sensor * p_sensor;
	struct_sensor_event* p_sensor_event;
	struct_sensor_stat* p_sensor_stat;
	uint16_t sid; 
	uint16_t volt;
	uint16_t time_flush;
	char *pd = list_sensor_debug_buff[0];
	
	
	memset(list_sensor_debug_buff,0,ROW_MAX*LINE_MAX);
	
	
	for(index=0;index<LANE_SENSOR_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.lane_and_sensor[index].before.sensor_id != 0)
		{

			p_sensor = &lane_to_sensor_info_and_result.lane_and_sensor[index].before;
			sid = ((p_sensor->sensor_id>>8)&0xff)|((p_sensor->sensor_id<<8)&0xff00);

			time_flush = (systerm_info.slot - p_sensor->last_packet_time_slot)/1000;
			sprintf(pd+line,"L%02d_%04X: %04d t=%03d ",
				index,sid,p_sensor->sensor_event.car_count,time_flush);
			line += 21;
		}
		if(lane_to_sensor_info_and_result.lane_and_sensor[index].after.sensor_id != 0)
		{

			p_sensor = &lane_to_sensor_info_and_result.lane_and_sensor[index].after;
			sid = ((p_sensor->sensor_id>>8)&0xff)|((p_sensor->sensor_id<<8)&0xff00);

			time_flush = (systerm_info.slot - p_sensor->last_packet_time_slot)/1000;
			sprintf(pd+line,"L%02d_%04X: %04d t=%03d ",
				index,sid,p_sensor->sensor_event.car_count,time_flush);
			line += 21;
		}		
		
	}

	for(index=0;index<NOLANE_SENSOR_MAX_COUNT;index++)
	{
		if(lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id != 0)
		{
			p_sensor_event = &lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_event;
			sid = ((lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id>>8)&0xff)|((lane_to_sensor_info_and_result.no_lane_sensor[index].sensor_id<<8)&0xff00);

			time_flush = (systerm_info.slot - lane_to_sensor_info_and_result.no_lane_sensor[index].last_packet_time_slot)/1000;
			sprintf(pd+line,"%04X:%04d ",sid,p_sensor_event->car_count);
			line += 10;
		}
	}	
	pd[line++] = '\r';
	return line;
}


void add_sensor_cfg(uint8_t lane,uint16_t sid,uint8_t before_or_after)
{
	if(before_or_after)
		lane_to_sensor_info_and_result.lane_and_sensor[lane].after.sensor_id = sid;
	else
		lane_to_sensor_info_and_result.lane_and_sensor[lane].before.sensor_id = sid;
}


