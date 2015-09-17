#include <LiquidCrystal.h>
#include <Wire.h>
#include <DS3232RTC.h>
#include <Time.h>


#define DS3231_I2C_ADDRESS 0x68				// RTC Module
#define AT24C32_I2C_ADDRESS 0x57			// EEPROM on RTC
#define btnRIGHT  0							// LCD Keypad Shild
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5
 
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);           // select the pins used on the LCD panel

// RTC= byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year

unsigned long sTime = 0;
unsigned int actionTimeProgram1 = 5;   // minute
boolean ALARM1 = false;

// Custom Character
byte sunDay[8] = {
0b01111,
0b10000,
0b10000,
0b01110,
0b00001,
0b11110,
0b00000,
0b11111
};

byte monDay[8] = {
0b10001,
0b11011,
0b10101,
0b10101,
0b10001,
0b10001,
0b00000,
0b11111
};

byte tuesDay[8] = {
0b11111,
0b00100,
0b00100,
0b00100,
0b00100,
0b00100,
0b00000,
0b11111
};

byte wednesDay[8] = {
0b10001,
0b10001,
0b10001,
0b10101,
0b10101,
0b01010,
0b00000,
0b11111
};

byte thursDay[8] = {
0b11111,
0b00100,
0b00100,
0b00100,
0b00100,
0b00100,
0b00000,
0b11111
};

byte friDay[8] = {
0b11111,
0b10000,
0b10000,
0b11110,
0b10000,
0b10000,
0b00000,
0b11111
};

byte saturDay[8] = {
0b01111,
0b10000,
0b10000,
0b01110,
0b00001,
0b11110,
0b00000,
0b11111
};

char* dayOfWeek[7][7] = { //[Row][Col]
	{ "s", "S" },
	{ "m", "M" },
	{ "t", "T" },
	{ "w", "W" },
	{ "t", "T" },
	{ "f", "F" },
	{ "s", "S" },
};



// define some values used by the panel and buttons
int lcd_key         = 0;
int last_lcd_key    = 5;

int adc_key_in      = 0;

int curr_menu_main  = 1;
int curr_menu_sub1;
int curr_menu_sub2;

int curr_menu_sub21 = 0; 
int max_menu_sub21 = 12;

int menu_sub2x[12][4] = {   //[Row][Col] = Col , Row , TextSize and Max choice
	{1, 0, 1, 1}, 
	{2, 0, 1, 1}, 
	{3, 0, 1, 1}, 
	{4, 0, 1, 1},
	{5, 0, 1, 1},
	{6, 0, 1, 1},
	{7, 0, 1, 1},
	{11,0, 2,23},	// Set Hour
	{14,0, 2,59},	// Set Minutes
	{3, 1, 2,30},   // every 30 days
	{11,1, 1, 9},	// water 9 hours
	{13,1, 2,59}	// water 59 minutes
};

int curr_val_sub2x[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

boolean blink_val_sub2x = false;

int curr_menu_sub3; 
int curr_menu_sub4;

int selected_menu   = 99;

int no_menus[] = { 5, 3, 3, 3, 3};

int waterPIN = 2;
int lcdLightPIN = 10;
 
void setup(){
	Serial.begin(9600);
	
	Wire.begin();
	

	//Restore from EEPROM
	eepromRead(0, 0, 12);

	// set the initial time here:
	// DS3231 seconds, minutes, hours, day, date, month, year
	//setDS3231time(00,16,20,7,13,9,2015);

	// create a new character
	lcd.createChar(0, sunDay);
	lcd.createChar(1, monDay);
	lcd.createChar(2, tuesDay);
	lcd.createChar(3, wednesDay);
	lcd.createChar(4, thursDay);
	lcd.createChar(5, friDay);
	lcd.createChar(6, saturDay);
  
	analogWrite(lcdLightPIN, 20);
	pinMode(2, OUTPUT);
	digitalWrite(waterPIN, LOW);
	lcd.begin(16, 2);               // start the library
	showDateTime();
	
}
 
void eepromWrite(byte highAddress, byte lowAddress, int data[], int byteWrite) {

	Wire.beginTransmission(AT24C32_I2C_ADDRESS);
	Wire.write(highAddress);
	Wire.write(lowAddress);

	for (byte i = 0; i<byteWrite; i++)      //Write 26 data bytes
	{
		Wire.write(data[i]);
	}
	delay(10);

	Wire.endTransmission();
	delay(10);
}

void eepromRead(byte highAddress, byte lowAddress, int byteRead) {

	Wire.beginTransmission(AT24C32_I2C_ADDRESS);
	Wire.write(highAddress);
	Wire.write(lowAddress);
	Wire.endTransmission();
	delay(10);

	Wire.requestFrom(AT24C32_I2C_ADDRESS, byteRead);
	delay(10);

	for (int i = 0; i<byteRead; i++)     
	{
		curr_val_sub2x[i] = Wire.read();

	}
	delay(10);
}

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val) {
	return((val / 10 * 16) + (val % 10));
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val) {
	return((val / 16 * 10) + (val % 16));
}

