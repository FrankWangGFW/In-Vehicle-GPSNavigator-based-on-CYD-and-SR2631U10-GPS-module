#include <TFT_eSPI.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <math.h>

// 包含NISSAN标志图片头文件
#include "nissan_logo.h"
// 包含GPS黑白图标头文件
#include "satellite_icon_bw.h"

// ======================
// Hardware Configuration
// ======================
#define GPS_BAUD_RATE 38400
#define GPS_RX_PIN 22
#define GPS_TX_PIN 27
#define TFT_ROTATION 1  // Landscape mode

// ======================
// Screen Configuration
// ======================
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define UPDATE_INTERVAL 1000  
#define MIN_DIFF 0.1         
#define SEARCH_UPDATE_INTERVAL 200  

// ======================
// Left Panel Configuration
// ======================
#define SECTION1_TOP 25
#define SECTION1_BOTTOM 66
#define SECTION2_TOP 66
#define SECTION2_BOTTOM 107
#define SECTION3_TOP 107
#define SECTION3_BOTTOM 148
#define SECTION4_TOP 158
#define SECTION4_BOTTOM 199
#define SECTION5_TOP 199
#define SECTION5_BOTTOM 240
#define SECTION_LEFT 160
#define SECTION_RIGHT 320
#define SECTION_WIDTH (SECTION_RIGHT - SECTION_LEFT)

// ======================
// Color Definitions
// ======================
#define COLOR_BACKGROUND 0x0000   // Pure Black BackGround
#define COLOR_HEADER_BG  0xFFFF   // White Header Background
#define COLOR_HEADER_TEXT 0x0000  // Header Black Text
#define COLOR_TEXT       0x07E0   // Green Text
#define COLOR_LINE       0x07E0   // Green Line
#define COLOR_SEARCHING  0xFFE0   // Yellow Searching Text

// ======================
// Global Objects
// ======================
TFT_eSPI tft = TFT_eSPI();
HardwareSerial GPS_Serial(1);
TinyGPSPlus gps;

// ======================
// GPS Data Storage
// ======================
float altitude = 0.0;
float latitude = 0.0;
float longitude = 0.0;
float speed = 0.0;
float heading = 0.0;
int satellites = 0;
int accuracy=0;
bool gpsValid = false;  // Default GPS is invalid
unsigned long lastUpdateTime = 0;
unsigned long lastSearchUpdateTime = 0;

// System state variables - Fix: Ensure global variables are used correctly      
bool inSearchingScreen = false; // Whether on the search interface - global variable
bool uiInitialized = false;  // whether the UI has been initialized

// Basic Cache
float last_latitude = -999.0;
float last_longitude = -999.0;
float last_speed = -999.0;
float last_heading = -999.0;
int last_satellites = -1;
bool last_gpsValid = false;
int last_accuracy = -1;
int last_hour = -1;
int last_minute = -1;
int last_second = -1;
bool timeInitialized = false;

// Exclusive cache for the right-side columns 1-3
char last_mainDir[8] = "";
char last_subDir[8] = "";
float last_angleDiff = -1.0;
bool last_isFrozen = false;

// whether the right panel is in Frozen state
bool isFrozen = false;
float frozenHeading = 0.0;
char frozenMainDir[8] = "";
char frozenSubDir[8] = "";
float frozenAngleDiff = 0.0;

// ======================
// Direction Calculation
// ======================
// ======================
// Update Left Panel
// ======================
/**
 * Updates the left panel of the GPS navigation display
 * Shows latitude, longitude, altitude, and speed information
 */
void updateLeftPanel() {
  int textColor = COLOR_TEXT;  // Set text color for display
  
  // Update latitude and longitude if they have changed significantly or GPS validity changed
  if (fabs(latitude - last_latitude) > 0.001 || fabs(longitude - last_longitude) > 0.001 || gpsValid != last_gpsValid) {
    // Clear the areas where latitude and longitude will be displayed
    tft.fillRect(20, 40, 100, 20, COLOR_BACKGROUND);
    tft.fillRect(20, 65, 100, 20, COLOR_BACKGROUND);
    
    // Set text properties and display latitude
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    
    char latBuf[10];
    sprintf(latBuf, "%.3f", latitude);  // Format latitude to 3 decimal places
    tft.setCursor(20, 40);
    tft.print(latBuf);
    
    // Set text properties and display longitude
    char lonBuf[10];
    sprintf(lonBuf, "%.3f", longitude);  // Format longitude to 3 decimal places
    tft.setCursor(20, 65);
    tft.print(lonBuf);
    
    // Update last known values for next comparison
    last_latitude = latitude;
    last_longitude = longitude;
  }
  
  // Static variable to track last displayed altitude
  static float last_altitude = -999.0;
  // Update altitude if it has changed significantly
  if (fabs(altitude - last_altitude) > MIN_DIFF) {
    // Clear the area where altitude will be displayed
    tft.fillRect(50, 95, 70, 20, COLOR_BACKGROUND);
    
    // Set text properties
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    
    // Ensure altitude is non-negative and format for display
    int displayAltitude = (altitude < 0) ? 0 : round(altitude);
    char altBuf[6];
    sprintf(altBuf, "%4d", displayAltitude);  // Format as 4-digit number
    tft.setCursor(50, 95);
    tft.print(altBuf);
    
    // Update last known altitude for next comparison
    last_altitude = altitude;
  }
  
  // Update speed if it has changed significantly
  if (fabs(speed - last_speed) > MIN_DIFF) {
    // Display speed as 0 if under 4 units, otherwise rounded value
    int displaySpeed = (speed < 4) ? 0 : round(speed);
    int speedColor = COLOR_TEXT;
    
    // Clear the area where speed will be displayed
    tft.fillRect(0, 160, 150, 35, COLOR_BACKGROUND);
    
    // Format speed for display and calculate text width
    char speedBuf[4];
    sprintf(speedBuf, "%d", displaySpeed);
    int speedWidth = tft.textWidth(speedBuf);
    
    // Calculate centered position for speed display
    int speedX = 44 - (speedWidth / 2);
    
    // Ensure speed text stays within bounds
    if (speedX < 0) speedX = 0;
    if (speedX + speedWidth > 150) speedX = 150 - speedWidth;
    
    // Display speed with large text size
    tft.setTextColor(speedColor);
    tft.setTextSize(5);
    tft.setCursor(speedX, 160);
    tft.print(speedBuf);
    
    // Update last known speed for next comparison
    last_speed = speed;
  }
  
  // Update last GPS validity state
  last_gpsValid = gpsValid;
}
  
