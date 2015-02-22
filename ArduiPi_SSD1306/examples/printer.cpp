#include "ArduiPi_SSD1306.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

// Instantiate the display
Adafruit_SSD1306 display;

int main(int argc, char **argv)
{
  float soil = 65.5;
  float temp = 5.5;

	// I2C change parameters to fit to your LCD
	if ( !display.init(OLED_I2C_RESET,OLED_ADAFRUIT_I2C_128x64) )
		exit(EXIT_FAILURE);

	display.begin();
	
	// init done
	display.clearDisplay();   // clears the screen and buffer

  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("     XBee Messung\n");
  display.drawLine(0, 10, display.width()-1, 10, WHITE);
  display.setCursor(0,15);
  display.printf("Bodenfeuchte: %4.1f %%\n", soil);
  display.printf("Temperatur:   %4.1f C\n\n\n", temp);
  if(soil > 50) {
    display.print("--> Feucht genug\n");
  } else {
    display.print("--> Zu Trocken! \n");
  }
  
  display.display();

  // Free PI GPIO ports
  display.close();
}