void setDS3231time(uint8_t second, uint8_t minute, uint8_t hour, uint8_t dayOfWeek, uint8_t dayOfMonth, uint8_t month, uint8_t year) {
	// sets time and date data to DS3231
	tmElements_t tm;
	tm.Wday = dayOfWeek;  //1=sunday 2=monday ... 7=saturday
	tm.Hour = hour;            
	tm.Minute = minute;
	tm.Second = second;
	tm.Day = dayOfMonth;
	tm.Month = month;
	tm.Year = year - 1970;    //tmElements_t.Year is the offset from 1970
	RTC.write(tm);
}

int  read_LCD_buttons(){               // read the buttons
    adc_key_in = analogRead(0);       // read the value from the sensor 
 
    // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
    // we add approx 50 to those values and check to see if we are close
    // We make this the 1st option for speed reasons since it will be the most likely result
 
    if (adc_key_in > 1000) return btnNONE; 
 /*
    // For V1.1 us this threshold
    if (adc_key_in < 50)   return btnRIGHT;  
    if (adc_key_in < 250)  return btnUP; 
    if (adc_key_in < 450)  return btnDOWN; 
    if (adc_key_in < 650)  return btnLEFT; 
    if (adc_key_in < 850)  return btnSELECT;  
 */
   // For V1.0 comment the other threshold and use the one below:
   
     if (adc_key_in < 50)   return btnRIGHT;  
     if (adc_key_in < 195)  return btnUP; 
     if (adc_key_in < 380)  return btnDOWN; 
     if (adc_key_in < 555)  return btnLEFT; 
     if (adc_key_in < 790)  return btnSELECT;   
   
 
    return btnNONE;                // when all others fail, return this.
}
  


void loop(){
	
	
	

	switch (selected_menu) {
	case 0:
		mainMenu();
		break;
	case 1:
		subMenu1();
		break;
	case 2:
		subMenu2();
		break;
	case 21:
		subMenu21();
		break;
	case 22:
		subMenu22();
		break;
	case 3:
		subMenu3();
		break;
	case 4:
		subMenu4();
		break;
	case 99:
		showDateTime(); 
		break;
	}


	if (RTC.alarm(ALARM_1)) {
		ALARM1 = true;
		sTime = millis();
		lcd.setCursor(15, 15); lcd.write("*");
		Serial.write("Alarm!");
	} else {
		Serial.write("Wait..");
		delay(1000);
	}

	if (ALARM1) {
		if (millis()-sTime <= (actionTimeProgram1*60*1000)) {
			lcd.setCursor(15, 15); lcd.write("*");
		} else {
			lcd.setCursor(15, 15); lcd.write(" ");
			ALARM1 = false;
		}
	}
	

}