// ======================
// Update Right Panel
// ======================
/**
 * Updates the right panel of the GPS navigation display
 * Shows direction information, time, and date
 */
void updateRightPanel() {
  int textColor = COLOR_TEXT;  // Set text color for display
  // Display speed as 0 if under 4 units, otherwise rounded value
  int displaySpeed = (speed < 4) ? 0 : round(speed);
  bool needRedraw1_3 = false;  // Flag to indicate if direction section needs redrawing
  
  // Handle heading freezing logic when speed is low
  if (displaySpeed < 4 && !isFrozen) {
    isFrozen = true;  // Freeze current heading
    frozenHeading = heading;
    getDirection(frozenHeading, frozenAngleDiff, frozenMainDir, frozenSubDir);  // Calculate frozen direction
    needRedraw1_3 = true;  // Need to redraw direction section
  } else if (displaySpeed >= 4 && isFrozen) {
    isFrozen = false;  // Unfreeze heading when speed increases
    needRedraw1_3 = true;  // Need to redraw direction section
  }
  
  // Determine current heading (frozen or live)
  float currentHeading = isFrozen ? frozenHeading : heading;
  // Validate heading value
  if (currentHeading < 0 || currentHeading > 360 || isnan(currentHeading)) {
    currentHeading = 0;  // Set to 0 if invalid
  }
  
  // Variables to store direction information
  float angleDiff;
  char mainDir[8] = "";
  char subDir[8] = "";
  
  // Get direction information based on current heading state
  if (isFrozen) {
    // Use frozen direction values
    angleDiff = frozenAngleDiff;
    strcpy(mainDir, frozenMainDir);
    strcpy(subDir, frozenSubDir);
  } else {
    // Calculate fresh direction values
    getDirection(currentHeading, angleDiff, mainDir, subDir);
  }
  
  // Check if direction information has changed significantly
  if (fabs(currentHeading - last_heading) > MIN_DIFF || 
      strcmp(mainDir, last_mainDir) != 0 || 
      strcmp(subDir, last_subDir) != 0 || 
      fabs(angleDiff - last_angleDiff) > MIN_DIFF ||
      isFrozen != last_isFrozen) {
    
    needRedraw1_3 = true;
    bool isExactDirection = (subDir[0] == '\0');  // Check if only main direction is needed
    
    if (isExactDirection) {
      // Clear larger area for single direction display
      tft.fillRect(SECTION_LEFT+5, SECTION1_TOP, SECTION_WIDTH, SECTION2_BOTTOM - SECTION1_TOP, COLOR_BACKGROUND);
      
      // Display main direction with large text size
      tft.setTextColor(textColor);
      tft.setTextSize(4);
      int mainDirWidth = tft.textWidth(mainDir);
      // Calculate centered position
      int mainDirX = SECTION_LEFT + (SECTION_WIDTH - mainDirWidth) / 2;
      int mainDirY = SECTION1_TOP + (SECTION2_BOTTOM - SECTION1_TOP - 20) / 2;
      tft.setCursor(mainDirX+5, mainDirY);
      tft.print(mainDir);
      
      // Clear angle difference area
      tft.fillRect(SECTION_LEFT+5, SECTION3_TOP, SECTION_WIDTH, SECTION3_BOTTOM - SECTION3_TOP, COLOR_BACKGROUND);
    } else {
      // Clear main direction area
      tft.fillRect(SECTION_LEFT+5, SECTION1_TOP, SECTION_WIDTH, SECTION1_BOTTOM - SECTION1_TOP, COLOR_BACKGROUND);
      // Display main direction
      tft.setTextColor(textColor);
      tft.setTextSize(3);
      int mainDirWidth = tft.textWidth(mainDir);
      // Calculate centered position
      int mainDirX = SECTION_LEFT + (SECTION_WIDTH - mainDirWidth) / 2;
      int mainDirY = SECTION1_TOP + (SECTION1_BOTTOM - SECTION1_TOP - 16) / 2;
      tft.setCursor(mainDirX+5, mainDirY);
      tft.print(mainDir);
      
      // Clear secondary direction area
      tft.fillRect(SECTION_LEFT+5, SECTION2_TOP, SECTION_WIDTH, SECTION2_BOTTOM - SECTION2_TOP, COLOR_BACKGROUND);
      // Display secondary direction
      tft.setTextColor(textColor);
      tft.setTextSize(3);
      int subDirWidth = tft.textWidth(subDir);
      // Calculate centered position
      int subDirX = SECTION_LEFT + (SECTION_WIDTH - subDirWidth) / 2;
      int subDirY = SECTION2_TOP + (SECTION2_BOTTOM - SECTION2_TOP - 16) / 2;
      tft.setCursor(subDirX+5, subDirY);
      tft.print(subDir);
      
      // Clear angle difference area
      tft.fillRect(SECTION_LEFT+5, SECTION3_TOP, SECTION_WIDTH, SECTION3_BOTTOM - SECTION3_TOP, COLOR_BACKGROUND);
    }
    
    // Display angle difference
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    char diffBuf[4];
    sprintf(diffBuf, "%.0f", angleDiff);  // Format as integer
    int diffWidth = tft.textWidth(diffBuf);
    // Calculate centered position
    int diffX = SECTION_LEFT + (SECTION_WIDTH - diffWidth) / 2;
    int diffY = SECTION3_TOP + (SECTION3_BOTTOM - SECTION3_TOP - 16) / 2;
    tft.setCursor(diffX+5, diffY);
    tft.print(diffBuf);
    // Display degree symbol
    tft.setCursor(diffX + 30, diffY - 5);
    tft.setTextSize(1);
    tft.print('o');
    
    // Update last known values for next comparison
    last_heading = currentHeading;
    strcpy(last_mainDir, mainDir);
    strcpy(last_subDir, subDir);
    last_angleDiff = angleDiff;
    last_isFrozen = isFrozen;
  }
  
  // Update time if GPS time is valid
  if (gps.time.isValid()) {
    // Convert UTC time to Beijing time
    int bjHour, bjMinute, bjSecond;
    utcToBeijingTime(gps.time.hour(), gps.time.minute(), gps.time.second(), bjHour, bjMinute, bjSecond);
    
    // Set text properties for time display
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    
    // Format full time string
    char fullTimeBuf[9];
    sprintf(fullTimeBuf, "%02d:%02d:%02d", bjHour, bjMinute, bjSecond);
    int fullTimeWidth = tft.textWidth(fullTimeBuf);
    // Calculate centered position
    int timeX = SECTION_LEFT + (SECTION_WIDTH - fullTimeWidth) / 2 + 6;
    int timeY = SECTION4_TOP + 18;
    
    // Initial time setup
    if (!timeInitialized) {
      // Clear time area and display full time
      tft.fillRect(SECTION_LEFT + 35, SECTION4_TOP + 5, 85, 30, COLOR_BACKGROUND);
      tft.setCursor(timeX, timeY);
      tft.print(fullTimeBuf);
      
      // Update initialization flag and last time values
      timeInitialized = true;
      last_hour = bjHour;
      last_minute = bjMinute;
      last_second = bjSecond;
    } 
    // Update full time when hour or minute changes
    else if (bjHour != last_hour || bjMinute != last_minute) {
      tft.fillRect(SECTION_LEFT + 35, SECTION4_TOP + 5, 85, 30, COLOR_BACKGROUND);
      tft.setCursor(timeX, timeY);
      tft.print(fullTimeBuf);
      
      last_hour = bjHour;
      last_minute = bjMinute;
      last_second = bjSecond;
    } 
    // Only update seconds when only seconds change
    else if (bjSecond != last_second) {
      // Format and display only seconds
      char secBuf[3];
      sprintf(secBuf, "%02d", bjSecond);
      int secWidth = tft.textWidth(secBuf);
      
      // Calculate position for seconds only
      int secX = timeX + tft.textWidth(fullTimeBuf) - secWidth;
      
      // Clear only seconds area and update
      tft.fillRect(secX - 5, timeY - 2, secWidth + 10, 35, COLOR_BACKGROUND);
      tft.setCursor(secX, timeY);
      tft.print(secBuf);
      
      last_second = bjSecond;
    }
  }
  
  // Update date if GPS date and time are valid
  if (gps.date.isValid() && gps.time.isValid()) {
    // Convert UTC date to Beijing date
    int bjYear, bjMonth, bjDay;
    utcToBeijingDate(gps.date.year(), gps.date.month(), gps.date.day(), bjYear, bjMonth, bjDay);
    
    // Static variables to track last displayed date
    static int last_bjYear = -1, last_bjMonth = -1, last_bjDay = -1;
    // Update date if it has changed
    if (bjYear != last_bjYear || bjMonth != last_bjMonth || bjDay != last_bjDay) {
      // Clear date area
      tft.fillRect(SECTION_LEFT+5, SECTION5_TOP, SECTION_WIDTH, SECTION5_BOTTOM - SECTION5_TOP, COLOR_BACKGROUND);
      // Set text properties
      tft.setTextColor(textColor);
      tft.setTextSize(2);
      
      // Format date string
      char dateBuf[11];
      sprintf(dateBuf, "%04d-%02d-%02d", bjYear, bjMonth, bjDay);
      int dateWidth = tft.textWidth(dateBuf);
      // Calculate centered position
      int dateX = SECTION_LEFT + (SECTION_WIDTH - dateWidth) / 2;
      int dateY = SECTION5_TOP + (SECTION5_BOTTOM - SECTION5_TOP - 16) / 2;
      // Display date
      tft.setCursor(dateX+5, dateY);
      tft.print(dateBuf);
      
      // Update last known date values for next comparison
      last_bjYear = bjYear;
      last_bjMonth = bjMonth;
      last_bjDay = bjDay;
    }
  }
}
/**
 * Calculates compass direction information based on a given heading angle
 * 
 * @param heading Input heading angle in degrees (0-360, where 0° is North, 90° is East, 180° is South, 270° is West)
 * @param angleDiff Output parameter - returns the minimum angle difference from the main direction
 * @param mainDir Output parameter - returns the main direction (NORTH, EAST, SOUTH, WEST)
 * @param subDir Output parameter - returns the secondary direction (if angle difference > 10°)
 */
