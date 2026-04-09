#ifndef __MQTT_H
#define __MQTT_H


#define UID  "759960d096494f47b96e70151c5ba421"    //用户标识符



#define Motor_TOPIC        "Motor"
#define SIGNAL_TOPIC    	 "Signal"
#define PICTURE_TOPIC      "Picture"

#define TAKE_TOPIC       	 "Take"

#define RELAY_TOPIC     	 "Relay"
#define STATE_TOPIC     	 "State"

#define MESSAGE_TOPIC			 "Message" 	 
// 0：摄像头初始化成功  1：摄像头初始化失败   
// 2：电机初始化成功    3：电机初始化失败  
// 4：4G模块初始化成功  5：4G模块初始化失败
 


extern  uint8_t PING_FLAG;


void Timer_Init(void);
void StartPing(uint32_t arr, uint32_t psc);
uint8_t MQTT_Ping(void);
uint8_t MQTT_Subscribe(char* uid,char *topic);              
uint8_t MQTT_Publish(char * uid,char *topic, char *data);
void Handle_Data(char * data);

void MQTT_Test(void);

#endif
