#include "mbed.h"
#include "MMA7660.h"
#include "C12832.h"
#include "LM75B.h"
#define VIB 1
#define TEMP 2
#define MENU 3

int state = MENU;
//Set JoySticks
BusIn down(p12);
BusIn up(p15);
DigitalIn centre(p14);
//set accelarometer
MMA7660 MMA(p28, p27);
//set sensor pins
LM75B sensor(p28, p27);
//set screen pins 
C12832 lcd(p5, p7, p6, p8, p11);
//set led for connection
DigitalOut connectionLed(LED1);
DigitalOut DigitalPin(LED3);
//set led 
PwmOut red(p23);
PwmOut green(p24);
PwmOut blue(p25);
//set speaker
PwmOut speaker(p26);
//set flippers
Ticker screen_flipper,red_led_flipper,blue_led_flipper;
//set pot
AnalogIn pot1(p19);
AnalogIn pot2(p20);
//set varibles
float lower_threshold_temp;
float lower_threshold_min_temp= 10.0;
float higher_threshold_max_temp = 50;
float higher_threshold_temp;
float lower_threshold_vibration = 0.10;
float higher_threshold_vibration;
float higher_threshold_max_vibration = 1;
//set axis
float Xaxis,Yaxis,Zaxis = 0;
//previous values for axis
float Pxaxis,Pyaxis,Pzaxis = 0;
float vibration;
float speaker_period = 0.003;
int flashing_led_red,flashing_led_blue = 0;
int screen_type = 3;
float room_temp = (float)sensor;

void print_lcd_menu(){
    lcd.cls();
    lcd.locate(0,1);
    lcd.printf("Menu");
    lcd.locate(0,12);
    lcd.printf("UP = Vibration Sensor\n");
    lcd.printf("Down = Temperature Sensor");
}

void print_lcd_vib(){
    lcd.cls();
    lcd.locate(0,1);
    lcd.printf("Axis': X= %.1f", Pxaxis);
    lcd.printf(" Y= %.1f", Pyaxis);
    lcd.printf(" Z= %.1f\n", Pzaxis);
    lcd.locate(0,12);
    lcd.printf("HigherV: %.1f",higher_threshold_vibration);
    lcd.printf("Vibration: %.1f",vibration);
    lcd.locate(0,20);
    lcd.printf("HigherT: %.1f", higher_threshold_temp );
    lcd.printf(" Temp: %.1f\n", (float)sensor);
}

void print_lcd_temp(){
    lcd.cls();
    lcd.locate(0,1);
    lcd.printf("Lower T= %.1f\n", lower_threshold_temp);
    lcd.locate(0,12);
    lcd.printf("Higher T = %.1f\n", higher_threshold_temp);
    lcd.locate(0,20);
    lcd.printf("Temp = %.1f\n", (float)sensor);
}

void screen_flip(){
    if(screen_type == 1){
        print_lcd_vib();
    }
    else if (screen_type ==2){
        print_lcd_temp();
    }
    else if (screen_type ==3){
        print_lcd_menu();
    }
}

void flip_led_red(){
    flashing_led_red = !flashing_led_red; 
}
void flip_led_blue(){
    flashing_led_blue = !flashing_led_blue;   
}
void speaker_cycle(float x){
    speaker = x;
}