void getDirection(float heading, float &angleDiff, char *mainDir, char *subDir) {
  // Validate heading angle (must be a valid number between 0 and 360 degrees)
  if (isnan(heading) || heading < 0 || heading > 360) {
    strcpy(mainDir, "NONE");  // Set main direction to NONE for invalid heading
    strcpy(subDir, "");       // Clear secondary direction
    angleDiff = 0;            // Set angle difference to 0
    return;                   // Exit function early
  }
  
  // Ensure heading is within 0-360 degree range (handles negative values)
  heading = fmod(heading + 360, 360);
  
  // Define the four cardinal directions with their respective angles
  const struct {
    float angle;    // Angle in degrees
    const char *name; // Direction name
  } directions[] = {
    {0.0, "NORTH"},  // North at 0°
    {90.0, "EAST"},  // East at 90°
    {180.0, "SOUTH"}, // South at 180°
    {270.0, "WEST"}   // West at 270°
  };
  
  // Initialize variables to find the closest main direction
  int closestIndex = 0;    // Index of the closest direction
  float minDiff = 360.0;   // Minimum angle difference (initialized to maximum possible value)
  
  // Find the closest cardinal direction
  for (int i = 0; i < 4; i++) {
    // Calculate absolute angle difference
    float diff = fabs(heading - directions[i].angle);
    // Account for 360° wrap-around (e.g., 350° to 10° is 20° difference, not 340°)
    diff = fmin(diff, 360 - diff);
    
    // Update minimum difference and closest index if current direction is closer
    if (diff < minDiff) {
      minDiff = diff;
      closestIndex = i;
    }
  }
  
  // Set output parameters for main direction and angle difference
  angleDiff = minDiff;
  strcpy(mainDir, directions[closestIndex].name);
  subDir[0] = '\0';  // Initialize secondary direction as empty string
  
  // If angle difference is less than 10°, only main direction is needed
  if (minDiff < 10.0) return;
  
  // Calculate indices for adjacent directions (handle wrap-around)
  int nextIndex = (closestIndex + 1) % 4;       // Next direction index (clockwise)
  int prevIndex = (closestIndex - 1 + 4) % 4;   // Previous direction index (counter-clockwise, +4 prevents negative)
  
  // Calculate angle difference to next direction
  float nextDiff = fabs(heading - directions[nextIndex].angle);
nextDiff = fmin(nextDiff, 360 - nextDiff);
  
  // Calculate angle difference to previous direction
  float prevDiff = fabs(heading - directions[prevIndex].angle);
  prevDiff = fmin(prevDiff, 360 - prevDiff);
  
  // Determine secondary direction based on which adjacent direction is closer
  if (nextDiff < prevDiff) {
    strcpy(subDir, directions[nextIndex].name);  // Secondary direction is next direction
  } else {
    strcpy(subDir, directions[prevIndex].name);  // Secondary direction is previous direction
  }
}