void showDateTime() {
	
	int t = RTC.temperature();
	float celsius = t / 4.0;

	tmElements_t tm;
	RTC.read(tm);
	
	String dayName = dayOfWeek[(int)tm.Wday][1];
	lcd.setCursor(0, 0); lcd.print(dayName);
	
	int shortYear = (1970 + tm.Year) - 2000;
	char date[9] = ""; sprintf(date, "%02d/%02d/%02d", (int)tm.Day, (int)tm.Month, shortYear);
	lcd.setCursor(2, 0); lcd.print(date);
	
	char time[6] = ""; sprintf(time, "%02d:%02d", (int)tm.Hour, (int)tm.Minute);
	
	lcd.setCursor(11, 0); lcd.print(time);
	
	lcd.setCursor(0, 1);  
	lcd.print(celsius);
	lcd.write(B11011111);
	lcd.print("        ");
	
	
	lcd_key = read_LCD_buttons();
	if ((lcd_key != last_lcd_key)&&(lcd_key == btnDOWN)) {    // Enter to Main Menu
		last_lcd_key = lcd_key;
		selected_menu  = 0;
		curr_menu_main = 1;
	}
	delay(5);
}

void displayMainMenu(int menu) {
   lcd.clear();
   lcd.setCursor(0,0);             // set the LCD cursor   position 
   lcd.print("Main Menu");  // print a simple message on the LCD
   lcd.setCursor(0,1);             // move to the begining of the second line
   switch (menu) {
    case 1:
          lcd.print(" 1. Date Time  >");
          break;
    case 2:
          lcd.print("<2. Scheduling >");
          break;
    case 3:
          lcd.print("<3. Sensors    >");
          break;
    case 4:
          lcd.print("<4. Turn On|Off>");
          break;
	case 5:
		  lcd.print("<5. Exit        ");
		  break;
   }
}

void mainMenu() {

   lcd_key = read_LCD_buttons();   // read the buttons


   if (lcd_key != last_lcd_key) {  // Navigate on Main Menu 

     last_lcd_key = lcd_key;
    
     switch (lcd_key){               
   
         case btnUP:{            
              displayMainMenu(curr_menu_main);
              break;
         }
         case btnDOWN:{
              displayMainMenu(curr_menu_main);
              break;
         }    
         case btnLEFT:{   //1
               
               curr_menu_main = curr_menu_main - 1;
               if (curr_menu_main <= 0) {
                curr_menu_main = no_menus[0];
               }
               displayMainMenu(curr_menu_main);
               break;
         }
         case btnRIGHT:{   //2
          
               curr_menu_main = curr_menu_main + 1;
               if (curr_menu_main > no_menus[0]) {
                curr_menu_main = 1;
               }
               displayMainMenu(curr_menu_main);
               break;
         }
         case btnSELECT:{
			 if (curr_menu_main == 5) {
				 selected_menu = 99;
				 last_lcd_key = 5;
				 lcd.clear();
			 } else {
				 selected_menu = curr_menu_main;
			 }
			  
			  
			  curr_menu_sub1 = 1;
			  curr_menu_sub2 = 1;
			  curr_menu_sub3 = 1;
			  curr_menu_sub4 = 1;
              break;
         }
         case btnNONE:{
              displayMainMenu(curr_menu_main);
              break;
         }
  
         
     }

	 
     
   } else {
	   
    // do not press key
    // do not change menu
   }

 delay(5);
}

void displaySubMenu1(int menu) {
   lcd.clear();
   lcd.setCursor(0,0);             // set the LCD cursor   position 
   lcd.print("Date Time");  // print a simple message on the LCD
   lcd.setCursor(0,1);             // move to the begining of the second line
   switch (menu) {
    case 1:
          lcd.print(" 1. Setting    >");
          break;
    case 2:
          lcd.print("<2. RTC Temp   >");
          break;
    case 3:
          lcd.print("<3. Main Menu   ");
          break;
   }
}

void displaySubMenu2(int menu) {
   lcd.clear();
   lcd.setCursor(0,0);             // set the LCD cursor   position 
   lcd.print("Scheduling");  // print a simple message on the LCD
   lcd.setCursor(0,1);             // move to the begining of the second line
   switch (menu) {
    case 1:
          lcd.print(" 1. Program 1  >");
          break;
    case 2:
          lcd.print("<2. Program 2  >");
          break;
    case 3:
          lcd.print("<3. Main Menu   ");
          break;
   }
}

