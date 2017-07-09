/*******************************************************
*******************************************************/
//желтый текст на синем фоне 15,15,3; 1,1,2
//желтый на коричневом 20,35,3;  5,3,1

#include <mega328p.h>
#include <spi.h>    //SPI
#include <delay.h>  //библиотека программных задержек
#include <stdio.h>  //стандартная либа ввода-вывода Си
#include <twi.h>    // i2c

#include <font_settings.h> //настроить используемый шрифт здесь, если стандартный планируется заменить
#include <ili9163.h>

#define ADC_ADDR   0b1001000
#define ADC_START  0b10001001
#define ADC_READ   0b00
#define ADC_CONFIG 0b01

#define SAMPLING_DEGREE 7    //количество сэмплов для устреднения результата будет 2^(SAMPLING_DEGREE)

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void){
}

unsigned int result, mVx4;
float pH;


 int adc_conv(){
  long adc=0;
  int i;
  char tx_buffer[4]; // transmission buffer
  char rx_buffer[2]; // receive buffer
  
  tx_buffer[0]=ADC_READ; //запрос результата
  
  tx_buffer[1]=ADC_CONFIG; //register address
  tx_buffer[2]=ADC_START;  //Start conversion + set.bit 1
  tx_buffer[3]=0b10000011; //set.bit 2
  
  for(i=0; i<(1<<SAMPLING_DEGREE); i++){
                      

   if (twi_master_trans( ADC_ADDR, &tx_buffer[1], 3, 0, 0 )){   //запустить преобразование
     delay_us(1000);       
     twi_master_trans( ADC_ADDR, &tx_buffer[0], 1, &rx_buffer[0], 2 );     
     adc += ((int)rx_buffer[0]<<4) + ((int)rx_buffer[1]>>4);
   }


   else {
     lcd_fill_str(8, 31, 1, 1);
     text_color(31,31,1);
     lcd_text(8,0,"ADC_Error");
     while(!twi_master_trans( ADC_ADDR, &tx_buffer[1], 3, 0, 0 ));
     lcd_fill_str(8, 1, 1, 2);
     text_color(20,25,3);
     lcd_text(8,0,"ADC Normal Mode");
   }
  }
  
  result = (adc>>SAMPLING_DEGREE);    
  return result;  
 }
 
 int get_mVx4(){
 adc_conv();
 if (result<2047) return result;
 else return     -(4096-result);
 }
 
void main(void){
// Input/Output Ports initialization
DDRB=(0<<DDB7) | (0<<DDB6) | (1<<DDB5) | (0<<DDB4) | (1<<DDB3) | (1<<DDB2) | (1<<DDB1) | (1<<DDB0);
PORTB=(0<<PORTB7) | (0<<PORTB6) | (0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (0<<PORTB2) | (1<<PORTB1) | (0<<PORTB0);

DDRD.7=1;

// spi initialization Clock Rate: 4000,000 kHz MSB First
SPCR=(1<<SPE) | (1<<MSTR) | (1<<CPOL);
SPSR=(1<<SPI2X);

// Timer/Counter 0 initialization
TCCR0B=(0<<WGM02) | (0<<CS02) | (1<<CS01) | (0<<CS00);
// Timer/Counter 0 Interrupt(s) initialization
TIMSK0=(0<<OCIE0B) | (0<<OCIE0A) | (1<<TOIE0);

// TWI initialization
// Mode: TWI Master
// Bit Rate: 100 kHz
twi_master_init(100);
// Global enable interrupts
#asm("sei")


lcd_init();
lcd_fill(1,1,1);

text_color(20,25,3);
bg_color(5,3,1);


lcd_fill_str(0, 1, 1, 2);
lcd_text(0,0,"Результат  I2C:");

lcd_fill_str(8, 1, 1, 2);
lcd_text(8,0,"ADC Sampling");
  
lcd_fill_str(2, 1, 1, 2);

  lcd_text(4,0,"Volt:        ");
  while(1){
  adc_conv();
  mVx4 = get_mVx4();
  pH=(7-(float)mVx4/256);
  
  lcd_1var(2,0, "ADC: %u      ", result);
  
  lcd_2var(4,5, "%i.%u mV", mVx4/4, (mVx4*25)%100);
  
  lcd_2var(6,0, "pH = %u.%u   ", pH, ((int)(pH*100))%100);
  }

}