// ======================
// UTC to Beijing Time
//Standard Time in China is UTC+8
//Find your local time zone offset from UTC and adjust the time accordingly.
// ======================
void utcToBeijingTime(int utcHour, int utcMinute, int utcSecond, 
                     int &bjHour, int &bjMinute, int &bjSecond) {
  bjHour = utcHour + 8;
  bjMinute = utcMinute;
  bjSecond = utcSecond;
  
  if (bjHour >= 24) bjHour -= 24;
}

// ======================
// UTC to Beijing Date
//Here is the code to convert UTC date to Beijing date.
//Find your local time zone offset from UTC and adjust the date accordingly.
// ======================
void utcToBeijingDate(int utcYear, int utcMonth, int utcDay, 
                     int &bjYear, int &bjMonth, int &bjDay) {
  bjYear = utcYear;
  bjMonth = utcMonth;
  bjDay = utcDay;
  
  if (gps.time.hour() >= 16) {
    bjDay++;
    
    if ((bjMonth == 4 || bjMonth == 6 || bjMonth == 9 || bjMonth == 11) && bjDay > 30) {
      bjDay = 1;
      bjMonth++;
    } else if (bjMonth == 2) {
      bool isLeapYear = (bjYear % 4 == 0 && bjYear % 100 != 0) || (bjYear % 400 == 0);
      if ((isLeapYear && bjDay > 29) || (!isLeapYear && bjDay > 28)) {
        bjDay = 1;
        bjMonth++;
      }
    } else if (bjDay > 31) {
      bjDay = 1;
      bjMonth++;
    }
    
    if (bjMonth > 12) {
      bjMonth = 1;
      bjYear++;
    }
  }
}

// ======================
// Draw the centerline
// ======================
void drawCenterLine() {
  tft.drawLine(160, 26, 160, 240, COLOR_LINE);
  tft.drawLine(161, 26, 161, 240, COLOR_LINE);
}

// ======================
// Initialize UI
// ======================
void initUI() {
  tft.fillScreen(COLOR_BACKGROUND);
   // ======================
  // static Display for The lines
  // ======================
  drawCenterLine();

  tft.drawLine(162, 148, 320, 148, COLOR_LINE);
  tft.drawLine(162, 149, 320, 149, COLOR_LINE);
  tft.drawLine(0, 125, 159, 125, COLOR_LINE);
  tft.drawLine(0, 126, 159, 126, COLOR_LINE);
  
  tft.setTextFont(1);
  tft.setTextWrap(false);

   // ======================
  // static Display for The Header
  // ======================

  tft.fillRect(0, 0, SCREEN_WIDTH, 25, COLOR_HEADER_BG);
  tft.setTextColor(COLOR_HEADER_TEXT);
  tft.setTextSize(2);
  tft.setCursor(10, 6);
  tft.print("PosiAccu:");
  

  tft.pushImage(295, 3, SATELLITE_ICON_BW_WIDTH, SATELLITE_ICON_BW_HEIGHT, satellite_icon_bw);


    // Initialize the precision display position and the 'm' character (120,6)
  tft.setTextSize(2);
  tft.setCursor(120, 6);
  tft.print("-");
  tft.setCursor(145, 6);
  tft.print("m");
  // Initialize satellite count display position (275,6)
  tft.setTextSize(2);
  tft.setCursor(270, 6);
  tft.print("--");
  
    // ======================
  // static Display for The Unit of measurement
  // ======================
  
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(120, 40);
  tft.print("N");
  tft.setCursor(120, 65);
  tft.print("E");
  tft.setCursor(20, 95);
  tft.print("HI:");
  tft.setCursor(120, 95);
  tft.print("m");
  tft.setTextSize(1);
  tft.setCursor(120, 210);
  tft.print("km/h");
  
  uiInitialized = true;
  timeInitialized = false;  // Reset time initialization flag
}