void displaySubMenu21() {
 lcd.clear();
 lcd.setCursor(0, 0);  lcd.write("#");
 lcd.setCursor(8, 0);  lcd.write("#");
 lcd.setCursor(13,0);  lcd.write(":");
 lcd.setCursor(0, 1);  lcd.write("#");
 lcd.setCursor(5, 1);  lcd.write("d");
 lcd.setCursor(8, 1);  lcd.write("#");
 lcd.setCursor(12,1);  lcd.write("h");
 lcd.setCursor(15,1);  lcd.write("m");

 for (int i = 0; i < max_menu_sub21;i++) {
	
	lcd.setCursor(menu_sub2x[i][0], menu_sub2x[i][1]);
	if (i < 7) {
		String str = dayOfWeek[i][curr_val_sub2x[i]];
		lcd.print(str);
		
	} else {
		lcd.print(curr_val_sub2x[i]);
	}
	
	 
 }
 
 
}

void displaySubMenu22() {
 lcd.clear();
 lcd.setCursor(0,0);
 lcd.write("d=");  
 
 for(int n = 0; n < 7; n++)
  {
    lcd.setCursor((n*2)+2,0);
    lcd.write(B01111110);   // ->
    lcd.write(n);
  }  

 lcd.setCursor(0,1);
  lcd.write("f=");lcd.write(B01111110); lcd.write("10d"); lcd.write("  ");
  lcd.write("t=");lcd.write(B01111110); lcd.write("1");  lcd.write(":");lcd.write(B01111110); lcd.write("10");
}

void displaySubMenu3(int menu) {
   lcd.clear();
   lcd.setCursor(0,0);             // set the LCD cursor   position 
   lcd.print("Sensors");  // print a simple message on the LCD
   lcd.setCursor(0,1);             // move to the begining of the second line
   switch (menu) {
    case 1:
          lcd.print(" 1. Rain       >");
          break;
    case 2:
          lcd.print("<2. Light      >");
          break;
    case 3:
          lcd.print("<3. Main Menu   ");
          break;
   }
}

void displaySubMenu4(int menu) {
   lcd.clear();
   lcd.setCursor(0,0);             // set the LCD cursor   position 
   lcd.print("Turn On|Off");  // print a simple message on the LCD
   lcd.setCursor(0,1);             // move to the begining of the second line
   switch (menu) {
    case 1:
          lcd.print(" 1. Water On   >");
          break;
    case 2:
          lcd.print("<2. Water Off  >");
          break;
    case 3:
          lcd.print("<3. Main Menu   ");
          break;
   }
}

void subMenu1() {

   lcd_key = read_LCD_buttons();   // read the buttons


   if ((lcd_key != last_lcd_key) ) {


    
   
     last_lcd_key = lcd_key;
    
     switch (lcd_key){               // depending on which button was pushed, we perform an action
   
         case btnUP:{             //  push button "RIGHT" and show the word on the screen
              //lcd.print("RIGHT ");
              break;
         }
         case btnDOWN:{
              //lcd.print("LEFT   "); //  push button "LEFT" and show the word on the screen
              break;
         }    
         case btnLEFT:{   //1
               
               curr_menu_sub1 = curr_menu_sub1 - 1;
               if (curr_menu_sub1 <= 0) {
                curr_menu_sub1 = no_menus[1];
               }
              
               break;
         }
         case btnRIGHT:{   //2
          
               curr_menu_sub1 = curr_menu_sub1 + 1;
               if (curr_menu_sub1 > no_menus[1]) {
                curr_menu_sub1 = 1;
               }
              
               break;
         }
         case btnSELECT:{
			 switch (curr_menu_sub1) {
			 case 1:

				 break;
			 case 2:

				 break;
			 default:
				 selected_menu = 0; break;
			 }
         }
         case btnNONE:{
              //lcd.print("NONE  ");  //  No action  will show "None" on the screen
              break;
         }
  
         
     }

     
     displaySubMenu1(curr_menu_sub1);

     
   } else {
    // do not press key
    // do not change menu
   }

 delay(5);
}