float map(float x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void set_thresholds_vib(){
    //maps the pots to the thresholds
    higher_threshold_temp  = map(pot1,0,1,int(lower_threshold_temp),higher_threshold_max_temp);
    higher_threshold_vibration = map(pot2,0,1,lower_threshold_vibration,higher_threshold_max_vibration );
}

void set_thresholds_temp(){
    //maps the pots to the thresholds
    lower_threshold_temp  = map(pot1,0,1,int(lower_threshold_min_temp),int(higher_threshold_temp)) - 1;
    higher_threshold_temp  = map(pot2,0,1,int(lower_threshold_temp ),higher_threshold_max_temp);          
}

void led_color(int rd,int gr,int bl){
    red = rd;
    green = gr;
    blue = bl;
}

void vibration_monitoring() {  
    //tests connection to accelerometer
    if (MMA.testConnection()){
        connectionLed = 1;
    }
    //set pervious axis    //set current axis
    Xaxis = MMA.x();
    Yaxis = MMA.y();
    Zaxis = MMA.z();
    //set vibration
    vibration = (Pxaxis - Xaxis) + (Pyaxis - Yaxis) + (Pzaxis - Zaxis) / 3;
    if(vibration < 0){
        vibration = vibration * -1;
    }
    Pxaxis = Xaxis;
    Pyaxis = Yaxis;
    Pzaxis = Zaxis;
    //vibration = (Xaxis) + (Yaxis) + (Zaxis) / 3;
    led_color(1,1,1);
    set_thresholds_vib();
    speaker_cycle(0);
    
    if((vibration >= higher_threshold_vibration) && ((float)sensor < higher_threshold_temp )){
        speaker_cycle(0);
        if(flashing_led_blue == 1){
            led_color(1,1,0);//blue
        }
        else{
            led_color(1,1,1); //turns off led
        }  
        DigitalPin = 0; 
    }
    if((vibration >= higher_threshold_vibration) && ((float)sensor >= higher_threshold_temp )){
        
        if(flashing_led_red == 1){
            led_color(0,1,1);//red
            speaker_cycle(0.95); //95% duty cycle
            DigitalPin = 1; 
        }
        else{
            led_color(1,1,1); //turns off led
            speaker_period = vibration * 0.001;
            DigitalPin = 0; 
            //speaker_period = map(vibration,0.001,0.003,lower_threshold_vibration,higher_threshold_max_vibration);
            speaker_cycle(0.0); //0% duty cycle
        }
    }
}


void temp_monitoring() { 
    room_temp = (float)sensor;
    //Try to open the LM75B
    if(((float)sensor < lower_threshold_temp ) && (lower_threshold_temp != higher_threshold_temp )){
        led_color(1,0,1);//makes led green r = 1, g = 0, b = 1
        set_thresholds_temp();
        speaker_cycle(0.0);
        //print_lcd();//prints temperature to screen   
    }
    else if((float)sensor >= higher_threshold_temp ){
        set_thresholds_temp();
        speaker.period(0.001);
        if(flashing_led_red == 1){
            led_color(0,1,1);
            speaker_cycle(0.95); //95% duty cycle   
        }
        else{
            led_color(1,1,1); //turns off led
               
        }
    }
    else if(((float)sensor >= lower_threshold_temp )&& ((float)sensor < higher_threshold_temp ) && (lower_threshold_temp  != higher_threshold_temp )){       
        set_thresholds_temp();
        speaker.period(0.003);
        if(flashing_led_blue == 1){
            led_color(0,0,1);
            speaker_cycle(0.95); //95% duty cycle
        }
        else{
            led_color(1,1,1); //turns off led 
            speaker_cycle(0.95); //95% duty cycle
        }   
    }
}
void FSM(){
     switch(state){
        case MENU:
            screen_type = 3;
            speaker = 0;
            led_color(1,0,1);
            if(up){ 
                state=VIB;
            }
            else if(down){
                 state=TEMP;
            }
            break;
        case VIB:
            if(centre){
                state=MENU;
            }
            else if(!centre){
                screen_type = 1;
                vibration_monitoring();
            }
            break;
        case TEMP:
            if(centre){
                state=MENU;
             }
            else if(!centre){
              //sets variables to stop errors
              lower_threshold_min_temp= 10.0;
              higher_threshold_max_temp = 50;
              screen_type = 2;
              temp_monitoring();
            }
            break;
}   
}
int main(){
    // setup flipper to call flip after 2 seconds
    blue_led_flipper.attach(&flip_led_blue, 2.0);
    red_led_flipper.attach(&flip_led_red, .20);
    screen_flipper.attach(&screen_flip, 2.0);
    speaker.period(0.003);
    led_color(1,1,1);
    Pxaxis = MMA.x();
    Pyaxis = MMA.y();
    Pzaxis = MMA.z();
    while(1){
       FSM();
    }
}