// ======================
// Update Header
// ======================
void updateHeaderAccuracy(){
   if (accuracy != last_accuracy) {
    tft.setTextColor(COLOR_HEADER_TEXT);
    tft.setTextSize(2);
    
    // ======================
    // Header修改部分 - 只修改这里
    // ======================
    // 在（120,6）处打印数字
    int accNumX = 120;
    int accNumY = 6;
    
    // 清除原有数字区域（考虑最大2位数字）
    tft.fillRect(accNumX - 2, accNumY - 2, 25, 20, COLOR_HEADER_BG);
    
    // 在指定位置显示精度
    tft.setCursor(accNumX, accNumY);
    if (accuracy > 0) {
      tft.print(accuracy);
    } else {
      tft.print("-");
    }

    last_accuracy = accuracy;
  }
}

void updateHeaderSatellites() {
  if (satellites != last_satellites) {
    tft.setTextColor(COLOR_HEADER_TEXT);
    tft.setTextSize(2);
    
    // ======================
    // Header修改部分 - 只修改这里
    // ======================
    // 在（265,6）处打印数字
    int satNumX = 270;
    int satNumY = 6;
    
    // 清除原有数字区域（考虑最大2位数字）
    tft.fillRect(satNumX - 2, satNumY - 2, 25, 20, COLOR_HEADER_BG);
    
    // 在指定位置显示卫星数量
    tft.setCursor(satNumX, satNumY);
    if (satellites > 0) {
      tft.print(satellites);
    } else {
      tft.print("-");
    }
    // ======================
    // Header修改结束
    // ======================
    
    last_satellites = satellites;
  }
}

void updateHeader() {
    updateHeaderAccuracy();
    updateHeaderSatellites();
}

// ======================
// Update Left Panel
// ======================
/**
 * Updates the left panel of the GPS navigation display
 * Shows latitude, longitude, altitude, and speed information
 */
void updateLeftPanel() {
  int textColor = COLOR_TEXT;  // Set text color for display
  
  // Update latitude and longitude if they have changed significantly or GPS validity changed
  if (fabs(latitude - last_latitude) > 0.001 || fabs(longitude - last_longitude) > 0.001 || gpsValid != last_gpsValid) {
    // Clear the areas where latitude and longitude will be displayed
    tft.fillRect(20, 40, 100, 20, COLOR_BACKGROUND);
    tft.fillRect(20, 65, 100, 20, COLOR_BACKGROUND);
    
    // Set text properties and display latitude
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    
    char latBuf[10];
    sprintf(latBuf, "%.3f", latitude);  // Format latitude to 3 decimal places
    tft.setCursor(20, 40);
    tft.print(latBuf);
    
    // Set text properties and display longitude
    char lonBuf[10];
    sprintf(lonBuf, "%.3f", longitude);  // Format longitude to 3 decimal places
    tft.setCursor(20, 65);
    tft.print(lonBuf);
    
    // Update last known values for next comparison
    last_latitude = latitude;
    last_longitude = longitude;
  }
  
  // Static variable to track last displayed altitude
  static float last_altitude = -999.0;
  // Update altitude if it has changed significantly
  if (fabs(altitude - last_altitude) > MIN_DIFF) {
    // Clear the area where altitude will be displayed
    tft.fillRect(50, 95, 70, 20, COLOR_BACKGROUND);
    
    // Set text properties
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    
    // Ensure altitude is non-negative and format for display
    int displayAltitude = (altitude < 0) ? 0 : round(altitude);
    char altBuf[6];
    sprintf(altBuf, "%4d", displayAltitude);  // Format as 4-digit number
    tft.setCursor(50, 95);
    tft.print(altBuf);
    
    // Update last known altitude for next comparison
    last_altitude = altitude;
  }
  
  // Update speed if it has changed significantly
  if (fabs(speed - last_speed) > MIN_DIFF) {
    // Display speed as 0 if under 4 units, otherwise rounded value
    int displaySpeed = (speed < 4) ? 0 : round(speed);
    int speedColor = COLOR_TEXT;
    
    // Clear the area where speed will be displayed
    tft.fillRect(0, 160, 150, 35, COLOR_BACKGROUND);
    
    // Format speed for display and calculate text width
    char speedBuf[4];
    sprintf(speedBuf, "%d", displaySpeed);
    int speedWidth = tft.textWidth(speedBuf);
    
    // Calculate centered position for speed display
    int speedX = 44 - (speedWidth / 2);
    
    // Ensure speed text stays within bounds
    if (speedX < 0) speedX = 0;
    if (speedX + speedWidth > 150) speedX = 150 - speedWidth;
    
    // Display speed with large text size
    tft.setTextColor(speedColor);
    tft.setTextSize(5);
    tft.setCursor(speedX, 160);
    tft.print(speedBuf);
    
    // Update last known speed for next comparison
    last_speed = speed;
  }
  
  // Update last GPS validity state
  last_gpsValid = gpsValid;
}
  
// ======================
// Update Right Panel
// ======================
/**
 * Updates the right panel of the GPS navigation display
 * Shows direction information, time, and date
 */