void subMenu2() {

	lcd_key = read_LCD_buttons();   // read the buttons


	if ((lcd_key != last_lcd_key)) {




		last_lcd_key = lcd_key;

		switch (lcd_key) {               // depending on which button was pushed, we perform an action

		case btnUP: {             //  push button "RIGHT" and show the word on the screen
								  //lcd.print("RIGHT ");
			displaySubMenu2(curr_menu_sub2);
			break;
		}
		case btnDOWN: {
			//lcd.print("LEFT   "); //  push button "LEFT" and show the word on the screen
			displaySubMenu2(curr_menu_sub2);
			break;
		}
		case btnLEFT: {   //1

			curr_menu_sub2 = curr_menu_sub2 - 1;
			if (curr_menu_sub2 <= 0) {
				curr_menu_sub2 = no_menus[2];
			}
			displaySubMenu2(curr_menu_sub2);
			break;
		}
		case btnRIGHT: {   //2

			curr_menu_sub2 = curr_menu_sub2 + 1;
			if (curr_menu_sub2 > no_menus[2]) {
				curr_menu_sub2 = 1;
			}
			displaySubMenu2(curr_menu_sub2);
			break;
		}
		case btnSELECT: {
			switch (curr_menu_sub2) {
			case 1:
				selected_menu = 21;
				break;
			case 2:
				selected_menu = 22;
				break;
			default:
				selected_menu = 0; break;
			}
			break;
		}
		case btnNONE: {
			//lcd.print("NONE  ");  //  No action  will show "None" on the screen
			displaySubMenu2(curr_menu_sub2);
			break;
		}


		}


		


	}
	else {
		// do not press key
		// do not change menu
	}

	delay(5);
}

void subMenu21() {
	lcd_key = read_LCD_buttons();   // read the buttons

	if ((lcd_key != last_lcd_key)) {

		last_lcd_key = lcd_key;

		switch (lcd_key) {        

		case btnUP: {             
			
			if (curr_menu_sub21 < 7) {
				if (curr_val_sub2x[curr_menu_sub21] == 0) {
					curr_val_sub2x[curr_menu_sub21] = 1;
				} else {
					curr_val_sub2x[curr_menu_sub21] = 0;
				}

			} else {

				curr_val_sub2x[curr_menu_sub21] --;

				if (curr_val_sub2x[curr_menu_sub21] < 0) {	                           // choice more than max value
					curr_val_sub2x[curr_menu_sub21] = menu_sub2x[curr_menu_sub21][3];  // back to max
					//curr_val_sub2x[curr_menu_sub21] = 12;
				}

			}

			break;
		}
		case btnDOWN: {
			
			
			if (curr_menu_sub21 < 7) {
				if (curr_val_sub2x[curr_menu_sub21] == 0) {
					curr_val_sub2x[curr_menu_sub21] = 1;
				} else {
					curr_val_sub2x[curr_menu_sub21] = 0;
				}
				
			} else {

				curr_val_sub2x[curr_menu_sub21] ++;

				if (curr_val_sub2x[curr_menu_sub21] > menu_sub2x[curr_menu_sub21][3]) {	// choice more than max value
					curr_val_sub2x[curr_menu_sub21] = 0;								// back to 0
				}
				
			}

			break;
		}
		case btnLEFT: {   //1

			curr_menu_sub21 = curr_menu_sub21 - 1;
			if (curr_menu_sub21 < 0) {
				curr_menu_sub21 = max_menu_sub21-1;
			}
			
			break;
		}
		case btnRIGHT: {   //2
			
			curr_menu_sub21 = curr_menu_sub21 + 1;
			if (curr_menu_sub21 > (max_menu_sub21-1)) {
				curr_menu_sub21 = 0;
			}
			
			break;
		}
		case btnSELECT: {
			eepromWrite(0, 0, curr_val_sub2x, 12);
			selected_menu = 2;
			break;
		}
		case btnNONE: {
			
			break;
		}


		}

		displaySubMenu21();

	} else {

		// show blink text
		if (blink_val_sub2x) {
			lcd.setCursor(menu_sub2x[curr_menu_sub21][0], menu_sub2x[curr_menu_sub21][1]);
			for (int i = 0; i < menu_sub2x[curr_menu_sub21][2]; i++) { lcd.write(" "); }
			
			blink_val_sub2x = false;
			delay(95);

		} else {
			lcd.setCursor(menu_sub2x[curr_menu_sub21][0], menu_sub2x[curr_menu_sub21][1]);
			
			if (curr_menu_sub21 < 7) {
				String str = dayOfWeek[curr_menu_sub21][curr_val_sub2x[curr_menu_sub21]];
				lcd.print(str);

			} else {
				lcd.print(curr_val_sub2x[curr_menu_sub21]);
			}

			blink_val_sub2x = true;
			delay(95);

		}
	}
	

	delay(5);
}