void updateRightPanel() {
  int textColor = COLOR_TEXT;  // Set text color for display
  // Display speed as 0 if under 4 units, otherwise rounded value
  int displaySpeed = (speed < 4) ? 0 : round(speed);
  bool needRedraw1_3 = false;  // Flag to indicate if direction section needs redrawing
  
  // Handle heading freezing logic when speed is low
  if (displaySpeed < 4 && !isFrozen) {
    isFrozen = true;  // Freeze current heading
    frozenHeading = heading;
    getDirection(frozenHeading, frozenAngleDiff, frozenMainDir, frozenSubDir);  // Calculate frozen direction
    needRedraw1_3 = true;  // Need to redraw direction section
  } else if (displaySpeed >= 4 && isFrozen) {
    isFrozen = false;  // Unfreeze heading when speed increases
    needRedraw1_3 = true;  // Need to redraw direction section
  }
  
  // Determine current heading (frozen or live)
  float currentHeading = isFrozen ? frozenHeading : heading;
  // Validate heading value
  if (currentHeading < 0 || currentHeading > 360 || isnan(currentHeading)) {
    currentHeading = 0;  // Set to 0 if invalid
  }
  
  // Variables to store direction information
  float angleDiff;
  char mainDir[8] = "";
  char subDir[8] = "";
  
  // Get direction information based on current heading state
  if (isFrozen) {
    // Use frozen direction values
    angleDiff = frozenAngleDiff;
    strcpy(mainDir, frozenMainDir);
    strcpy(subDir, frozenSubDir);
  } else {
    // Calculate fresh direction values
    getDirection(currentHeading, angleDiff, mainDir, subDir);
  }
  
  // Check if direction information has changed significantly
  if (fabs(currentHeading - last_heading) > MIN_DIFF || 
      strcmp(mainDir, last_mainDir) != 0 || 
      strcmp(subDir, last_subDir) != 0 || 
      fabs(angleDiff - last_angleDiff) > MIN_DIFF ||
      isFrozen != last_isFrozen) {
    
    needRedraw1_3 = true;
    bool isExactDirection = (subDir[0] == '\0');  // Check if only main direction is needed
    
    if (isExactDirection) {
      // Clear larger area for single direction display
      tft.fillRect(SECTION_LEFT+5, SECTION1_TOP, SECTION_WIDTH, SECTION2_BOTTOM - SECTION1_TOP, COLOR_BACKGROUND);
      
      // Display main direction with large text size
      tft.setTextColor(textColor);
      tft.setTextSize(4);
      int mainDirWidth = tft.textWidth(mainDir);
      // Calculate centered position
      int mainDirX = SECTION_LEFT + (SECTION_WIDTH - mainDirWidth) / 2;
      int mainDirY = SECTION1_TOP + (SECTION2_BOTTOM - SECTION1_TOP - 20) / 2;
      tft.setCursor(mainDirX+5, mainDirY);
      tft.print(mainDir);
      
      // Clear angle difference area
      tft.fillRect(SECTION_LEFT+5, SECTION3_TOP, SECTION_WIDTH, SECTION3_BOTTOM - SECTION3_TOP, COLOR_BACKGROUND);
    } else {
      // Clear main direction area
      tft.fillRect(SECTION_LEFT+5, SECTION1_TOP, SECTION_WIDTH, SECTION1_BOTTOM - SECTION1_TOP, COLOR_BACKGROUND);
      // Display main direction
      tft.setTextColor(textColor);
      tft.setTextSize(3);
      int mainDirWidth = tft.textWidth(mainDir);
      // Calculate centered position
      int mainDirX = SECTION_LEFT + (SECTION_WIDTH - mainDirWidth) / 2;
      int mainDirY = SECTION1_TOP + (SECTION1_BOTTOM - SECTION1_TOP - 16) / 2;
      tft.setCursor(mainDirX+5, mainDirY);
      tft.print(mainDir);
      
      // Clear secondary direction area
      tft.fillRect(SECTION_LEFT+5, SECTION2_TOP, SECTION_WIDTH, SECTION2_BOTTOM - SECTION2_TOP, COLOR_BACKGROUND);
      // Display secondary direction
      tft.setTextColor(textColor);
      tft.setTextSize(3);
      int subDirWidth = tft.textWidth(subDir);
      // Calculate centered position
      int subDirX = SECTION_LEFT + (SECTION_WIDTH - subDirWidth) / 2;
      int subDirY = SECTION2_TOP + (SECTION2_BOTTOM - SECTION2_TOP - 16) / 2;
      tft.setCursor(subDirX+5, subDirY);
      tft.print(subDir);
      
      // Clear angle difference area
      tft.fillRect(SECTION_LEFT+5, SECTION3_TOP, SECTION_WIDTH, SECTION3_BOTTOM - SECTION3_TOP, COLOR_BACKGROUND);
    }
    
    // Display angle difference
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    char diffBuf[4];
    sprintf(diffBuf, "%.0f", angleDiff);  // Format as integer
    int diffWidth = tft.textWidth(diffBuf);
    // Calculate centered position
    int diffX = SECTION_LEFT + (SECTION_WIDTH - diffWidth) / 2;
    int diffY = SECTION3_TOP + (SECTION3_BOTTOM - SECTION3_TOP - 16) / 2;
    tft.setCursor(diffX+5, diffY);
    tft.print(diffBuf);
    // Display degree symbol
    tft.setCursor(diffX + 30, diffY - 5);
    tft.setTextSize(1);
    tft.print('o');
    
    // Update last known values for next comparison
    last_heading = currentHeading;
    strcpy(last_mainDir, mainDir);
    strcpy(last_subDir, subDir);
    last_angleDiff = angleDiff;
    last_isFrozen = isFrozen;
  }
  
  // Update time if GPS time is valid
  if (gps.time.isValid()) {
    // Convert UTC time to Beijing time
    int bjHour, bjMinute, bjSecond;
    utcToBeijingTime(gps.time.hour(), gps.time.minute(), gps.time.second(), bjHour, bjMinute, bjSecond);
    
    // Set text properties for time display
    tft.setTextColor(textColor);
    tft.setTextSize(2);
    
    // Format full time string
    char fullTimeBuf[9];
    sprintf(fullTimeBuf, "%02d:%02d:%02d", bjHour, bjMinute, bjSecond);
    int fullTimeWidth = tft.textWidth(fullTimeBuf);
    // Calculate centered position
    int timeX = SECTION_LEFT + (SECTION_WIDTH - fullTimeWidth) / 2 + 6;
    int timeY = SECTION4_TOP + 18;
    
    // Initial time setup
    if (!timeInitialized) {
      // Clear time area and display full time
      tft.fillRect(SECTION_LEFT + 35, SECTION4_TOP + 5, 85, 30, COLOR_BACKGROUND);
      tft.setCursor(timeX, timeY);
      tft.print(fullTimeBuf);
      
      // Update initialization flag and last time values
      timeInitialized = true;
      last_hour = bjHour;
      last_minute = bjMinute;
      last_second = bjSecond;
    } 
    // Update full time when hour or minute changes
    else if (bjHour != last_hour || bjMinute != last_minute) {
      tft.fillRect(SECTION_LEFT + 35, SECTION4_TOP + 5, 85, 30, COLOR_BACKGROUND);
      tft.setCursor(timeX, timeY);
      tft.print(fullTimeBuf);
      
      last_hour = bjHour;
      last_minute = bjMinute;
      last_second = bjSecond;
    } 
    // Only update seconds when only seconds change
    else if (bjSecond != last_second) {
      // Format and display only seconds
      char secBuf[3];
      sprintf(secBuf, "%02d", bjSecond);
      int secWidth = tft.textWidth(secBuf);
      
      // Calculate position for seconds only
      int secX = timeX + tft.textWidth(fullTimeBuf) - secWidth;
      
      // Clear only seconds area and update
      tft.fillRect(secX - 5, timeY - 2, secWidth + 10, 35, COLOR_BACKGROUND);
      tft.setCursor(secX, timeY);
      tft.print(secBuf);
      
      last_second = bjSecond;
    }
  }
  
  // Update date if GPS date and time are valid
  if (gps.date.isValid() && gps.time.isValid()) {
    // Convert UTC date to Beijing date
    int bjYear, bjMonth, bjDay;
    utcToBeijingDate(gps.date.year(), gps.date.month(), gps.date.day(), bjYear, bjMonth, bjDay);
    
    // Static variables to track last displayed date
    static int last_bjYear = -1, last_bjMonth = -1, last_bjDay = -1;
    // Update date if it has changed
    if (bjYear != last_bjYear || bjMonth != last_bjMonth || bjDay != last_bjDay) {
      // Clear date area
      tft.fillRect(SECTION_LEFT+5, SECTION5_TOP, SECTION_WIDTH, SECTION5_BOTTOM - SECTION5_TOP, COLOR_BACKGROUND);
      // Set text properties
      tft.setTextColor(textColor);
      tft.setTextSize(2);
      
      // Format date string
      char dateBuf[11];
      sprintf(dateBuf, "%04d-%02d-%02d", bjYear, bjMonth, bjDay);
      int dateWidth = tft.textWidth(dateBuf);
      // Calculate centered position
      int dateX = SECTION_LEFT + (SECTION_WIDTH - dateWidth) / 2;
      int dateY = SECTION5_TOP + (SECTION5_BOTTOM - SECTION5_TOP - 16) / 2;
      // Display date
      tft.setCursor(dateX+5, dateY);
      tft.print(dateBuf);
      
      // Update last known date values for next comparison
      last_bjYear = bjYear;
      last_bjMonth = bjMonth;
      last_bjDay = bjDay;
    }
  }
}


// ======================
// Parse GPS Data
// ======================
/**
 * Parses GPS data received from the GPS module
 * This function reads all available serial data and uses the TinyGPSPlus library
 * to decode and extract relevant GPS information, updating global variables
 * with the latest valid GPS data.
 */
void parseGPSData() {
  // Process all available bytes from the GPS serial port
  while (GPS_Serial.available() > 0) {
    // Read a byte from GPS serial and pass it to TinyGPSPlus for decoding
    // Returns true when a complete sentence has been successfully encoded
    if (gps.encode(GPS_Serial.read())) {
      // Update location data if valid
      if (gps.location.isValid()) {
        latitude = gps.location.lat();    // Update latitude (degrees)
        longitude = gps.location.lng();   // Update longitude (degrees)
        gpsValid = true;                  // Set GPS validity flag to true
      } else {
        gpsValid = false;                 // Set GPS validity flag to false if location invalid
      }
      
      // Update altitude if valid (in meters)
      if (gps.altitude.isValid()) altitude = gps.altitude.meters();
      
      // Update speed if valid (in km/h)
      if (gps.speed.isValid()) speed = gps.speed.kmph();
      
      // Update heading/course if valid (in degrees)
      if (gps.course.isValid()) heading = gps.course.deg();
      
      // Update satellite count if valid
      if (gps.satellites.isValid()) satellites = gps.satellites.value();
      
      // Update accuracy if HDOP is valid
      // HDOP (Horizontal Dilution of Precision) is multiplied by 2 and rounded
      // to provide an estimated accuracy in meters
      if (gps.hdop.isValid()) accuracy = (int)round(gps.hdop.hdop() * 2.0);
    }
  }
}

// ======================
// Show Searching Screen
// ======================
/**
 * Displays the GPS signal searching screen
 * This screen is shown when the GPS module is searching for satellites
 * and hasn't acquired a valid signal yet.
 */
void showSearchingScreen() {
  // Clear the entire screen with background color
  tft.fillScreen(COLOR_BACKGROUND);
  
  // Display Nissan logo at specified position
  tft.pushImage(110, 30, NISSAN_LOGO_WIDTH, NISSAN_LOGO_HEIGHT, nissan_logo);
  
  // Set text properties and display searching message
  tft.setTextColor(COLOR_SEARCHING);
  tft.setTextSize(2);
  tft.setTextFont(1);
  tft.setCursor(40, 130);
  tft.print("Signal Searching...");

  // Set smaller text size for satellite and HDOP information
  tft.setTextSize(1);
  tft.setTextColor(COLOR_SEARCHING);
  
  // Display SATELLITES label
  tft.setCursor(40, 180);
  tft.print("SATELLITES:");
  
  // Display HDOP label
  tft.setCursor(200, 180);
  tft.print("HDOP:");
  
  // Clear the areas where satellite count and HDOP values will be displayed
  tft.fillRect(115, 180, 25, 20, COLOR_BACKGROUND);
  tft.fillRect(236, 180, 40, 20, COLOR_BACKGROUND);
  
  // Initialize with placeholder values
  tft.setCursor(115, 180);
  tft.print("-");  // Placeholder for satellite count
  tft.setCursor(236, 180);
  tft.print("-.-");  // Placeholder for HDOP value
}