void subMenu22() {
	lcd_key = read_LCD_buttons();   // read the buttons


	if ((lcd_key != last_lcd_key)) {




		last_lcd_key = lcd_key;

		switch (lcd_key) {               // depending on which button was pushed, we perform an action

		case btnUP: {             //  push button "RIGHT" and show the word on the screen
								  //lcd.print("RIGHT ");
			break;
		}
		case btnDOWN: {
			//lcd.print("LEFT   "); //  push button "LEFT" and show the word on the screen
			break;
		}
		case btnLEFT: {   //1


			break;
		}
		case btnRIGHT: {   //2


			break;
		}
		case btnSELECT: {
			selected_menu = 2;
			break;
		}
		case btnNONE: {
			//lcd.print("NONE  ");  //  No action  will show "None" on the screen
			break;
		}


		}


		displaySubMenu22();


	}
	else {
		// do not press key
		// do not change menu
	}

	delay(5);
}


void subMenu3() {

	lcd_key = read_LCD_buttons();   // read the buttons


	if ((lcd_key != last_lcd_key)) {




		last_lcd_key = lcd_key;

		switch (lcd_key) {               // depending on which button was pushed, we perform an action

		case btnUP: {             //  push button "RIGHT" and show the word on the screen
								  //lcd.print("RIGHT ");
			break;
		}
		case btnDOWN: {
			//lcd.print("LEFT   "); //  push button "LEFT" and show the word on the screen
			break;
		}
		case btnLEFT: {   //1

			curr_menu_sub3 = curr_menu_sub3 - 1;
			if (curr_menu_sub3 <= 0) {
				curr_menu_sub3 = no_menus[3];
			}

			break;
		}
		case btnRIGHT: {   //2

			curr_menu_sub3 = curr_menu_sub3 + 1;
			if (curr_menu_sub3 > no_menus[3]) {
				curr_menu_sub3 = 1;
			}

			break;
		}
		case btnSELECT: {
			switch (curr_menu_sub3) {
			case 1:

				break;
			case 2:

				break;
			default:
				selected_menu = 0; break;
			}
			break;
		}
		case btnNONE: {
			//lcd.print("NONE  ");  //  No action  will show "None" on the screen
			break;
		}


		}


		displaySubMenu3(curr_menu_sub3);


	}
	else {
		// do not press key
		// do not change menu
	}

	delay(5);
}

void subMenu4() {

	lcd_key = read_LCD_buttons();   // read the buttons


	if ((lcd_key != last_lcd_key)) {




		last_lcd_key = lcd_key;

		switch (lcd_key) {               // depending on which button was pushed, we perform an action

		case btnUP: {             //  push button "RIGHT" and show the word on the screen
								  //lcd.print("RIGHT ");
			break;
		}
		case btnDOWN: {
			//lcd.print("LEFT   "); //  push button "LEFT" and show the word on the screen
			break;
		}
		case btnLEFT: {   //1

			curr_menu_sub4 = curr_menu_sub4 - 1;
			if (curr_menu_sub4 <= 0) {
				curr_menu_sub4 = no_menus[4];
			}

			break;
		}
		case btnRIGHT: {   //2

			curr_menu_sub4 = curr_menu_sub4 + 1;
			if (curr_menu_sub4 > no_menus[4]) {
				curr_menu_sub4 = 1;
			}

			break;
		}
		case btnSELECT: {
			switch (curr_menu_sub4) {
			case 1:
				digitalWrite(waterPIN, HIGH);
				break;
			case 2:
				digitalWrite(waterPIN, LOW);
				break;
			default:
				selected_menu = 0; break;
			}
			break;
		}
		case btnNONE: {
			//lcd.print("NONE  ");  //  No action  will show "None" on the screen
			break;
		}


		}


		displaySubMenu4(curr_menu_sub4);


	}
	else {
		// do not press key
		// do not change menu
	}

	delay(5);
}