// ======================
// Update Searching Information
// ======================
/**
 * Updates the satellite count and HDOP values on the searching screen
 * This function only updates the display when the values have changed
 * to minimize unnecessary screen redraws.
 */
void updateSearchingInfo() {
  // Get current satellite count from GPS module
  int satNum = gps.satellites.value();
  
  // Get HDOP value if valid, otherwise set to 0.0
  float hdop = gps.hdop.isValid() ? gps.hdop.hdop() : 0.0;
  
  // Static variables to track previous values (for change detection)
  static int lastSatellites = -1;
  static float lastHdop = -1.0;
  
  // Only update display if values have changed
  if (satNum != lastSatellites || (hdop > 0 && hdop != lastHdop)) {
    // Clear the areas where satellite count and HDOP values will be displayed
    tft.fillRect(115, 180, 25, 20, COLOR_BACKGROUND);
    tft.fillRect(236, 180, 40, 20, COLOR_BACKGROUND);
    
    // Set text properties for displaying values
    tft.setTextColor(COLOR_SEARCHING);
    tft.setTextSize(1);
    tft.setTextFont(1);
    
    // Display satellite count or placeholder if invalid
    tft.setCursor(115, 180);
    if (satNum > 0) {
      tft.print(satNum);
    } else {
      tft.print("-");
    }
    
    // Display HDOP value or placeholder if invalid
    tft.setCursor(236, 180);
    if (hdop > 0) {
      // Format HDOP to show one decimal place
      int hdopInt = (int)hdop;
      int hdopDec = (int)((hdop - hdopInt) * 10);
      tft.print(hdopInt);
      tft.print(".");
      tft.print(hdopDec);
    } else {
      tft.print("-.-");
    }
    
    // Update previous values for next comparison
    lastSatellites = satNum;
    lastHdop = hdop;
  }
}

// ======================
// Switch to Main UI
// ======================
/**
 * Switches from searching screen to the main GPS navigation interface
 * This function is called when a valid GPS signal is acquired.
 */
void switchToMainUI() {
  // Log transition to main UI
  Serial.println("GPS signal acquired, switching to main UI...");
  
  // Clear entire screen to prevent残影 (ghost images)
  tft.fillScreen(COLOR_BACKGROUND);
  
  // Initialize the main UI components
  initUI();
  
  // Update global state to indicate we're no longer in searching screen
  inSearchingScreen = false;
  
  // Reset cached values to ensure full UI update
  last_latitude = -999.0;
  last_longitude = -999.0;
  last_speed = -999.0;
  last_heading = -999.0;
  last_satellites = -1;
  last_gpsValid = false;
  
  // Reset update timer
  lastUpdateTime = millis();
}

// ======================
// Switch to Search Screen
// ======================
/**
 * Switches from main UI to searching screen
 * This function is called when the GPS signal is lost or insufficient.
 */
void switchToSearchScreen() {
  // Log transition to searching screen
  Serial.println("GPS signal lost, switching to search screen...");
  
  // Clear entire screen to prevent残影 (ghost images)
  tft.fillScreen(COLOR_BACKGROUND);
  
  // Display searching screen without blocking delay
  showSearchingScreen();
  
  // Update global state to indicate we're in searching screen
  inSearchingScreen = true;
  
  // Reset search screen update timer
  lastSearchUpdateTime = millis();
}

// ======================
// Setup - Fixed: Correctly initialize state variables
// ======================
/**
 * Arduino setup function - runs once at system startup
 * Initializes hardware, serial communication, display, and GPS module
 * Determines initial screen based on GPS signal availability
 */
void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  
  // Initialize GPS serial communication with specified baud rate and pins
  GPS_Serial.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  // Initialize TFT display with specified rotation and byte order
  tft.init();
  tft.setRotation(TFT_ROTATION);
  tft.setSwapBytes(true);
  
  // Initialize system state variables
  inSearchingScreen = false;  // Default to not in searching screen
  uiInitialized = false;      // UI not yet initialized
  
  // Log system initialization
  Serial.println("System initialized, checking GPS status...");
  
  // Parse initial GPS data to determine current status
  parseGPSData();
  
  // Determine initial screen based on satellite count
  if (satellites > 5) {
    Serial.println("GPS signal already available!");
    switchToMainUI();  // Show main UI if sufficient satellites found
  } else if (satellites <= 5) {
    Serial.println("Waiting for GPS signal...");
    switchToSearchScreen();  // Show searching screen if insufficient satellites
  }
  
  // Log successful initialization
  Serial.println("GPS Navigator Initialized - Fixed Screen Switch Version");
}

// ======================
// Main Loop - Maintain existing logic
// ======================
/**
 * Arduino main loop function - runs continuously
 * Manages GPS data parsing, screen switching, and UI updates
 */
void loop() {
  // Continuously parse GPS data
  parseGPSData();
  
  // Check GPS status and switch screens if needed
  if (satellites > 5 && inSearchingScreen) {
    // Switch to main UI when GPS signal is acquired
    switchToMainUI();
  } else if (satellites <= 5 && !inSearchingScreen) {
    // Switch to searching screen when GPS signal is lost
    switchToSearchScreen();
  }
  
  // Update appropriate screen based on current state
  if (inSearchingScreen) {
    // Update searching screen information at specified interval
    if (millis() - lastSearchUpdateTime >= SEARCH_UPDATE_INTERVAL) {
      updateSearchingInfo();
      lastSearchUpdateTime = millis();
    }
  } else {
    // Update main UI at specified interval
    if (millis() - lastUpdateTime >= UPDATE_INTERVAL) {
      updateHeader();     // Update header information
      updateLeftPanel();  // Update left panel content
      updateRightPanel(); // Update right panel content
      lastUpdateTime = millis();
    }
  }
  
  // Small delay to prevent excessive CPU usage
  delay(10);
}